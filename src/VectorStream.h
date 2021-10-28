#ifndef VECTORSTREAM_H
#define VECTORSTREAM_H

#include <streambuf>
#include <vector>

/**
    Primarily the vector IO streams are intended for
    use with the 'Cereal' serilisation library to get
    an efficient in memory representation of serialised
    binary streams and to be able to deserialise from
    them. The data can then be transmitted/received
    with no coupling to the Cereal library.
*/
namespace VectorStream {
typedef std::vector<std::streambuf::char_type> Buffer;
typedef std::streambuf::char_type CharType;
}  // namespace VectorStream

/**
    An output stream buffer that writes directly
    to an internal vector.
*/
class VectorOutputStream : public std::streambuf {
 public:
  VectorOutputStream() {}
  VectorOutputStream(const size_t reserve)
      : m_v(reserve) {}

  VectorStream::Buffer& get() { return m_v; }
  const VectorStream::Buffer& get() const { return m_v; }

  void clear() { m_v.clear(); }

  VectorOutputStream(VectorOutputStream&& toMove) {
    m_v = std::move(toMove.m_v);
  }

 private:
  virtual std::streambuf::int_type overflow(std::streambuf::int_type ch) {
    if (ch == std::streambuf::traits_type::eof()) {
      return std::streambuf::traits_type::eof();
    }
    m_v.push_back(ch);
    return ch;
  }

  std::streamsize xsputn(const std::streambuf::char_type* s, std::streamsize n) {
    // Make sure vector has enough capacity then insert all the bytes:
    m_v.reserve(m_v.size() + n);
    m_v.insert(m_v.end(), s, s + n);
    return n;
  }

  VectorStream::Buffer m_v;
};

/**
    An input stream buffer that reads directly
    from a std::vector that is passed into the
    constructor.
*/
class VectorInputStream : public std::streambuf {
 public:
  /**
        @param v Vector to input from - this vector must not
        be modified for the lifetime of the VectorInputStream object.
    */
  explicit VectorInputStream(const VectorStream::Buffer& v) {
    const std::streambuf::char_type* begin = v.data();
    setg(const_cast<char_type*>(begin),
         const_cast<char_type*>(begin),
         const_cast<char_type*>(begin + v.size()));
  }

 private:
  virtual std::streambuf::int_type overflow() {
    return traits_type::eof();
  }
};

#endif  // VECTORSTREAM_H
