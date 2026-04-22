#pragma once

#include "dto/dto_struct.h"
#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

#include <optional>
#include <string>
#include <vector>

class UserSqlReader {
 public:
  explicit UserSqlReader(SQLRequests &executor);

  bool CheckUserLoginSQL(const std::string &login, PGconn *conn);
  bool CheckUserPasswordSQL(const UserLoginPasswordDTO &credentials,
                            PGconn *conn);

  std::optional<UserDTO> GetUserWithPasshashSQL(const std::string &login,
                                                PGconn *conn);
  std::optional<std::vector<UserDTO>> GetUsersByTextPartSQL(
      const UserLoginPasswordDTO &packet, PGconn *conn);
  std::optional<std::vector<UserDTO>> GetSeveralUsersSQL(
      const std::vector<std::string> &logins, PGconn *conn);

 private:
  SQLRequests &executor_;
};
