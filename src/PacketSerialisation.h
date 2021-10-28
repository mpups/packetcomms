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
void serialise(PacketMuxer& muxer, const std::string& id, const Args&... types) {
  VectorOutputStream stream;
  serialise(stream, std::forward<const Args&>(types)...);
  muxer.emplacePacket(id, std::move(stream.get()));
}

template <typename... Args>
void deserialise(const ComPacket::ConstSharedPacket& packet, Args&... types) {
  VectorInputStream stream(packet->getData());
  deserialise(stream, std::forward<Args&>(types)...);
}

#endif  // PACKETSERIALISATION_H
