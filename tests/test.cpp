#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>

#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

#include "../src/VectorStream.h"
#include "../src/ComPacket.h"
#include "../src/PacketComms.h"
#include "../src/IdManager.h"
#include "../src/network/Socket.h"
#include "../src/network/TcpSocket.h"
#include "../src/network/UdpSocket.h"
#include "../src/network/Ipv4Address.h"
#include "MockSockets.h"

#include <memory>
#include <pthread.h>

struct Type1
{
    int32_t axis1;
    int32_t axis2;
    int32_t max;
};

struct Type2
{
    bool b;
    float f;
    Type1 t;
};

template<class Archive>
void serialize(Archive& archive,
               Type1& t1)
{
    archive( t1.axis1, t1.axis2, t1.max);
}

template<class Archive>
void serialize(Archive& archive,
               Type2& t2)
{
    archive( t2.b, t2.f, t2.t);
}

BOOST_AUTO_TEST_CASE(TestVectorStream) {
    Type1 in1{1,2,3};
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
        archive(in2,in3);

        // If we wanted to emplace the serialised data into the
        // comms-system then we would need to move the storage out
        // of VectorOutputStream into a ComPacket so test that here:
        const_cast<ComPacket&>(pkt) = ComPacket(IdManager::InvalidPacket,std::move(vs.Get()));
    }

    Type2 out3;
    Type1 out1;
    Type1 out2;
    {
        const VectorStream::Buffer& buffer = pkt.GetData();
        const size_t sizeBefore = buffer.size();
        VectorInputStream vsIn(buffer);
        std::istream achiveInputStream(&vsIn);
        cereal::PortableBinaryInputArchive archive(achiveInputStream);
        int intOut = 0;
        archive( out1, intOut, out2, out3);
        const size_t sizeAfter = buffer.size();
        BOOST_CHECK_EQUAL(intOut, intIn);
        // Need to check because VectorInputStream uses const cast inside
        BOOST_CHECK_EQUAL(sizeBefore, sizeAfter);
    }

    BOOST_CHECK_EQUAL(in1.axis1, out1.axis1);
    BOOST_CHECK_EQUAL(in1.axis2, out1.axis2);
    BOOST_CHECK_EQUAL(in1.max,   out1.max);
    BOOST_CHECK_EQUAL(in2.axis1, out2.axis1);
    BOOST_CHECK_EQUAL(in2.axis2, out2.axis2);
    BOOST_CHECK_EQUAL(in2.max,   out2.max);

    BOOST_CHECK_EQUAL(out3.b,       in3.b);
    BOOST_CHECK_EQUAL(out3.f,       in3.f);
    BOOST_CHECK_EQUAL(out3.t.axis1, in3.t.axis1);
    BOOST_CHECK_EQUAL(out3.t.axis2, in3.t.axis2);
    BOOST_CHECK_EQUAL(out3.t.max,   out3.t.max);
}

BOOST_AUTO_TEST_CASE(TestIdManager) {
    IdManager packetIds({ "Type1", "Type2", "Type3" });

    const IdManager::PacketType ctrl = packetIds.ToId( IdManager::ControlString);
    BOOST_CHECK_EQUAL(0u, packetIds.ToId( IdManager::InvalidString ));
    BOOST_CHECK_EQUAL(1u, ctrl);
    BOOST_CHECK_EQUAL(ctrl+1, packetIds.ToId( "Type1" ));
    BOOST_CHECK_EQUAL(ctrl+2, packetIds.ToId( "Type2" ));
    BOOST_CHECK_EQUAL(ctrl+3, packetIds.ToId( "Type3" ));
    BOOST_CHECK_EQUAL(IdManager::InvalidString, packetIds.ToString(0));
    BOOST_CHECK_EQUAL(IdManager::ControlString, packetIds.ToString(ctrl));
    BOOST_CHECK_EQUAL("Type1", packetIds.ToString(ctrl+1));
    BOOST_CHECK_EQUAL("Type2", packetIds.ToString(ctrl+2));
    BOOST_CHECK_EQUAL("Type3", packetIds.ToString(ctrl+3));
}

