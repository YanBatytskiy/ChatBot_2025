#pragma once

#include "dto/dto_struct.h"
#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

#include <optional>
#include <set>
#include <string>
#include <vector>

class ChatSqlReader {
 public:
  explicit ChatSqlReader(SQLRequests &executor);

  std::vector<std::string> GetChatIdsForUserSQL(const std::string &login,
                                                PGconn *conn);
  std::optional<std::vector<ParticipantsDTO>> GetChatParticipantsSQL(
      const std::string &chat_id, PGconn *conn);
  std::optional<std::multiset<std::pair<std::string, std::size_t>>>
  GetChatMessagesDeletedStatusSQL(const std::string &chat_id, PGconn *conn);
  std::optional<MessageChatDTO> GetChatMessagesSQL(const std::string &chat_id,
                                                   PGconn *conn);

 private:
  SQLRequests &executor_;
};
