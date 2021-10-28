/*
    Copyright (C) Mark Pupilli 2013, All rights reserved
*/
#ifndef __MOCK_SOCKETS_H__
#define __MOCK_SOCKETS_H__

#include "../src/network/AbstractSocket.h"

#include <arpa/inet.h>

class MuxerTestSocket : public AbstractSocket {
 public:
  enum State {
    Type = 0,
    Size,
    Payload
  };

  MuxerTestSocket(int testPayloadSize)
      : m_testPayloadSize(testPayloadSize), m_totalBytes(0), m_expected(Type) {}

  void setBlocking(bool) {}

  void checkType(const char* data, std::size_t size) {
    BOOST_CHECK_EQUAL(sizeof(uint32_t), size);
    uint32_t type = ntohl(*reinterpret_cast<const uint32_t*>(data));
    m_type        = static_cast<IdManager::PacketType>(type);
  }

  void checkSize(const char* data, std::size_t size) {
    BOOST_CHECK_EQUAL(sizeof(uint32_t), size);
    uint32_t payloadSize = ntohl(*reinterpret_cast<const uint32_t*>(data));
    if (m_type > 1) {
      BOOST_CHECK_EQUAL(m_testPayloadSize, payloadSize);
    }
  }

  /// @todo - this test is very tied to implementation (i.e. knows how the writes are broken down). Not good.
  void checkPayload(const char*, std::size_t size) {
    if (m_type > 1) {
      BOOST_CHECK_EQUAL(m_testPayloadSize, size);
    }
  }

  int write(const char* data, std::size_t size) {
    switch (m_expected) {
      case Type:
        checkType(data, size);
        m_expected = Size;
        break;
      case Size:
        checkSize(data, size);
        m_expected = Payload;
        break;
      case Payload:
        checkPayload(data, size);
        m_expected = Type;
        break;
      default:
        BOOST_FAIL("MuxerTestSocket write fail");
        break;
    }

    m_totalBytes += size;

    return size;  // pretend we sent it all
  }

  // Check read functions are unused by muxer:
  int read(char*, std::size_t) {
    BOOST_FAIL("MuxerTestSocket read fail");
    return -1;
  }

  bool readyForReading(int) const {
    BOOST_FAIL("MuxerTestSocket ready fail");
    return false;
  }

  int m_testPayloadSize;
  int m_totalBytes;
  State m_expected;
  IdManager::PacketType m_type;
};

class DemuxerTestSocket : public AbstractSocket {
 public:
  DemuxerTestSocket(){};
  virtual ~DemuxerTestSocket(){};

  virtual void setBlocking(bool) {}
  virtual int write(const char*, std::size_t size) { return size; }
  virtual int read(char*, std::size_t size) { return size; }
  virtual bool readyForReading(int) const { return true; };
};

/**
    This mock socket always reports being ready to read, and always
    reports an io error(returns -1) if read() or write() are called.
*/
class AlwaysFailSocket : public AbstractSocket {
 public:
  AlwaysFailSocket(){};
  virtual ~AlwaysFailSocket(){};

  virtual void setBlocking(bool) {}
  virtual int write(const char*, std::size_t) { return -1; }
  virtual int read(char*, std::size_t) { return -1; }
  virtual bool readyForReading(int) const { return true; };
};

#endif /* __MOCK_SOCKETS_H__ */
