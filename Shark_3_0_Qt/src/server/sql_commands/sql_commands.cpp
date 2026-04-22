#include "sql_commands.h"

#include "exceptions_cpp/sql_exception.h"

#include <iostream>

PGresult *SQLRequests::execTransactionToSQL(
    PGconn *conn, std::multimap<int, std::string> &sql_requests,
    std::multimap<int, std::string> &sql_description) {
  PGresult *result = PQexec(conn, "BEGIN");
  std::string error_table;

  try {
    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK) {
      if (result) {
        PQclear(result);
      }
      throw exc::SQLExecException("Открытие транзакции.");
    }
    PQclear(result);
    int index = 1;

    for (const auto &request : sql_requests) {
      result = execSQL(conn, request.second);

      if (!result) {
        auto it = sql_description.find(index);
        error_table = (it != sql_description.end()) ? it->second : std::string();

        throw exc::SQLCreateTableException("p" + std::to_string(index) + ", " +
                                           request.second);
      }

      auto status = PQresultStatus(result);
      if (status != PGRES_COMMAND_OK && status != PGRES_TUPLES_OK) {
        PQclear(result);

        auto it = sql_description.find(index);
        error_table = (it != sql_description.end()) ? it->second : std::string();

        throw exc::SQLCreateTableException("p" + std::to_string(index) + ", " +
                                           request.second);
      }
      PQclear(result);
      ++index;
    }
  } catch (exc::SQLCreateTableException &ex) {
    std::cerr << ex.what() << "\n"
              << " Проверьте SQL запросы: " << error_table << "\n";
    result = PQexec(conn, "ROLLBACK");
    if (result) {
      PQclear(result);
    }
    return nullptr;
  } catch (exc::SQLExecException &ex) {
    std::cerr << ex.what() << "\n"
              << " Проверьте доступ к базе." << "\n";
    result = PQexec(conn, "ROLLBACK");
    if (result) {
      PQclear(result);
    }
    return nullptr;
  }

  result = PQexec(conn, "COMMIT");
  if (!result) {
    return nullptr;
  }
  const bool ok = (PQresultStatus(result) == PGRES_COMMAND_OK);

  return ok ? result : nullptr;
}

PGresult *SQLRequests::execSQL(PGconn *conn, const std::string &sql) {
  PGresult *result = PQexec(conn, sql.c_str());

  try {
    if (!result) {
      std::string error = PQerrorMessage(conn);
      throw exc::SQLExecException(error);
    }
    if (PQresultStatus(result) != PGRES_COMMAND_OK &&
        PQresultStatus(result) != PGRES_TUPLES_OK) {
      std::string error = PQresultErrorMessage(result);
      PQclear(result);
      throw exc::SQLExecException(error);
    }
  } catch (exc::SQLExecException &ex) {
    std::cerr << ex.what() << "\n";
    return nullptr;
  }

  return result;
}
