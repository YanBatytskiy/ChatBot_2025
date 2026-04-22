#include "packet_parser.h"

#include "exceptions_cpp/network_exception.h"
#include "system/serialize.h"

PacketParser::ParsedPacket PacketParser::ParseIncoming(
    const std::vector<std::uint8_t> &buffer) const {
  try {
    auto packets = deSerializePacketList(buffer);

    if (packets.empty()) {
      throw exc::EmptyPacketException();
    }

    const PacketDTO &header_packet = packets.front();

    if (header_packet.structDTOClassType !=
        StructDTOClassType::userLoginPasswordDTO) {
      throw exc::HeaderWrongTypeException();
    }

    const auto &login_packet = static_cast<const StructDTOClass<
        UserLoginPasswordDTO> &>(*header_packet.structDTOPtr)
                                   .getStructDTOClass();

    if (login_packet.passwordhash != "UserHeder") {
      throw exc::HeaderWrongTypeException();
    }

    if (login_packet.login.empty()) {
      throw exc::HeaderWrongDataException();
    }

    packets.erase(packets.begin());

    if (packets.empty()) {
      throw exc::EmptyPacketException();
    }

    PacketListDTO payload;
    for (const auto &packet : packets) {
      payload.packets.push_back(packet);
    }

    return {header_packet.requestType, payload};
  } catch (const exc::NetworkException &) {
    throw;
  } catch (const std::exception &ex) {
    throw exc::NetworkException(ex);
  } catch (...) {
    throw exc::NetworkException();
  }
}