BOOST_AUTO_TEST_CASE(TestComPacket) {
    ComPacket pkt;
    BOOST_CHECK(pkt.GetType() == IdManager::InvalidPacket);
    BOOST_CHECK_EQUAL(0, pkt.GetDataSize());
    BOOST_CHECK_EQUAL(nullptr, pkt.GetDataPtr());

    constexpr int size = 6;
    VectorStream::CharType bytes[size] = "hello";
    ComPacket pkt2( IdManager::InvalidPacket, bytes, size);
    BOOST_CHECK_EQUAL(size, pkt2.GetDataSize());
    BOOST_CHECK_NE(nullptr, pkt2.GetDataPtr());

    // Test packet contains the byte data:
    for( int i=0; i<size; ++i )
    {
        BOOST_CHECK_EQUAL(bytes[i], pkt2.GetData()[i]);
    }

    // Create an Odometry packet with uninitialised data:
    constexpr int size2 = 17;
    ComPacket pkt3( IdManager::ControlPacket, size2);
    BOOST_CHECK_EQUAL(size2, pkt3.GetDataSize());
    BOOST_CHECK_NE(nullptr, pkt3.GetDataPtr());

    // Test that pkt3 gets moved (and made invalid):
    BOOST_CHECK_NE(size, size2);
    BOOST_CHECK_EQUAL(IdManager::ControlPacket, pkt3.GetType());
    pkt2 = std::move( pkt3);
    BOOST_CHECK_EQUAL(IdManager::InvalidPacket, pkt3.GetType());
    BOOST_CHECK_EQUAL(size2, pkt2.GetDataSize());

    // Test move constructor on pkt2 (which was pkt3):
    ComPacket pkt4( std::move(pkt2));
    BOOST_CHECK_EQUAL(size2, pkt4.GetDataSize());
    BOOST_CHECK_EQUAL(IdManager::ControlPacket, pkt4.GetType());
    BOOST_CHECK_EQUAL(0, pkt2.GetDataSize());
    BOOST_CHECK_EQUAL(IdManager::InvalidPacket, pkt2.GetType());
}

BOOST_AUTO_TEST_CASE(TestSimpleQueue) {
    SimpleQueue q;

    // Check new queue is empty:
    BOOST_CHECK_EQUAL(0, q.Size());
    BOOST_CHECK(q.Empty());

    // Add a packet onto queue:
    constexpr int pktSize = 7;
    auto sptr = std::make_shared<ComPacket>( IdManager::ControlPacket, pktSize);
    BOOST_CHECK_EQUAL(1, sptr.use_count());
    q.Emplace( sptr);
    BOOST_CHECK_EQUAL(1, q.Size());
    BOOST_CHECK_EQUAL(2, sptr.use_count());

    // Check SimpleQueue::Front() returns shared_ptr to same packet:
    auto sptr2 = q.Front();
    BOOST_CHECK_EQUAL(3, sptr.use_count());
    BOOST_CHECK(sptr == sptr2);

    // Check queue locking:
    SimpleQueue::LockedQueue lockedQueue = q.Lock();

    // Whilts queue is not empty, and we have a lock
    // check WaitNotEmpty() returns without blocking:
    bool blocked = true;
    auto asyncWaiter = std::thread([&]() {
        // Wait asynchronously so failing test does not block forever:
        lockedQueue.WaitNotEmpty();
        blocked = false;
    });
    try
    {
        asyncWaiter.join();
    } catch (const std::system_error& e)
    {
        std::clog << "Error: " << e.what() << std::endl;
    }
    usleep( 1000); // sleep so async func has time to finish
    BOOST_CHECK(!blocked);

    // Check popping makes it empty again:
    q.Pop();
    BOOST_CHECK_EQUAL(0, q.Size());
    BOOST_CHECK(q.Empty());
    BOOST_CHECK_EQUAL(2, sptr.use_count());
    sptr2.reset();
}

BOOST_AUTO_TEST_CASE(TestPacketMuxerExitsCleanly) {
    AlwaysFailSocket mockSocket;
    PacketMuxer muxer(mockSocket, {});
    while (muxer.Ok()) {}
}

BOOST_AUTO_TEST_CASE(TestPacketMuxer) {
    constexpr int testPayloadSize = 11;
    MuxerTestSocket socket( testPayloadSize);
    PacketMuxer muxer( socket, {"MockPacket"});

    // Transport should be ok here:
    BOOST_CHECK(muxer.Ok());

    usleep( 100000);

    // Emplace payload:
    VectorStream::CharType payload[testPayloadSize];
    muxer.EmplacePacket( "MockPacket", payload, testPayloadSize);

    // Post identical payload:
    muxer.EmplacePacket( "MockPacket", payload, testPayloadSize);

    // Give the muxer time to send:
    usleep( 100000);

    // Did it send all posted packets before exiting?
    BOOST_CHECK_EQUAL(muxer.GetNumPosted(), muxer.GetNumSent());
}

