#pragma once

#include "dto/dto_struct.h"
#include "session_transport/session_transport.h"
#include "sql_queries/user_sql_reader.h"

#include <optional>
#include <string>
#include <vector>

class ServerSession;
class UserDataQueryProcessor {
 public:
  explicit UserDataQueryProcessor(UserSqlReader &user_sql_reader);

  bool Process(ServerSession &session, SessionTransport &transport,
               PacketListDTO &packet_list, const RequestType &request_type,
               int connection);

 private:
  UserSqlReader &user_sql_reader_;
};
