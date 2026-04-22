#pragma once

#include "dto/dto_struct.h"
#include "session_transport/session_transport.h"
#include "sql_queries/database_sql_manager.h"

class ServerSession;
class UserDatabaseInitProcessor {
 public:
  explicit UserDatabaseInitProcessor(DatabaseSqlManager &database_sql_manager);

  bool Process(ServerSession &session, SessionTransport &transport,
               PacketListDTO &packet_list, const RequestType &request_type,
               int connection);

 private:
  DatabaseSqlManager &database_sql_manager_;
};
