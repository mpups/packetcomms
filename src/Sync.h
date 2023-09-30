// Copyright (C) Mark Pupilli 2023, All rights reserved

#ifndef __PACKET_SYNC_H__
#define __PACKET_SYNC_H__

#include "PacketMuxer.h"
#include "PacketDemuxer.h"

#include "PacketSerialisation.h"

// Utility functions for building a client server synchronisation mechanism.
// The server initiaties sync and the client acknowledges. This relationship
// is for the purposes of describing the sync behaviour, the actual
// client/server releationship at the application level is not relevant here.

using namespace std::chrono_literals;

// Blocks until client acknowledges it is ready. The readyMsgStr must be defined and agreed by
// both side in the application level protocol. (I.e. by adding a string identifier to the
// packet ids like any other packetcomms msg).
void syncWithClient(PacketMuxer& tx, PacketDemuxer& rx, const std::string& readyMsgStr);

// Blocks until server ready message is received then sends an acknowledgement message back.
// The readyMsgStr must be defined and agreed by both side in the application level protocol.
// (I.e. by adding a string identifier to the packet ids like any other packetcomms msg).
void syncWithServer(PacketMuxer& tx, PacketDemuxer& rx, const std::string& readyMsgStr);

#endif // __PACKET_SYNC_H__
