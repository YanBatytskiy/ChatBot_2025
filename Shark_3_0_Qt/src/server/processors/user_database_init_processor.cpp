#include "user_database_init_processor.h"

#include "exceptions_cpp/network_exception.h"
#include "exceptions_cpp/sql_exception.h"
#include "server_session.h"

#include <iostream>
#include <memory>

UserDatabaseInitProcessor::UserDatabaseInitProcessor(
    DatabaseSqlManager &database_sql_manager)
    : database_sql_manager_(database_sql_manager) {}

bool UserDatabaseInitProcessor::Process(ServerSession &session,
                                        SessionTransport &transport,
                                        PacketListDTO &packet_list,
                                        const RequestType &request_type,
                                        int connection) {
  (void)packet_list;
  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  try {
    bool init_result =
        database_sql_manager_.InitDatabaseSQL(session.getPGConnection());

    PacketDTO response_packet;
    response_packet.requestType = request_type;
    response_packet.structDTOClassType = StructDTOClassType::responceDTO;
    response_packet.reqDirection = RequestDirection::ClientToSrv;

    ResponceDTO responce_dto;
    responce_dto.anyNumber = 0;
    responce_dto.anyString = "";
    responce_dto.reqResult = init_result;

    response_packet.structDTOPtr =
        std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);

    response_packet_list.packets.push_back(response_packet);
    session.sendPacketListDTO(transport, response_packet_list, connection);
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::sqlException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }

  return true;
}