BOOST_AUTO_TEST_CASE(TestDemuxerExitsCleanly) {
    AlwaysFailSocket socket;
    PacketDemuxer demuxer(socket, {});
    while (demuxer.Ok()) {}
}

BOOST_AUTO_TEST_CASE(TestPacketDemuxer) {
    AlwaysFailSocket socket;
    PacketDemuxer demuxer(socket, {});
}

const int MSG_SIZE = 8;
const char MSG[ MSG_SIZE ] = "1234abc";
const char UDP_MSG[] = "Udp connection-less Datagram!";

/*
    Server waits for client to connect then reads the test message from the client.
*/
void* RunTcpServerThread( void* arg )
{
    TcpSocket* server = (TcpSocket*)arg;
    auto connection = server->Accept();

    BOOST_CHECK(connection != nullptr);
    if ( connection )
    {
        char msg[256] = "";
        
        BOOST_CHECK(connection->IsValid());
        int bytes = connection->Read( msg, MSG_SIZE);
        BOOST_CHECK_EQUAL(bytes, MSG_SIZE);
        BOOST_CHECK_EQUAL(msg, MSG);

        connection->Shutdown();
    }

	return 0;
}

BOOST_AUTO_TEST_CASE(TestTcp) {
    const int TEST_PORT = 2000;
    TcpSocket server;
    BOOST_CHECK(server.IsValid());
    server.Bind( TEST_PORT);
    server.Listen( 0);

    // Create a server thread for testing:
    pthread_t serverThread;
    pthread_create( &serverThread, 0 , RunTcpServerThread, (void*)&server);

    // client connects and sends a test message
    TcpSocket client;
    BOOST_REQUIRE(client.Connect( "localhost", TEST_PORT ));

    client.Write( MSG, MSG_SIZE);
    client.Shutdown();

    pthread_join( serverThread, 0);
}

/*
    Server waits for messages from client and checks they are correct.
*/
void* RunUdpServerThread( void* arg )
{
    UdpSocket* server = (UdpSocket*)arg;
    
    char msg[256] = "";

    // First read the connection-less udp message:
    const int udpMsgSize = strlen(UDP_MSG) + 1;
    int bytes = server->Read( msg, udpMsgSize);
    BOOST_CHECK_EQUAL(bytes, udpMsgSize);
    BOOST_CHECK_EQUAL(msg, UDP_MSG);

    // Then read the connected message:
    bytes = server->Read( msg, MSG_SIZE);
    BOOST_CHECK_EQUAL(bytes, MSG_SIZE);
    BOOST_CHECK_EQUAL(msg, MSG);

	return 0;
}

BOOST_AUTO_TEST_CASE(TestUdp) {
    const int TEST_PORT = 3000;
    UdpSocket server;
    BOOST_CHECK(server.IsValid());
    server.Bind( TEST_PORT);

    // Create a server thread for testing:
    pthread_t serverThread;
    pthread_create( &serverThread, 0 , RunUdpServerThread, (void*)&server);

    // Test connection-less datagram:
    Ipv4Address localhost( "127.0.0.1", TEST_PORT);
    BOOST_REQUIRE(localhost.IsValid());
    UdpSocket client;
    int bytesSent = client.SendTo( localhost, UDP_MSG, strlen(UDP_MSG)+1);
    BOOST_REQUIRE_EQUAL(strlen(UDP_MSG)+1, bytesSent);

    // Test datagram to a connection:
    client.Connect( "127.0.0.1", TEST_PORT);
    client.Write( MSG, MSG_SIZE);

    pthread_join( serverThread, 0);

    client.Shutdown();
}

BOOST_AUTO_TEST_CASE(TestIpv4Address) {
    const int TEST_PORT = 3000;
    Ipv4Address localhost( "localhost", TEST_PORT);
    BOOST_REQUIRE(localhost.IsValid());

    std::string hostName;
    localhost.GetHostName( hostName);
    BOOST_CHECK_EQUAL("localhost", hostName.c_str());
    std::string hostIP;
    localhost.GetHostAddress( hostIP);
    BOOST_CHECK_EQUAL("127.0.0.1", hostIP.c_str());

    BOOST_CHECK_EQUAL(TEST_PORT, localhost.GetPort());

    Ipv4Address nonsense( "@nonsense.ww.arg.?", 120);
    BOOST_REQUIRE(!nonsense.IsValid());

    Ipv4Address uninitialised;
    BOOST_REQUIRE(!uninitialised.IsValid());

    Ipv4Address copy( localhost);
    BOOST_REQUIRE(localhost.IsValid());
}
