#define BOOST_TEST_MAIN
#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

#include <boost/test/unit_test.hpp>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/cereal.hpp>

#include "../src/ComPacket.h"
#include "../src/IdManager.h"
#include "../src/PacketComms.h"
#include "../src/VectorStream.h"
#include "../src/network/Ipv4Address.h"
#include "../src/network/Socket.h"
#include "../src/network/TcpSocket.h"
#include "../src/network/UdpSocket.h"
#include "../src/Sync.h"
#include "MockSockets.h"

#ifdef WIN32
  #include <windows.h>
  #include <thread>
  #include <chrono>
#else
  #include <pthread.h>
  #include <unistd.h>
#endif
#include <memory>

struct Type1 {
  int32_t axis1;
  int32_t axis2;
  int32_t max;
};

struct Type2 {
  bool b;
  float f;
  Type1 t;
};

template <class Archive>
void serialize(Archive& archive,
               Type1& t1) {
  archive(t1.axis1, t1.axis2, t1.max);
}

template <class Archive>
void serialize(Archive& archive,
               Type2& t2) {
  archive(t2.b, t2.f, t2.t);
}

BOOST_AUTO_TEST_CASE(TestVectorStream) {
  Type1 in1{1, 2, 3};
  constexpr int intIn = 35;
  Type1 in2{4000, 5000, 6000};
  Type2 in3{true, 0.1f, {10, 11, 12}};

  const ComPacket pkt;
  {
    VectorOutputStream vs;
    std::ostream archiveStream(&vs);
    cereal::PortableBinaryOutputArchive archive(archiveStream);
    archive(in1);
    archive(intIn);
    archive(in2, in3);

    // If we wanted to emplace the serialised data into the
    // comms-system then we would need to move the storage out
    // of VectorOutputStream into a ComPacket so test that here:
    const_cast<ComPacket&>(pkt) = ComPacket(IdManager::InvalidPacket, std::move(vs.get()));
  }

  Type2 out3;
  Type1 out1;
  Type1 out2;
  {
    const VectorStream::Buffer& buffer = pkt.getData();
    const size_t sizeBefore            = buffer.size();
    VectorInputStream vsIn(buffer);
    std::istream achiveInputStream(&vsIn);
    cereal::PortableBinaryInputArchive archive(achiveInputStream);
    int intOut = 0;
    archive(out1, intOut, out2, out3);
    const size_t sizeAfter = buffer.size();
    BOOST_CHECK_EQUAL(intOut, intIn);
    // Need to check because VectorInputStream uses const cast inside
    BOOST_CHECK_EQUAL(sizeBefore, sizeAfter);
  }

  BOOST_CHECK_EQUAL(in1.axis1, out1.axis1);
  BOOST_CHECK_EQUAL(in1.axis2, out1.axis2);
  BOOST_CHECK_EQUAL(in1.max, out1.max);
  BOOST_CHECK_EQUAL(in2.axis1, out2.axis1);
  BOOST_CHECK_EQUAL(in2.axis2, out2.axis2);
  BOOST_CHECK_EQUAL(in2.max, out2.max);

  BOOST_CHECK_EQUAL(out3.b, in3.b);
  BOOST_CHECK_EQUAL(out3.f, in3.f);
  BOOST_CHECK_EQUAL(out3.t.axis1, in3.t.axis1);
  BOOST_CHECK_EQUAL(out3.t.axis2, in3.t.axis2);
  BOOST_CHECK_EQUAL(out3.t.max, out3.t.max);
}

BOOST_AUTO_TEST_CASE(TestIdManager) {
  IdManager packetIds({"Type1", "Type2", "Type3"});

  const IdManager::PacketType ctrl = packetIds.toId(IdManager::ControlString);
  BOOST_CHECK_EQUAL(0u, packetIds.toId(IdManager::InvalidString));
  BOOST_CHECK_EQUAL(1u, ctrl);
  BOOST_CHECK_EQUAL(ctrl + 1, packetIds.toId("Type1"));
  BOOST_CHECK_EQUAL(ctrl + 2, packetIds.toId("Type2"));
  BOOST_CHECK_EQUAL(ctrl + 3, packetIds.toId("Type3"));
  BOOST_CHECK_EQUAL(IdManager::InvalidString, packetIds.toString(0));
  BOOST_CHECK_EQUAL(IdManager::ControlString, packetIds.toString(ctrl));
  BOOST_CHECK_EQUAL("Type1", packetIds.toString(ctrl + 1));
  BOOST_CHECK_EQUAL("Type2", packetIds.toString(ctrl + 2));
  BOOST_CHECK_EQUAL("Type3", packetIds.toString(ctrl + 3));
}

