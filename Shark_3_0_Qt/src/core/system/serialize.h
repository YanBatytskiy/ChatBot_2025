#pragma once

#include "dto/dto_struct.h"
#include <vector>
#include <cstdint>

std::uint64_t toBigEndian64(std::uint64_t value);

std::uint64_t fromBigEndian64(std::uint64_t value);

std::vector<std::uint8_t> serializeToBinary(const PacketDTO &packetDTO);
std::vector<std::uint8_t>
serializePacketList(std::vector<PacketDTO>& packetList);

PacketDTO deSerializeFromBinary(const std::vector<std::uint8_t> &messageIncome);
std::vector<PacketDTO>
deSerializePacketList(const std::vector<uint8_t> &buffer);







