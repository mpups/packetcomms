#ifndef MUXERCONTROLMESSAGES_H
#define MUXERCONTROLMESSAGES_H

#include <cstdint>

enum class ControlMessage : std::uint8_t {
  HeartBeat = 0,
  Hello     = 254,
  GoodBye   = 255
};

#endif  // MUXERCONTROLMESSAGES_H
