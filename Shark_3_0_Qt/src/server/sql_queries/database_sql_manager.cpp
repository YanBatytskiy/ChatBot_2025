#include "server/sql_queries/database_sql_manager.h"

#include "exceptions_cpp/sql_exception.h"
#include "server/sql_queries/database_init_sql_builder.h"

#include <iostream>
#include <map>
#include <string>

DatabaseSqlManager::DatabaseSqlManager(SQLRequests &executor)
    : executor_(executor) {}

bool DatabaseSqlManager::InitDatabaseSQL(PGconn *conn) {
  std::multimap<int, std::string> sql_requests;
  std::multimap<int, std::string> sql_description;

  PGresult *result = nullptr;

  if (!ClearDatabaseSQL(conn)) {
    return false;
  }

  DatabaseInitSqlBuilder builder;
  sql_requests = builder.BuildSchemaSql();
  sql_description.insert({1, "chats"});
  sql_description.insert({2, "users"});
  sql_description.insert({3, "messages"});
  sql_description.insert({4, "message_status"});
  sql_description.insert({5, "users_passhash"});
  sql_description.insert({6, "participants"});
  sql_description.insert({7, "insert users"});
  sql_description.insert({8, "users_buns"});
  sql_description.insert({9, "Chat 1"});
  sql_description.insert({10, "Chat 2"});

  auto chat_first_sql = builder.BuildChatFirstSql();
  if (!chat_first_sql.empty()) {
    sql_requests.insert({9, chat_first_sql.begin()->second});
  }
  result = executor_.execTransactionToSQL(conn, sql_requests, sql_description);

  if (result) {
    PQclear(result);
    result = nullptr;
  } else {
    return false;
  }

  auto chat_second_sql = builder.BuildChatSecondSql();
  sql_requests.clear();
  sql_description.clear();
  if (!chat_second_sql.empty()) {
    sql_requests.insert({1, chat_second_sql.begin()->second});
    sql_description.insert({1, "Chat 2"});
  }
  result = executor_.execTransactionToSQL(conn, sql_requests, sql_description);

  bool ok = (result != nullptr);
  if (result) {
    PQclear(result);
  }
  return ok;
}

bool DatabaseSqlManager::ClearDatabaseSQL(PGconn *conn) {
  PGresult *result = nullptr;

  try {
    std::string sql = R"(DROP OWNED BY CURRENT_USER CASCADE;
	  )";

    result = executor_.execSQL(conn, sql);
    if (!result) {
      return false;
    }
    PQclear(result);

    sql = R"(CREATE SCHEMA public AUTHORIZATION CURRENT_USER;
SET search_path TO public;)";
    result = executor_.execSQL(conn, sql);

    const bool ok = (result != nullptr);
    if (result) {
      PQclear(result);
    }
    return ok;
  } catch (const exc::SQLExecException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result) {
      PQclear(result);
    }
    return false;
  }
}
