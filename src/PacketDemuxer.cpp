#include "PacketDemuxer.h"

#include <algorithm>
#include <functional>
#include <iostream>

#ifdef WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <arpa/inet.h>
#endif
#include <assert.h>

/**
    Create a new demuxer that will receive packets from the specified socket.

    This object is guaranteed to only ever read from the socket.
*/
PacketDemuxer::PacketDemuxer(AbstractReader& socket, const std::vector<std::string>& packetIds)
    : m_packetIds(packetIds),
      m_transport(socket),
      m_transportError(false),
      m_receiverThread(std::bind(&PacketDemuxer::receiveLoop, std::ref(*this))) {
  m_transport.setBlocking(false);
}

PacketDemuxer::~PacketDemuxer() {
  signalTransportError();  /// Causes receive-thread to exit (@todo use better method)

  // Check if there any remaining subscribers:
  warnAboutSubscribers();

  try {
    m_receiverThread.join();
  } catch (const std::system_error& e) {
    std::clog << "Error: " << e.what() << std::endl;
  }
}

/**
    Return false if there have been any communication errors, true otherwise.
*/
bool PacketDemuxer::ok() const {
  return m_transportError == false;
}

/**
 *  Subscribe to a packet type and register a callback.
 *
 *  Returns a subscription object that will automatically unsubsribe
 *  (and deregister the callback) when it goes out of scope.
*/
PacketSubscription PacketDemuxer::subscribe(const std::string& typeName, PacketSubscriber::CallBack callback) {
  const IdManager::PacketType type = m_packetIds.toId(typeName);

  std::lock_guard<std::mutex> guard(m_subscriberLock);
  SubscriptionEntry::second_type& queue = m_subscribers[type];
  queue.emplace_back(new PacketSubscriber(type, *this, callback));  /// @note Can't use make_shared because of protected constructor.

  std::clog << "New subscriber for '" << typeName << "'" << std::endl;

  return PacketSubscription(queue.back());
}

void PacketDemuxer::unsubscribe(const PacketSubscriber* pSubscriber) {
  const IdManager::PacketType type = pSubscriber->getType();
  std::clog << "Removing subscriber for '" << m_packetIds.toString(type) << "'" << std::endl;

  std::lock_guard<std::mutex> guard(m_subscriberLock);
  SubscriptionEntry::second_type& queue = m_subscribers[type];

  // Search through all subscribers of this type for the specific subscriber:
  auto itr = std::remove_if(queue.begin(), queue.end(), [pSubscriber](const PacketDemuxer::SubscriberPtr& subscriber) {
    return subscriber.get() == pSubscriber;
  });

  assert(itr != queue.end());
  queue.erase(itr);
}

/**
    @return true if the specified subscriber is subscribed to this demuxer.
*/
bool PacketDemuxer::isSubscribed(const PacketSubscriber* pSubscriber) const {
  const IdManager::PacketType type = pSubscriber->getType();
  auto queueItr                    = m_subscribers.find(type);

  if (queueItr == m_subscribers.end()) {
    return false;
  }

  const SubscriptionEntry::second_type& queue = queueItr->second;

  // Search through all subscribers of this type for the specific subscriber:
  auto itr = std::find_if(queue.begin(), queue.end(), [pSubscriber](const PacketDemuxer::SubscriberPtr& subscriber) {
    return subscriber.get() == pSubscriber;
  });

  return itr != queue.end();
}

/**
    This function loops receiving data from the transport layer, splitting
    it into packets. The loop exits if there is a transport error
    (e.g. if the other end hangs up).

    Runs asnchronously in its own thread :- it is passed
    using std::bind to a SimpleAsyncFunction object.
*/
void PacketDemuxer::receiveLoop() {
  std::clog << "PacketDemuxer::receiveLoop() entered." << std::endl;
  ComPacket packet;
  constexpr int helloTimeoutInMilliseconds = 2000;
  receiveHelloMessage(packet, helloTimeoutInMilliseconds);

  while (m_transportError == false) {
    constexpr int timeoutInMilliseconds = 1000;
    if (receivePacket(packet, timeoutInMilliseconds)) {
      const IdManager::PacketType packetType = packet.getType();  // Need to cache this before we use std::move
      //std::clog << GetIdManager().toString( packetType ) << " bytes: " << packet.getDataSize() << std::endl;
      auto sptr = std::make_shared<ComPacket>(std::move(packet));

      if (packetType == IdManager::ControlPacket) {
        // Control messages are used by the muxer to communicate
        // with the demuxer (this is a one way protocol).
        handleControlMessage(sptr);
      } else {
        // Post the new packet to the message queues of all the subscribers for this packet type:
        std::lock_guard<std::mutex> guard(m_subscriberLock);
        SubscriptionEntry::second_type& queue = m_subscribers[packetType];
        //std::clog << "Posting '" << m_packetIds.toString(packetType) << "' to " << queue.size() << " subscribers" << std::endl;
        for (auto& subscriber : queue) {
          subscriber->m_callback(sptr);
        }
      }
    }
  }

  std::clog << "PacketDemuxer::receiveLoop() exited." << std::endl;
}

