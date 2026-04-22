#include "server/sql_queries/user_sql_writer.h"

#include "exceptions_cpp/sql_exception.h"
#include "system/system_function.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

UserSqlWriter::UserSqlWriter(SQLRequests &executor)
    : executor_(executor) {}

bool UserSqlWriter::CreateUserSQL(const UserDTO &user_dto,
                                         PGconn *conn) {
  PGresult *result = nullptr;

  try {
    std::string sql = R"(with
	user_insert as (
		insert into public.users (login, name, email, phone)
	values (')";

    sql += makeStringForSQL(user_dto.login) + "', '";
    sql += makeStringForSQL(user_dto.userName) + "', '";
    sql += makeStringForSQL(user_dto.email) + "', '";
    sql += makeStringForSQL(user_dto.phone) + "') \n";

    sql += R"(returning id
	),
	passhash_insert as (
		insert into public.users_passhash (user_id, password_hash)
		select user_insert.id, ')";

    sql += makeStringForSQL(user_dto.passwordhash) + "'\n";

    sql += R"(from user_insert
		returning user_id
		)
		SELECT (SELECT count(*) FROM user_insert)
     	+ (SELECT count(*) FROM passhash_insert) AS total_updated;)";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", CreateUserSQL");
    }

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
      PQclear(result);
      throw exc::SQLSelectException(", CreateUserSQL");
    }

    if (PQntuples(result) != 1) {
      PQclear(result);
      throw exc::SQLSelectException(", CreateUserSQL");
    }

    int quantity = std::atoi(PQgetvalue(result, 0, 0));

    if (quantity == 0) {
      PQclear(result);
      throw exc::SQLSelectException(", CreateUserSQL");
    }

    PQclear(result);
    return true;
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }
}

bool UserSqlWriter::ChangeUserDataSQL(const UserDTO &user_dto,
                                             PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(user_dto.login);
    const std::string name = makeStringForSQL(user_dto.userName);
    const std::string email = makeStringForSQL(user_dto.email);
    const std::string phone = makeStringForSQL(user_dto.phone);

    const std::string sql =
        "UPDATE public.users SET name = '" + name + "', email = '" + email +
        "', phone = '" + phone + "' WHERE login = '" + login + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", ChangeUserDataSQL");
    }

    const char *tuples = PQcmdTuples(result);
    const long affected_rows =
        tuples != nullptr ? std::strtol(tuples, nullptr, 10) : 0;

    PQclear(result);

    return affected_rows > 0;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

bool UserSqlWriter::BlockUserSQL(const UserDTO &user_dto, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(user_dto.login);
    const std::string disable_reason = makeStringForSQL(user_dto.disable_reason);

    const std::string sql = "UPDATE public.users SET is_active = false, disable_reason = '" +
                            disable_reason + "' WHERE login = '" + login + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException("BlockUserSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
      PQclear(result);
      return false;
    }

    const char *tuples = PQcmdTuples(result);
    const long affected_rows =
        tuples != nullptr ? std::strtol(tuples, nullptr, 10) : 0;
    PQclear(result);

    return affected_rows > 0;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

bool UserSqlWriter::UnblockUserSQL(const UserDTO &user_dto,
                                          PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(user_dto.login);
    const std::string disable_reason = makeStringForSQL(user_dto.disable_reason);

    const std::string sql =
        "UPDATE public.users SET is_active = true, disable_reason = '' WHERE login = '" + login + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException("UnBlockUserSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
      PQclear(result);
      return false;
    }

    const char *tuples = PQcmdTuples(result);
    const long affected_rows =
        tuples != nullptr ? std::strtol(tuples, nullptr, 10) : 0;
    PQclear(result);

    return affected_rows > 0;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

bool UserSqlWriter::BanUserSQL(const UserDTO &user_dto, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(user_dto.login);

    std::string sql = "UPDATE public.users SET bun_until = ";
    sql += std::to_string(user_dto.ban_until) + " WHERE login = '" + login + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException("BunUserSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
      PQclear(result);
      return false;
    }

    const char *tuples = PQcmdTuples(result);
    const long affected_rows =
        tuples != nullptr ? std::strtol(tuples, nullptr, 10) : 0;
    PQclear(result);

    return affected_rows > 0;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

bool UserSqlWriter::UnbanUserSQL(const UserDTO &user_dto, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(user_dto.login);

    std::string sql = "UPDATE public.users SET bun_until = 0 WHERE login = '" + login + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException("BunUserSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
      PQclear(result);
      return false;
    }

    const char *tuples = PQcmdTuples(result);
    const long affected_rows =
        tuples != nullptr ? std::strtol(tuples, nullptr, 10) : 0;
    PQclear(result);

    return affected_rows > 0;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}