BOOST_AUTO_TEST_CASE(TestComPacket) {
  ComPacket pkt;
  BOOST_CHECK(pkt.getType() == IdManager::InvalidPacket);
  BOOST_CHECK_EQUAL(0, pkt.getDataSize());
  BOOST_CHECK_EQUAL(nullptr, pkt.getDataPtr());

  constexpr int size                 = 6;
  VectorStream::CharType bytes[size] = "hello";
  ComPacket pkt2(IdManager::InvalidPacket, bytes, size);
  BOOST_CHECK_EQUAL(size, pkt2.getDataSize());
  BOOST_CHECK_NE(nullptr, pkt2.getDataPtr());

  // Test packet contains the byte data:
  for (int i = 0; i < size; ++i) {
    BOOST_CHECK_EQUAL(bytes[i], pkt2.getData()[i]);
  }

  // Create an Odometry packet with uninitialised data:
  constexpr int size2 = 17;
  ComPacket pkt3(IdManager::ControlPacket, size2);
  BOOST_CHECK_EQUAL(size2, pkt3.getDataSize());
  BOOST_CHECK_NE(nullptr, pkt3.getDataPtr());

  // Test that pkt3 gets moved (and made invalid):
  BOOST_CHECK_NE(size, size2);
  BOOST_CHECK_EQUAL(IdManager::ControlPacket, pkt3.getType());
  pkt2 = std::move(pkt3);
  BOOST_CHECK_EQUAL(IdManager::InvalidPacket, pkt3.getType());
  BOOST_CHECK_EQUAL(size2, pkt2.getDataSize());

  // Test move constructor on pkt2 (which was pkt3):
  ComPacket pkt4(std::move(pkt2));
  BOOST_CHECK_EQUAL(size2, pkt4.getDataSize());
  BOOST_CHECK_EQUAL(IdManager::ControlPacket, pkt4.getType());
  BOOST_CHECK_EQUAL(0, pkt2.getDataSize());
  BOOST_CHECK_EQUAL(IdManager::InvalidPacket, pkt2.getType());
}

BOOST_AUTO_TEST_CASE(TestSimpleQueue) {
  SimpleQueue q;

  // Check new queue is empty:
  BOOST_CHECK_EQUAL(0, q.size());
  BOOST_CHECK(q.empty());

  // Add a packet onto queue:
  constexpr int pktSize = 7;
  auto sptr             = std::make_shared<ComPacket>(IdManager::ControlPacket, pktSize);
  BOOST_CHECK_EQUAL(1, sptr.use_count());
  q.emplace(sptr);
  BOOST_CHECK_EQUAL(1, q.size());
  BOOST_CHECK_EQUAL(2, sptr.use_count());

  // Check SimpleQueue::Front() returns shared_ptr to same packet:
  auto sptr2 = q.front();
  BOOST_CHECK_EQUAL(3, sptr.use_count());
  BOOST_CHECK(sptr == sptr2);

  // Check queue locking:
  SimpleQueue::LockedQueue lockedQueue = q.lock();

  // Whilts queue is not empty, and we have a lock
  // check WaitNotempty() returns without blocking:
  bool blocked     = true;
  auto asyncWaiter = std::thread([&]() {
    // Wait asynchronously so failing test does not block forever:
    lockedQueue.waitNotempty();
    blocked = false;
  });
  try {
    asyncWaiter.join();
  } catch (const std::system_error& e) {
    std::clog << "Error: " << e.what() << std::endl;
  }
#ifdef WIN32
  std::this_thread::sleep_for(std::chrono::microseconds(1000));
#else
  usleep(1000);  // sleep so async func has time to finish
#endif
  BOOST_CHECK(!blocked);

  // Check popping makes it empty again:
  q.pop();
  BOOST_CHECK_EQUAL(0, q.size());
  BOOST_CHECK(q.empty());
  BOOST_CHECK_EQUAL(2, sptr.use_count());
  sptr2.reset();
}

BOOST_AUTO_TEST_CASE(TestPacketMuxerExitsCleanly) {
  AlwaysFailSocket mockSocket;
  PacketMuxer muxer(mockSocket, {});
  while (muxer.ok()) {
  }
}

BOOST_AUTO_TEST_CASE(TestPacketMuxer) {
  constexpr int testPayloadSize = 11;
  MuxerTestSocket socket(testPayloadSize);
  PacketMuxer muxer(socket, {"MockPacket"});

  // Transport should be ok here:
  BOOST_CHECK(muxer.ok());

#ifdef WIN32
  std::this_thread::sleep_for(std::chrono::microseconds(100000));
#else
  usleep(100000);
#endif

  // Emplace payload:
  VectorStream::CharType payload[testPayloadSize];
  muxer.emplacePacket("MockPacket", payload, testPayloadSize);

  // Post identical payload:
  muxer.emplacePacket("MockPacket", payload, testPayloadSize);

  // Give the muxer time to send:
#ifdef WIN32
  std::this_thread::sleep_for(std::chrono::microseconds(100000));
#else
  usleep(100000);
#endif

  // Did it send all posted packets before exiting?
  BOOST_CHECK_EQUAL(muxer.getNumPosted(), muxer.getNumSent());
}

