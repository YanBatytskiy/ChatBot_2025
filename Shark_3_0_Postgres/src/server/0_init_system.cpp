#include "0_init_system.h"
#include "init_sql_requests.h"
#include "server/sql_server.h"
#include "server_session.h"
#include <cstdint>
#include <ctime>
#include <iostream>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include <string>

bool checkBaseStructureSrv([[maybe_unused]] PGconn *conn) {
  return true;
}

bool initDatabaseOnServer(PGconn *conn) {

  std::multimap<int, std::string> sqlRequests;
  std::multimap<int, std::string> temp_sql;
  std::multimap<int, std::string> sqlDescription;

  sqlRequests.clear();
  PGresult *result;

  // Check whether the database is empty
  //   bool emptyResult = checkEmptyBaseSQL(conn);
  bool emptyResult = true;

  // If the database is empty, populate it
  if (emptyResult) {

    // Clear
    clearBaseSQL(conn);

    // Populate
    sqlRequests = createInitTablesSQL();
    sqlDescription.insert({1, "chats"});
    sqlDescription.insert({2, "users"});
    sqlDescription.insert({3, "messages"});
    sqlDescription.insert({4, "message_status"});
    sqlDescription.insert({5, "users_passhash"});
    sqlDescription.insert({6, "participants"});
    sqlDescription.insert({7, "insert users"});
    sqlDescription.insert({8, "Chat 1"});
    sqlDescription.insert({9, "Chat 2"});

    sqlRequests.insert({8, createChatFirstSQL().begin()->second});
    result = execTransactionToSQL(conn, sqlRequests, sqlDescription);

    sqlRequests.clear();
    sqlRequests.insert({9, createChatSecondSQL().begin()->second});
    result = execTransactionToSQL(conn, sqlRequests, sqlDescription);

    bool ok = (result != nullptr);
    if (result)
      PQclear(result);
    return ok;
    // If the database is not empty, verify its integrity
  } else {
    return true;
    // If the database is corrupted, offer the user to recreate it or exit
    clearBaseSQL(conn);
  }
  return true;
}

/**
 * @brief make timeStamp for initialization
 *
 */
std::int64_t makeTimeStamp(int year, int month, int day, int hour, int minute, int second) {
  tm tm = {};
  tm.tm_year = year - 1900; // years since 1900
  tm.tm_mon = month - 1;    // months starting at 0
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  time_t timeT = mktime(&tm);                // local time
  return static_cast<int64_t>(timeT) * 1000; // milliseconds
}

// const std::string initUserPassword[] = {"User01", "User02", "User03",
// "User04", "User05",
//                                         "User06", "User07", "User08"};
[[maybe_unused]] const std::string initUserPassword[] = {"1", "1", "1", "1", "1", "1", "1", "1"};

// const std::string initUserLogin[] = {"alex1980", "elena1980", "serg1980",
// "vit1980",
//                                      "mar1980",  "fed1980",   "vera1980",
//                                      "yak1980"};

[[maybe_unused]] const std::string initUserLogin[] = {"a", "e", "s", "v", "m", "f", "ver", "y"};

bool systemInitForTest([[maybe_unused]] ServerSession &serverSession, PGconn *conn) {

  if (!initDatabaseOnServer(conn)) {
    std::cout << "Failed to initialize the database on the server. \n";
    return false;
  };

  return true;
}
