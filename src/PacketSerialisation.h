#ifndef PACKETSERIALISATION_H
#define PACKETSERIALISATION_H

/**
    'Cereal' serialisation support for the packet-comms system.

    Any type that has Cereal compatible serialisation functions
    can be serialised direct to a muxer, and directly from a
    shared ComPacket.
*/

#include "Serialisation.h"

template <typename... Args>
void Serialise(PacketMuxer& muxer, const std::string& id, const Args&... types) {
  VectorOutputStream stream;
  Serialise(stream, std::forward<const Args&>(types)...);
  muxer.EmplacePacket(id, std::move(stream.Get()));
}

template <typename... Args>
void Deserialise(const ComPacket::ConstSharedPacket& packet, Args&... types) {
  VectorInputStream stream(packet->GetData());
  Deserialise(stream, std::forward<Args&>(types)...);
}

#endif  // PACKETSERIALISATION_H
