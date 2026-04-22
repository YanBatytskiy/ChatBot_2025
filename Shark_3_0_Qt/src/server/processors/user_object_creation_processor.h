#pragma once

#include "dto/dto_struct.h"
#include "session_transport/session_transport.h"
#include "sql_queries/chat_sql_writer.h"
#include "sql_queries/message_sql_writer.h"
#include "sql_queries/user_sql_writer.h"

class ServerSession;
class UserObjectCreationProcessor {
 public:
  UserObjectCreationProcessor(UserSqlWriter &user_sql_writer,
                              ChatSqlWriter &chat_sql_writer,
                              MessageSqlWriter &message_sql_writer);

  bool Process(ServerSession &session, SessionTransport &transport,
               PacketListDTO &packet_list, const RequestType &request_type,
               int connection);

 private:
  UserSqlWriter &user_sql_writer_;
  ChatSqlWriter &chat_sql_writer_;
  MessageSqlWriter &message_sql_writer_;
};
