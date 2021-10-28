#ifndef IDMANAGER_H
#define IDMANAGER_H

#include <assert.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class IdManager {
 public:
  typedef std::uint32_t PacketType;
  static constexpr PacketType InvalidPacket = 0;
  static constexpr PacketType ControlPacket = 1;
  static const std::string InvalidString;
  static const std::string ControlString;

  IdManager(const std::vector<std::string>& list) {
    m_map[InvalidString] = InvalidPacket;
    m_reverse.push_back(InvalidString);
    m_map[ControlString] = ControlPacket;
    m_reverse.push_back(ControlString);

    std::size_t id = ControlPacket;
    for (const std::string& name : list) {
      assert(m_map.find(name) == m_map.end());
      id += 1;
      m_map[name] = id;
      m_reverse.push_back(name);
    }
  }

  virtual ~IdManager() {}

  PacketType toId(const std::string& name) const { return m_map.at(name); }
  const std::string& toString(const PacketType id) const { return m_reverse[id]; }

 private:
  std::map<std::string, PacketType> m_map;
  std::vector<std::string> m_reverse;
};

#endif  // IDMANAGER_H
