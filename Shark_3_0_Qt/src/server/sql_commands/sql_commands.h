#pragma once

#include <libpq-fe.h>

#include <map>
#include <string>

class SQLRequests {
 public:
  SQLRequests() = default;
  ~SQLRequests() = default;

  PGresult *execTransactionToSQL(PGconn *conn,
                                 std::multimap<int, std::string> &sql_requests,
                                 std::multimap<int, std::string> &sql_description);

  PGresult *execSQL(PGconn *conn, const std::string &sql);
};