BOOST_AUTO_TEST_CASE(TestDemuxerExitsCleanly) {
  AlwaysFailSocket socket;
  PacketDemuxer demuxer(socket, {});
  while (demuxer.ok()) {
  }
}

BOOST_AUTO_TEST_CASE(TestPacketDemuxer) {
  AlwaysFailSocket socket;
  PacketDemuxer demuxer(socket, {});
}

const int MSG_SIZE           = 8;
const char TEST_MSG[MSG_SIZE] = "1234abc";
const char UDP_MSG[]     = "Udp connection-less Datagram!";

/*
    Server waits for client to connect then reads the test message from the client.
*/
void* RunTcpServerThread(void* arg) {
  TcpSocket* server = (TcpSocket*)arg;
  auto connection   = server->Accept();

  BOOST_CHECK(connection != nullptr);
  if (connection) {
    char msg[256] = "";

    BOOST_CHECK(connection->IsValid());
    int bytes = connection->read(msg, MSG_SIZE);
    BOOST_CHECK_EQUAL(bytes, MSG_SIZE);
    BOOST_CHECK_EQUAL(msg, TEST_MSG);

    connection->Shutdown();
  }

  return 0;
}

BOOST_AUTO_TEST_CASE(TestTcp) {
  const int TEST_PORT = 2000;
  TcpSocket server;
  BOOST_CHECK(server.IsValid());
  server.Bind(TEST_PORT);
  server.Listen(0);

  // Create a server thread for testing:
#ifdef WIN32
  std::thread serverThread(RunTcpServerThread, (void*)&server);
#else
  pthread_t serverThread;
  pthread_create(&serverThread, 0, RunTcpServerThread, (void*)&server);
#endif

  // client connects and sends a test message
  TcpSocket client;
  BOOST_REQUIRE(client.Connect("localhost", TEST_PORT));

  client.write(TEST_MSG, MSG_SIZE);
  client.Shutdown();

#ifdef WIN32
  serverThread.join();
#else
  pthread_join(serverThread, 0);
#endif
}

/*
    Server waits for messages from client and checks they are correct.
*/
void* RunUdpServerThread(void* arg) {
  UdpSocket* server = (UdpSocket*)arg;

  char msg[256] = "";

  // First read the connection-less udp message:
  const int udpMsgSize = strlen(UDP_MSG) + 1;
  int bytes            = server->read(msg, udpMsgSize);
  BOOST_CHECK_EQUAL(bytes, udpMsgSize);
  BOOST_CHECK_EQUAL(msg, UDP_MSG);

  // Then read the connected message:
  bytes = server->read(msg, MSG_SIZE);
  BOOST_CHECK_EQUAL(bytes, MSG_SIZE);
  BOOST_CHECK_EQUAL(msg, TEST_MSG);

  return 0;
}

BOOST_AUTO_TEST_CASE(TestUdp) {
  const int TEST_PORT = 3000;
  UdpSocket server;
  BOOST_CHECK(server.IsValid());
  server.Bind(TEST_PORT);

  // Create a server thread for testing:
#ifdef WIN32
  std::thread serverThread(RunUdpServerThread, (void*)&server);
#else
  pthread_t serverThread;
  pthread_create(&serverThread, 0, RunUdpServerThread, (void*)&server);
#endif

  // Test connection-less datagram:
  Ipv4Address localhost("127.0.0.1", TEST_PORT);
  BOOST_REQUIRE(localhost.IsValid());
  UdpSocket client;
  int bytesSent = client.SendTo(localhost, UDP_MSG, strlen(UDP_MSG) + 1);
  BOOST_REQUIRE_EQUAL(strlen(UDP_MSG) + 1, bytesSent);

  // Test datagram to a connection:
  client.Connect("127.0.0.1", TEST_PORT);
  client.write(TEST_MSG, MSG_SIZE);

#ifdef WIN32
  serverThread.join();
#else
  pthread_join(serverThread, 0);
#endif

  client.Shutdown();
}

BOOST_AUTO_TEST_CASE(TestIpv4Address) {
  const int TEST_PORT = 3000;
  Ipv4Address localhost("localhost", TEST_PORT);
  BOOST_REQUIRE(localhost.IsValid());

  std::string hostName;
  localhost.GetHostName(hostName);
#ifndef WIN32
  // On Windows, localhost resolves to actual machine hostname, which is correct behavior
  BOOST_CHECK_EQUAL("localhost", hostName.c_str());
#endif
  std::string hostIP;
  localhost.GetHostAddress(hostIP);
  BOOST_CHECK_EQUAL("127.0.0.1", hostIP.c_str());

  BOOST_CHECK_EQUAL(TEST_PORT, localhost.GetPort());

  Ipv4Address nonsense("@nonsense.ww.arg.?", 120);
  BOOST_REQUIRE(!nonsense.IsValid());

  Ipv4Address uninitialised;
  BOOST_REQUIRE(!uninitialised.IsValid());

  Ipv4Address copy(localhost);
  BOOST_REQUIRE(localhost.IsValid());
}
