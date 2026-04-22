#include "user_account_update_processor.h"

#include "exceptions_cpp/network_exception.h"
#include "exceptions_cpp/sql_exception.h"
#include "server_session.h"

#include <cstdlib>
#include <iostream>
#include <libpq-fe.h>
#include <memory>

UserAccountUpdateProcessor::UserAccountUpdateProcessor(
    UserSqlWriter &user_sql_writer)
    : user_sql_writer_(user_sql_writer) {}

bool UserAccountUpdateProcessor::Process(ServerSession &session,
                                         SessionTransport &transport,
                                         PacketListDTO &packet_list,
                                         const RequestType &request_type,
                                         int connection) {
  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  try {
    switch (request_type) {
      case RequestType::RqFrClientChangeUserData: {
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

        if (ChangeUserData(session, user_packet)) {
          responce_dto.reqResult = true;
          responce_dto.anyString = user_packet.login;
        } else {
          responce_dto.reqResult = false;
        }

        response_packet.structDTOPtr =
            std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
        response_packet_list.packets.push_back(response_packet);
        break;
      }
      case RequestType::RqFrClientChangeUserPassword:
        // TODO: реализовать смену пароля клиента.
        break;
      default:
        break;
    }

    if (!response_packet_list.packets.empty()) {
      session.sendPacketListDTO(transport, response_packet_list, connection);
    }
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::EmptyPacketException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }

  return true;
}

bool UserAccountUpdateProcessor::ChangeUserData(ServerSession &session,
                                                const UserDTO &user_dto) {
  return user_sql_writer_.ChangeUserDataSQL(user_dto,
                                            session.getPGConnection());
}
