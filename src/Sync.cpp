// Copyright (C) Mark Pupilli 2023, All rights reserved

#include "Sync.h"

#include <string>
#include <atomic>

using namespace std::chrono_literals;

// Blocks until client acknowledges it is ready. The readyMsgStr must be defined and agreed by
// both side in the application level protocol. (I.e. by adding a string identifier to the
// packet ids like any other packetcomms msg).
void syncWithClient(PacketMuxer& tx, PacketDemuxer& rx, const std::string& readyMsgStr) {
  std::atomic<bool> clientReady(false);

  // Subscription that sets clientReady to true
  // when it receives the ready message:
  auto subs = rx.subscribe(readyMsgStr, [&](const ComPacket::ConstSharedPacket& packet) {
    bool discard;
    deserialise(packet, discard);
    clientReady = true;
  });

  // Keep sending ready messages until the client resposnds:
  while (!clientReady) {
    serialise(tx, readyMsgStr, true);
    std::this_thread::sleep_for(5ms);
  }
}

// Blocks until server ready message is received then sends an acknowledgement message back.
// The readyMsgStr must be defined and agreed by both side in the application level protocol.
// (I.e. by adding a string identifier to the packet ids like any other packetcomms msg).
void syncWithServer(PacketMuxer& tx, PacketDemuxer& rx, const std::string& readyMsgStr) {
  std::atomic<bool> serverReady(false);

  // Subscription that sets serverReady to true
  // when it receives the ready message:
  auto subs = rx.subscribe(readyMsgStr, [&](const ComPacket::ConstSharedPacket& packet) {
    bool discard;
    deserialise(packet, discard);
    serverReady = true;
  });

  // Sleep until we receive the server ready message:
  while (!serverReady) {
    std::this_thread::sleep_for(5ms);
  }

  // Acknowledge by telling the server we are ready:
  serialise(tx, readyMsgStr, true);
}

