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

bool checkBaseStructureSrv(PGconn *conn) { return true; }

bool initDatabaseOnServer(PGconn *conn) {

  std::multimap<int, std::string> sqlRequests;
  std::multimap<int, std::string> temp_sql;
  std::multimap<int, std::string> sqlDescription;

  sqlRequests.clear();
  PGresult *result;

  // проверяем пустая ли база
  //   bool emptyResult = checkEmptyBaseSQL(conn);
  bool emptyResult = true;

  // если база пустая, то мы ее заполняем
  if (emptyResult) {

    // очищаем
    clearBaseSQL(conn);

    // заполняем
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
    // если база не пустая, то мы проверяем ее целостность
  } else {
    return true;
    // если база битая, предлагаем пользователю либо пересоздать ее либо выйти
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
  tm.tm_year = year - 1900; // годы от 1900
  tm.tm_mon = month - 1;    // месяцы от 0
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  time_t timeT = mktime(&tm);                // локальное время
  return static_cast<int64_t>(timeT) * 1000; // миллисекунды
}

// const std::string initUserPassword[] = {"User01", "User02", "User03",
// "User04", "User05",
//                                         "User06", "User07", "User08"};
const std::string initUserPassword[] = {"1", "1", "1", "1", "1", "1", "1", "1"};

// const std::string initUserLogin[] = {"alex1980", "elena1980", "serg1980",
// "vit1980",
//                                      "mar1980",  "fed1980",   "vera1980",
//                                      "yak1980"};

const std::string initUserLogin[] = {"a", "e", "s", "v", "m", "f", "ver", "y"};

bool systemInitForTest(ServerSession &serverSession, PGconn *conn) {

  if (!initDatabaseOnServer(conn)) {
    std::cout << "Не удалось инициаизировать базу на сервере. \n";
    return false;
  };

  return true;
}