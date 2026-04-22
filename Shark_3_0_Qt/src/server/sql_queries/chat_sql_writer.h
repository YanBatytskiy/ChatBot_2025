#pragma once

#include "dto/dto_struct.h"
#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

#include <string>
#include <vector>

class ChatSqlWriter {
 public:
  explicit ChatSqlWriter(SQLRequests &executor);

  std::vector<std::string> CreateChatWithInitialMessageSQL(
      const ChatDTO &chat_dto, const MessageChatDTO &message_chat_dto,
      PGconn *conn);

 private:
  SQLRequests &executor_;
};
