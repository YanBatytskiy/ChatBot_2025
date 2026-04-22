#pragma once

#include "dto/dto_struct.h"
#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

#include <cstddef>

class MessageSqlWriter {
 public:
  explicit MessageSqlWriter(SQLRequests &executor);

  bool SetLastReadMessageSQL(const MessageDTO &message_dto, PGconn *conn);
  std::size_t CreateMessageSQL(const MessageDTO &message_dto, PGconn *conn);

 private:
  SQLRequests &executor_;
};
