#pragma once

#include "sql_commands/sql_commands.h"

#include <libpq-fe.h>

class DatabaseSqlManager {
 public:
  explicit DatabaseSqlManager(SQLRequests &executor);

  bool InitDatabaseSQL(PGconn *conn);
  bool ClearDatabaseSQL(PGconn *conn);

 private:
  SQLRequests &executor_;
};
