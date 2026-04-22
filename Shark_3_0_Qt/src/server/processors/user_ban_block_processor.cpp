#include "user_ban_block_processor.h"

#include "exceptions_cpp/network_exception.h"
#include "server_session.h"

#include <iostream>
#include <memory>

UserBanBlockProcessor::UserBanBlockProcessor(UserSqlWriter &user_sql_writer)
    : user_sql_writer_(user_sql_writer) {}

bool UserBanBlockProcessor::Process(ServerSession &session,
                                    SessionTransport &transport,
                                    PacketListDTO &packet_list,
                                    const RequestType &request_type,
                                    int connection) {
  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  try {
    if (packet_list.packets.empty()) {
      throw exc::EmptyPacketException();
    }

    PacketDTO response_packet;
    response_packet.requestType = request_type;
    response_packet.structDTOClassType = StructDTOClassType::responceDTO;
    response_packet.reqDirection = RequestDirection::ClientToSrv;

    const auto &user_packet =
        static_cast<const StructDTOClass<UserDTO> &>(
            *packet_list.packets[0].structDTOPtr)
            .getStructDTOClass();

    ResponceDTO responce_dto;
    responce_dto.anyNumber = 0;
    responce_dto.anyString = "";

    switch (request_type) {
      case RequestType::RqFrClientBlockUser:
        if (user_sql_writer_.BlockUserSQL(
                user_packet, session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = user_packet.login;
        } else {
          responce_dto.reqResult = false;
        }
        break;
      case RequestType::RqFrClientUnBlockUser:
        if (user_sql_writer_.UnblockUserSQL(
                user_packet, session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = user_packet.login;
        } else {
          responce_dto.reqResult = false;
        }
        break;
      case RequestType::RqFrClientBunUser:
        if (user_sql_writer_.BanUserSQL(user_packet,
                                        session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = user_packet.login;
        } else {
          responce_dto.reqResult = false;
        }
        break;
      case RequestType::RqFrClientUnBunUser:
        if (user_sql_writer_.UnbanUserSQL(
                user_packet, session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = user_packet.login;
        } else {
          responce_dto.reqResult = false;
        }
        break;
      default:
        break;
    }

    response_packet.structDTOPtr =
        std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
    response_packet_list.packets.push_back(response_packet);

    session.sendPacketListDTO(transport, response_packet_list, connection);
  } catch (const exc::EmptyPacketException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }

  return true;
}
