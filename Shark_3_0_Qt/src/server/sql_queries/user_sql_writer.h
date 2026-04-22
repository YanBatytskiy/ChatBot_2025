#pragma once

#include "dto/dto_struct.h"
#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

class UserSqlWriter {
 public:
  explicit UserSqlWriter(SQLRequests &executor);

  bool CreateUserSQL(const UserDTO &user_dto, PGconn *conn);
  bool ChangeUserDataSQL(const UserDTO &user_dto, PGconn *conn);

  bool BlockUserSQL(const UserDTO &user_dto, PGconn *conn);
  bool UnblockUserSQL(const UserDTO &user_dto, PGconn *conn);
  bool BanUserSQL(const UserDTO &user_dto, PGconn *conn);
  bool UnbanUserSQL(const UserDTO &user_dto, PGconn *conn);

 private:
  SQLRequests &executor_;
};
