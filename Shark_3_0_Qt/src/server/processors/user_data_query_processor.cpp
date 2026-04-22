#include "user_data_query_processor.h"

#include "exceptions_cpp/network_exception.h"
#include "exceptions_cpp/sql_exception.h"
#include "exceptions_cpp/login_exception.h"
#include "server_session.h"

#include <iostream>
#include <libpq-fe.h>
#include <memory>

UserDataQueryProcessor::UserDataQueryProcessor(
    UserSqlReader &user_sql_reader)
    : user_sql_reader_(user_sql_reader) {}

bool UserDataQueryProcessor::Process(ServerSession &session,
                                     SessionTransport &transport,
                                     PacketListDTO &packet_list,
                                     const RequestType &request_type,
                                     int connection) {
  if (request_type != RequestType::RqFrClientGetUsersData) {
    return false;
  }

  std::vector<std::string> logins;
  PacketListDTO response_packet_list;

  try {
    for (const auto &packet : packet_list.packets) {
      if (packet.requestType != RequestType::RqFrClientGetUsersData) {
        throw exc::HeaderWrongTypeException();
      }

      const auto &login_packet =
          static_cast<const StructDTOClass<UserLoginDTO> &>(
              *packet.structDTOPtr)
              .getStructDTOClass();

      if (!user_sql_reader_.CheckUserLoginSQL(login_packet.login,
                                              session.getPGConnection())) {
        throw exc::UserNotFoundException();
      }

      logins.push_back(login_packet.login);
    }

    if (logins.empty()) {
      throw exc::UserNotFoundException();
    }

    auto users = session.FillForSendSeveralUsersDTOFromSrvSQL(logins);
    if (!users.has_value()) {
      throw exc::UserNotFoundException();
    }

    for (const auto &user_dto : users.value()) {
      PacketDTO packet;
      packet.requestType = RequestType::RqFrClientGetUsersData;
      packet.structDTOClassType = StructDTOClassType::userDTO;
      packet.reqDirection = RequestDirection::ClientToSrv;
      packet.structDTOPtr =
          std::make_shared<StructDTOClass<UserDTO>>(user_dto);
      response_packet_list.packets.push_back(packet);
    }

    session.sendPacketListDTO(transport, response_packet_list, connection);
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }

  return true;
}
