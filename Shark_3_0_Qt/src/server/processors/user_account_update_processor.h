#pragma once

#include "dto/dto_struct.h"
#include "sql_queries/user_sql_writer.h"
#include "session_transport/session_transport.h"

class ServerSession;
class UserAccountUpdateProcessor {
 public:
  explicit UserAccountUpdateProcessor(
      UserSqlWriter &user_sql_writer);

  bool Process(ServerSession &session, SessionTransport &transport,
               PacketListDTO &packet_list, const RequestType &request_type,
               int connection);

 private:
  bool ChangeUserData(ServerSession &session, const UserDTO &user_dto);

  UserSqlWriter &user_sql_writer_;
};
