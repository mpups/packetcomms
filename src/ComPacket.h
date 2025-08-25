/*
    Copyright (C) Mark Pupilli 2012, All rights reserved
*/
#ifndef __COM_PACKET_H__
#define __COM_PACKET_H__

#include <cstdint>
#include <memory>
#include <queue>
#include <type_traits>
#include <vector>

#include "IdManager.h"
#include "VectorStream.h"

class ComPacket {
 public:
  typedef std::shared_ptr<ComPacket> SharedPacket;
  typedef std::shared_ptr<const ComPacket> ConstSharedPacket;
  typedef std::queue<SharedPacket> PacketContainer;

  ComPacket(const ComPacket&) = delete;
  ComPacket& operator=(const ComPacket&) = delete;

  /// Default constructed invalid packet:
  ComPacket()
      : m_type(IdManager::InvalidPacket) {}

  /// Construct a com packet from raw buffer of stream data:
  ComPacket(IdManager::PacketType type, const VectorStream::CharType* buffer, int size)
      : m_type(type), m_data(buffer, buffer + size) {}

  ComPacket(IdManager::PacketType type, VectorStream::Buffer&& buffer)
      : m_type(type) {
    std::swap(m_data, buffer);
  }

  /// Construct ComPacket with preallocated data size (but no valid data).
  ComPacket(IdManager::PacketType type, int size)
      : m_type(type), m_data(size) {}

  virtual ~ComPacket() {}

  /// @param p The ComPacket to be moved - it will become of invalid type, with an empty data vector.
  ComPacket(ComPacket&& p)
      : m_type(IdManager::InvalidPacket) {
    std::swap(p.m_type, m_type);
    std::swap(p.m_data, m_data);
  };  /// @todo use delgating constructor to create  invalid packet when upgraded to gcc-4.7+

  ComPacket& operator=(ComPacket&& p) {
    std::swap(p.m_type, m_type);
    std::swap(p.m_data, m_data);
    return *this;
  };

  IdManager::PacketType getType() const { return m_type; };
  const VectorStream::CharType* getDataPtr() const { return m_data.data(); };
  VectorStream::CharType* getDataPtr() { return m_data.data(); };
  std::vector<VectorStream::CharType>::size_type getDataSize() const noexcept { return m_data.size(); };
  const std::vector<VectorStream::CharType>& getData() const { return m_data; };
  std::vector<VectorStream::CharType>& getData() { return m_data; };

 protected:
 private:
  IdManager::PacketType m_type;
  VectorStream::Buffer m_data;
};

#endif /* __COM_PACKET_H__ */
