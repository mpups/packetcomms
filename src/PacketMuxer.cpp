#include "PacketMuxer.h"

#include <arpa/inet.h>

#include <algorithm>
#include <iostream>
#include <type_traits>

/**
    Create a new muxer that will send packets over the specified
    socket.

    This object is guaranteed to only ever write to the socket.
*/
PacketMuxer::PacketMuxer(AbstractWriter& socket, const std::vector<std::string>& packetIds)
    : m_packetIds(packetIds),
      m_numPosted(0),
      m_numSent(0),
      m_transport(socket),
      m_transportError(false),
      m_sendThread(std::bind(&PacketMuxer::sendLoop, std::ref(*this))) {
  m_transport.setBlocking(false);
}

PacketMuxer::~PacketMuxer() {
  {
    std::lock_guard<std::recursive_mutex> guard(m_txLock);
    m_transportError = true;  /// Causes threads to exit (@todo use better method)
    m_txReady.notify_all();
  }

  try {
    m_sendThread.join();
  } catch (const std::system_error& e) {
    std::clog << "Error: " << e.what() << std::endl;
  }
}

/**
    Return false if ther ehave been any communication errors, true otherwise.
*/
bool PacketMuxer::ok() const {
  return m_transportError == false;
}

/**
    This function loops sending all the queued packets over the
    transport layer. The loop exits if there is a transport error
    (e.g. if the other end hangs up).

    Runs asnchronously in its own thread :- it is passed
    using std::bind to a SimpleAsyncFunction object.
*/
void PacketMuxer::sendLoop() {
  std::clog << "PacketMuxer::sendLoop() entered." << std::endl;

  sendControlMessage(ControlMessage::Hello);

  // Grab the lock for the transmit/send queues:
  std::unique_lock<std::recursive_mutex> guard(m_txLock);

  while (m_transportError == false) {
    if (m_numPosted == m_numSent) {
      // Atomically relinquish lock for send queues and wait
      // until new data is posted (don't care to which queue it
      // is posted, hence one condition variable for all queues):
      std::cv_status status = m_txReady.wait_for(guard, std::chrono::seconds(1));
      if (status == std::cv_status::timeout) {
        // If there are no packets to send after waiting for 1 second then
        // send a 'HeartBeat' message - this serves two purposes:
        // 1. Lets the other side know we are still connected.
        // 2. Lets this side detect if the other side has hung up or crashed (Sending the packet will fail at the socket level).
        // These are similar to TCP keep-alive messages, but there is no defined standard about how to use TCP keepalive to
        // achieve the same behaviour:
        sendControlMessage(ControlMessage::HeartBeat);
      }
    }

    // Send in priority order:
    /// @todo if one queue is always full we could get starvation here...how to fix? Counter for each queue
    /// indicating how many send loops it has been not empty?
    /// (But in that case the system is overloaded anyway so what would we like to do when overloaded?)
    for (auto& pair : m_txQueues) {
      sendAll(pair.second);
    }
  }

  std::clog << "PacketMuxer::sendLoop() exited." << std::endl;
}

/**
    Send all the packets from the FIFO container. The container will
    be empty after calling this function, regardless of whether there
    were any errors from the transport layer.

    @note Assumes you have acquired the appropriate
    lock to access the specified PacketContainer.

    @bug Bug on shutdown - the queues are always empty so we never attempt to send any packets
    and therefore never get to state where m_transportError is true, hence the caller loops forever.
*/
void PacketMuxer::sendAll(ComPacket::PacketContainer& packets) {
  while (m_transportError == false && packets.empty() == false) {
    // Here we must send before popping to guarantee the shared_ptr is valid for the lifetime of SendPacket.
    sendPacket(*(packets.front().get()));
    packets.pop();
    m_numSent += 1;
  }
}

/**
    Used internally to send a packet over the transport layer.

    Header is:
    type (4-bytes)
    data-size (4-bytes)

    followed by the data payload.
*/
void PacketMuxer::sendPacket(const ComPacket& packet) {
  assert(packet.getType() != IdManager::InvalidPacket);  // Catch attempts to send invalid packets

  // Write the type as an unsigned 32-bit integer in network byte order:
  size_t writeCount = sizeof(uint32_t);
  uint32_t type     = htonl(static_cast<uint32_t>(packet.getType()));
  bool ok           = writeBytes(reinterpret_cast<const uint8_t*>(&type), writeCount);

  // Write the data size:
  uint32_t size = htonl(packet.getDataSize());
  writeCount    = sizeof(uint32_t);
  ok &= writeBytes(reinterpret_cast<const uint8_t*>(&size), writeCount);

  // Write the byte data:
  writeCount = packet.getDataSize();
  ok &= writeBytes(reinterpret_cast<const uint8_t*>(packet.getDataPtr()), writeCount);

  m_transportError = !ok;
}

/**
    Loop to guarantee the number of bytes requested are actually written.

    On error (return of false) size will contain the number of bytes that were remaining to be written.

    @return true if all bytes were written, false if there was an error at any point.
*/
bool PacketMuxer::writeBytes(const uint8_t* buffer, size_t& size) {
  while (size > 0) {
    int n = m_transport.write(reinterpret_cast<const VectorStream::CharType*>(buffer), size);
    if (n < 0 || m_transportError) {
      return false;
    }

    size -= n;
    buffer += n;
  }

  return true;
}

void PacketMuxer::signalPacketPosted() {
  m_numPosted += 1;
  m_txReady.notify_one();
}

void PacketMuxer::sendControlMessage(ControlMessage msg) {
  emplacePacket(IdManager::ControlString, reinterpret_cast<VectorStream::CharType*>(&msg), sizeof(std::underlying_type<ControlMessage>::type));
}
