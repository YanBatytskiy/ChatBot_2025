#pragma once

#include "dto/dto_struct.h"

#include <cstdint>
#include <vector>

class PacketParser {
public:
  struct ParsedPacket {
    RequestType request_type;
    PacketListDTO payload;
  };

  ParsedPacket ParseIncoming(const std::vector<std::uint8_t> &buffer) const;
};