/**
    @param packet If return value is true then packet will contain the new data, if false packet remains unchanged.
    @return false on comms error, true if successful.
*/
bool PacketDemuxer::receivePacket(ComPacket& packet, const int timeoutInMilliseconds) {
  if (m_transport.readyForReading(timeoutInMilliseconds) == false) {
    return false;
  }

  uint32_t type = 0;
  uint32_t size = 0;

  // For the first read we generate a transport error on zero bytes
  // (because readyForReading() said there were bytes available):
  ///@note - the above is incorrect as ReadyForReading uses POLLIN which
  /// also returns true if there is out of band data ready for reading.
  size_t byteCount = sizeof(uint32_t);
  bool ok          = readBytes(reinterpret_cast<uint8_t*>(&type), byteCount, false);
  if (!ok) {
    return false;
  }

  byteCount = sizeof(uint32_t);
  ok        = readBytes(reinterpret_cast<uint8_t*>(&size), byteCount);
  if (!ok) {
    return false;
  }

  type = ntohl(type);
  size = ntohl(size);

  ComPacket p(static_cast<IdManager::PacketType>(type), size);
  byteCount = p.getDataSize();
  ok        = readBytes(reinterpret_cast<uint8_t*>(p.getDataPtr()), byteCount);
  if (!ok) {
    return false;
  }

  std::swap(p, packet);
  assert(packet.getType() != IdManager::InvalidPacket);  // Catch invalid packets at the lowest level.

  return true;
}

/**
    Loop to guarantee the number of bytes requested are actually read.

    On error (return of false) size will contain the number of bytes that were remaining to be read.

    @param transportErrorOnZeroBytes If this is true then in the case that read() returns zero bytes
    a transport error will be signalled and ReadBytes() will return false.

    @return true if all bytes were written, false if there was an error at any point. Also see impact
    of the transportErrorOnZeroBytes parameter on the return value.
*/
bool PacketDemuxer::readBytes(uint8_t* buffer, size_t& size, bool transportErrorOnZeroBytes) {
  while (size > 0 && m_transportError == false) {
    const int n = m_transport.read(reinterpret_cast<char*>(buffer), size);

    if (n < 0 || (n == 0 && transportErrorOnZeroBytes)) {
      std::clog << "Signalling transport error because bytes read := " << n << std::endl;
      signalTransportError();
      return false;
    }

    size -= n;
    buffer += n;
  }

  if (m_transportError) {
    return false;
  }

  return true;
}

void PacketDemuxer::signalTransportError() {
  m_transportError = true;
}

/**
    Receive the hello message. The first packet sent from a PacketMuxer to
    a demuxer will always be an Hello control message. If the first message
    is not such a message it is considered a transport error and the demuxer
    will terminate for safety.

    This should make it extremely unlikely that an accidental connection
    can cause the demuxer to do anything dodgy. This is not intended to be
    a security measure, if security is important the application must perform
    its own secure handshaking procedure at a higher level (external to the
    Muxer/Demuxer system).
*/
void PacketDemuxer::receiveHelloMessage(ComPacket& packet, const int timeoutInMillisecs) {
  if (receivePacket(packet, timeoutInMillisecs)) {
    bool failHard = true;

    // Very first packet should be a 'Hello' control packet:
    if (packet.getType() == IdManager::ControlPacket) {
      auto sptr          = std::make_shared<ComPacket>(std::move(packet));
      ControlMessage msg = getControlMessage(sptr);
      if (msg == ControlMessage::Hello) {
        failHard = false;
      }
    }

    if (failHard) {
      std::cerr << "Error in PacketDemuxer::Receive() - first message was not 'Hello'." << std::endl;
      signalTransportError();
    }
  }
}

void PacketDemuxer::handleControlMessage(const ComPacket::ConstSharedPacket&) {
}

ControlMessage PacketDemuxer::getControlMessage(const ComPacket::ConstSharedPacket& sptr) {
  typedef std::underlying_type<ControlMessage>::type UnderlyingType;
  const UnderlyingType netMsg = *reinterpret_cast<const UnderlyingType*>((sptr->getDataPtr()));
  return static_cast<ControlMessage>(netMsg);
}

void PacketDemuxer::warnAboutSubscribers() {
  std::lock_guard<std::mutex> guard(m_subscriberLock);
  for (const auto& entry : m_subscribers) {
    const size_t n = entry.second.size();
    if (n > 0) {
      std::clog << "Warning: there are " << n << " live subscribers for '" << m_packetIds.toString(entry.first) << "'" << std::endl;
    }
  }
}
