#include "server/sql_queries/user_sql_reader.h"

#include "exceptions_cpp/sql_exception.h"
#include "system/system_function.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

UserSqlReader::UserSqlReader(SQLRequests &executor)
    : executor_(executor) {}

bool UserSqlReader::CheckUserLoginSQL(const std::string &login,
                                          PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string escaped_login = makeStringForSQL(login);
    const std::string sql =
        "select id from public.users as u where u.login = '" + escaped_login +
        "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(
          ", UserSqlReader::CheckUserLoginSQL");
    }

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      PQclear(result);
      return true;
    }

    PQclear(result);
    return false;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

bool UserSqlReader::CheckUserPasswordSQL(
    const UserLoginPasswordDTO &credentials, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login = makeStringForSQL(credentials.login);
    const std::string password = makeStringForSQL(credentials.passwordhash);

    const std::string sql =
        "with user_record as (\n"
        "    select id as user_id\n"
        "    from public.users\n"
        "    where login = '" +
        login +
        "')\n"
        "select password_hash from public.users_passhash as ph join "
        "user_record ur on ph.user_id = ur.user_id where password_hash = '" +
        password + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(
          ", UserSqlReader::CheckUserPasswordSQL");
    }

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      PQclear(result);
      return true;
    }

    PQclear(result);
    return false;
  } catch (const exc::SQLSelectException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    if (result != nullptr) {
      PQclear(result);
    }
    return false;
  }
}

std::optional<UserDTO> UserSqlReader::GetUserWithPasshashSQL(
    const std::string &login, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    const std::string login_escaped = makeStringForSQL(login);

    std::string sql = R"(select * from public.users as us  
		join public.users_passhash as ph on ph.user_id = us.id
		where us.login = ')";
    sql += login_escaped + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetUserWithPasshashSQL");
    }

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      UserDTO user_dto;

      user_dto.login = PQgetvalue(result, 0, 1);
      user_dto.userName = PQgetvalue(result, 0, 2);
      user_dto.email = PQgetvalue(result, 0, 3);
      user_dto.phone = PQgetvalue(result, 0, 4);
      user_dto.is_active = (std::strcmp(PQgetvalue(result, 0, 5), "t") == 0);
      user_dto.disabled_at = static_cast<std::size_t>(
          std::strtoull(PQgetvalue(result, 0, 6), nullptr, 10));
      user_dto.ban_until = static_cast<std::size_t>(
          std::strtoull(PQgetvalue(result, 0, 7), nullptr, 10));
      user_dto.disable_reason = PQgetvalue(result, 0, 8);
      user_dto.passwordhash = PQgetvalue(result, 0, 10);

      PQclear(result);
      return user_dto;
    }

    PQclear(result);
    throw exc::SQLSelectException(", GetUserWithPasshashSQL");
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::vector<UserDTO>> UserSqlReader::GetUsersByTextPartSQL(
    const UserLoginPasswordDTO &packet, PGconn *conn) {
  PGresult *result = nullptr;
  std::vector<UserDTO> value;

  try {
    std::string sql;
    sql = R"(select * 
	from public.users 
	where (lower(login) similar to '%)";

    sql += makeStringForSQL(packet.passwordhash);

    sql += R"(%'
  or lower(name) similar to '%)";

    const std::string text_to_find_lower = TextToLower(packet.passwordhash);
    sql += makeStringForSQL(text_to_find_lower);

    sql += R"(%') and login <>')";

    const std::string login_exclude = TextToLower(packet.login);
    sql += makeStringForSQL(login_exclude);
    sql += "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetUsersByTextPartSQL");
    }

    const auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {
      for (int i = 0; i < quantity; ++i) {
        UserDTO user_dto;
        user_dto.login = PQgetvalue(result, i, 1);
        user_dto.userName = PQgetvalue(result, i, 2);
        user_dto.email = PQgetvalue(result, i, 3);
        user_dto.phone = PQgetvalue(result, i, 4);
        user_dto.is_active = (*PQgetvalue(result, i, 5) == 't');
        user_dto.disabled_at = std::stoull(PQgetvalue(result, i, 6));
        user_dto.ban_until = std::stoull(PQgetvalue(result, i, 7));
        user_dto.disable_reason = PQgetvalue(result, i, 8);
        value.push_back(user_dto);
      }

      PQclear(result);
      return value;
    }

    PQclear(result);
    return std::nullopt;
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::vector<UserDTO>> UserSqlReader::GetSeveralUsersSQL(
    const std::vector<std::string> &logins, PGconn *conn) {
  if (logins.empty()) {
    return std::nullopt;
  }

  PGresult *result = nullptr;
  std::vector<UserDTO> value;

  try {
    std::string sql = "with users_list(login) as (values\n";

    bool first = true;
    for (const auto &login : logins) {
      if (!first) {
        sql += ",\n";
      }
      sql += "('" + makeStringForSQL(login) + "')";
      first = false;
    }

    sql += ")\n";
    sql += R"(
	select pu.login, pu.name, pu.email, pu.phone, pu.is_active,	pu.disabled_at, pu.ban_until, pu.disable_reason 
	from public.users pu
	join users_list ul on pu.login = ul.login;)";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetSeveralUsersSQL");
    }

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
      auto quantity = PQntuples(result);

      for (int i = 0; i < quantity; ++i) {
        UserDTO user_dto;
        user_dto.login = PQgetvalue(result, i, 0);
        user_dto.userName = PQgetvalue(result, i, 1);
        user_dto.email = PQgetvalue(result, i, 2);
        user_dto.phone = PQgetvalue(result, i, 3);
        user_dto.is_active = (*PQgetvalue(result, i, 4) == 't');
        user_dto.disabled_at = std::stoull(PQgetvalue(result, i, 5));
        user_dto.ban_until = std::stoull(PQgetvalue(result, i, 6));
        user_dto.disable_reason = PQgetvalue(result, i, 7);

        value.push_back(user_dto);
      }

      PQclear(result);
      return value;
    }

    PQclear(result);
    throw exc::SQLSelectException(", GetSeveralUsersSQL");
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}
