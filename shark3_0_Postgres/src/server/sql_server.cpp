#include "sql_server.h"
#include "dto_struct.h"
#include "exception/sql_exception.h"
#include "message/message_content.h"
#include "message/message_content_struct.h"
#include "system/system_function.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <vector>

// utilities
bool checkEmptyBaseSQL(PGconn *conn) {

  std::string sql = "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema = 'public';";
  PGresult *result = PQexec(conn, sql.c_str());
  std::string error = "";
  try {
    if (!result) {
      error = PQerrorMessage(conn);
      throw exc::SQLEmptyBaseException(error);
    }
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
      error = PQresultErrorMessage(result);
      PQclear(result);
      throw exc::SQLEmptyBaseException(error);
    }

    long count = std::stol(PQgetvalue(result, 0, 0));

    PQclear(result);

    if (count > 0)
      return false;
    else
      return true;
  } // try
  catch (exc::SQLEmptyBaseException &ex) {
    std::cerr << ex.what() << "\n";
    return false;
  }
}
//
//
//
bool clearBaseSQL(PGconn *conn) {

  std::multimap<int, std::string> sqlRequests;
  std::multimap<int, std::string> sqlDescription;

  sqlRequests.clear();
  sqlDescription.clear();

  PGresult *result;

  std::string sql =
      R"(DROP OWNED BY CURRENT_USER CASCADE;
	  )";
  result = execSQL(conn, sql);

  sql =
      R"(CREATE SCHEMA public AUTHORIZATION CURRENT_USER;
SET search_path TO public;)";
  result = execSQL(conn, sql);

  bool ok = (result != nullptr);
  if (result)
    PQclear(result);
  return ok;
  return true;
}

//
//
//
bool checkBaseTablesSQL(PGconn *conn, std::multimap<int, std::string> sqlMultimap,
                        std::multimap<int, std::string> sqlDescription) {

  using json = nlohmann::json;

  std::ifstream file_schema(std::string(CONFIG_DIR) + "/schema_db.conf");
  if (!file_schema.is_open()) {
    std::cerr << "schema_db.conf: файл не открыт\n";
    return false;
  }
  json config;
  try {
    file_schema >> config;
  } catch (const std::exception &e) {
    throw exc::SQLReadConfigException(" schema_db.conf: : ошибка JSON: ");
  }

  int quantity = 0;
  try {
    quantity = config.at("database").at("quantity").get<int>();

  } catch (const std::exception &e) {
    throw exc::SQLReadConfigException(" schema_db.conf: quantity ");
  }

  if (quantity <= 0) {
    throw exc::SQLReadConfigException(" schema_db.conf: quantity ");
  }

  try {
    for (int i = 1; i <= quantity; ++i) {

      // взяли очередную таблицу
      std::string tableKey = "table" + std::to_string(i);
      std::string tableName;
      try {
        tableName = config.at("database").at(tableKey).get<std::string>();

      } catch (const std::exception &e) {
        throw exc::SQLCreateTableException("schema_db.conf: отсутствует или неверный тип ключа: " + tableKey);
      }

      // сформировали запрос
      std::string sql = "SELECT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_schema = 'public' "
                        "     AND table_name = '" +
                        tableName + "');";

      // выполнили запрос
      PGresult *result = execSQL(conn, sql);
      if (!result) {
        throw exc::SQLTableAbscentException(tableName);
      }

      if (PQresultStatus(result) != PGRES_TUPLES_OK || PQntuples(result) < 1 || PQnfields(result) < 1) {
        PQclear(result);
        throw exc::SQLTableAbscentException(tableName);
      }

      // правка №2: безопасная проверка NULL
      if (PQgetisnull(result, 0, 0)) {
        PQclear(result);
        throw exc::SQLTableAbscentException(tableName);
      }

      const char *val = PQgetvalue(result, 0, 0);
      bool exists = (val && std::string(val) == "t");

      if (!exists) {
        PQclear(result);
        throw exc::SQLTableAbscentException(tableName);
      }
      PQclear(result);
    }
  } // try
  catch (exc::SQLTableAbscentException &ex) {
    std::cerr << ex.what() << "\n"
              << " Переинициализируйте базу." << "\n";
    return false;
  } catch (exc::SQLReadConfigException &ex) {
    std::cerr << ex.what() << "\n"
              << " Переинициализируйте базу." << "\n";
    return false;
  } catch (exc::SQLCreateTableException &ex) {
    std::cerr << ex.what() << "\n"
              << " Переинициализируйте базу." << "\n";
    return false;
  }
  return true;
}
//
//
// sql command

PGresult *execTransactionToSQL(PGconn *conn, std::multimap<int, std::string> &sqlRequests,
                               std::multimap<int, std::string> &sqlDescription) {

  PGresult *result = PQexec(conn, "BEGIN");
  std::string errorTable;

  try {
    // открываем транзакцию

    if (!result || PQresultStatus(result) != PGRES_COMMAND_OK) {
      if (result)
        PQclear(result);
      throw exc::SQLExecException("Открытие транзакции.");
    }
    PQclear(result);
    int i = 1;

    for (const auto &request : sqlRequests) {

      // выполнили запрос
      result = execSQL(conn, request.second);

      if (!result) {
        auto it = sqlDescription.find(i);
        errorTable = (it != sqlDescription.end()) ? it->second : std::string();

        throw exc::SQLCreateTableException("p" + std::to_string(i) + ", " + request.second);
      }

      // определили результат
      auto st = PQresultStatus(result);
      if (st != PGRES_COMMAND_OK && st != PGRES_TUPLES_OK) {
        PQclear(result);

        auto it = sqlDescription.find(i);
        errorTable = (it != sqlDescription.end()) ? it->second : std::string();

        throw exc::SQLCreateTableException("p" + std::to_string(i) + ", " + request.second);
      }
      PQclear(result);
      ++i;
    } // for
  } // try
  catch (exc::SQLCreateTableException &ex) {
    std::cerr << ex.what() << "\n"
              << " Проверьте SQL запросы: " << errorTable << "\n";
    result = PQexec(conn, "ROLLBACK");
    if (result)
      PQclear(result);
    return nullptr;
  } catch (exc::SQLExecException &ex) {
    std::cerr << ex.what() << "\n"
              << " Проверьте доступ к базе." << "\n";
    result = PQexec(conn, "ROLLBACK");
    if (result)
      PQclear(result);
    return nullptr;
  }
  result = PQexec(conn, "COMMIT");
  if (!result)
    return nullptr;
  bool ok = (PQresultStatus(result) == PGRES_COMMAND_OK);

  return ok ? result : nullptr;
}
//
//
//
PGresult *execSQL(PGconn *conn, const std::string &sql) {

  PGresult *result = PQexec(conn, sql.c_str());

  try {
    if (!result) {
      std::string error = PQerrorMessage(conn);
      throw exc::SQLExecException(error);
    }
    if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK) {
      std::string error = PQresultErrorMessage(result);
      PQclear(result);
      throw exc::SQLExecException(error);
    }
  } // try
  catch (exc::SQLExecException &ex) {
    std::cerr << ex.what() << "\n";
    return nullptr;
  }

  return result;
}

//
//
//
std::vector<std::string> getChatListSQL(PGconn *conn, const std::string &login) {

  PGresult *result = nullptr;
  std::vector<std::string> value;
  value.clear();

  std::string sql = "";

  try {

    sql = R"(with user_record as (
		select id as user_id from public.users where login = ')";

    std::string loginEsc = makeStringForSQL(login);
    sql += loginEsc + "')\n";

    sql += R"(select distinct chat_id from public.participants as pa
		join user_record as ur
		on pa.user_id = ur.user_id
		where pa.user_id = ur.user_id;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getChatListSQL");

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
      PQclear(result);
      throw exc::SQLSelectException(", getChatListSQL");
    }

    int quantity = PQntuples(result);

    if (quantity == 0) {
      PQclear(result);
      value.clear();
      return value;
    }

    for (int i = 0; i < quantity; ++i)
      value.push_back(PQgetvalue(result, i, 0));

    PQclear(result);
    return value;
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    value.clear();
    return value;
  }
}

std::optional<std::vector<ParticipantsDTO>> getChatParticipantsSQL(PGconn *conn, const std::string &chat_id) {

  PGresult *result = nullptr;
  std::vector<ParticipantsDTO> value;
  value.clear();

  std::string sql = "";

  try {

    sql = R"(select us.login, pa.last_read_message_id, pa.deleted_from_chat
         from public.participants pa
         join public.users us on us.id = pa.user_id
         where pa.chat_id = ')" +
          makeStringForSQL(chat_id) + "';";
    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getChatParticipantsSQL");

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {

      for (int i = 0; i < quantity; ++i) {
        ParticipantsDTO participantsDTO;

        participantsDTO.login = PQgetvalue(result, i, 0);

        if (PQgetisnull(result, i, 1) == 0) {
          participantsDTO.lastReadMessage = static_cast<std::int64_t>(std::stoll(PQgetvalue(result, i, 1)));
        } else {
          participantsDTO.lastReadMessage = 0; // или любое значение по умолчанию
        }

        participantsDTO.deletedFromChat = (PQgetvalue(result, i, 2)[0] == 't');
        value.push_back(participantsDTO);
      }
      PQclear(result);
      return value;

    } else {
      PQclear(result);
      throw exc::SQLSelectException(", getChatParticipantsSQL");
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::multiset<std::pair<std::string, std::size_t>>> getChatMessagesDeletedStatusSQL(
    PGconn *conn, const std::string &chat_id) {

  PGresult *result = nullptr;
  std::multiset<std::pair<std::string, std::size_t>> value;
  value.clear();

  std::string sql = "";

  try {

    sql = R"(select message_id, user_id, login, status_deleted, chat_id 
		from 
		public.message_status as ms_st
		join messages as ms on ms_st.message_id = ms.id
		join users as us on ms_st.user_id = us.id
		where ms.chat_id = ')";

    sql += makeStringForSQL(chat_id) + "' and ms_st.status_deleted = true ";
    sql += "order by us.login;";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getChatMessagesDeletedStatusSQL");

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
      if (quantity > 0) {

        for (int i = 0; i < quantity; ++i) {
          value.insert({PQgetvalue(result, i, 2), static_cast<std::int64_t>(std::stoll(PQgetvalue(result, i, 0)))});
        }
      }
      PQclear(result);
      result = nullptr;
    } else {
      PQclear(result);
      result = nullptr;
      throw exc::SQLSelectException(", getChatMessagesDeletedStatusSQL");
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    result = nullptr;
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
  return value;
}
//
//
//
std::optional<MessageChatDTO> getChatMessagesSQL(PGconn *conn, const std::string &chat_id) {

  PGresult *result = nullptr;
  MessageChatDTO value;
  value.chatId = static_cast<std::size_t>(std::stoull(chat_id));

  std::string sql = "";
  sql = R"(select ps.id, chat_id, sender_id, message_text, time_stamp, login 
	from public.messages as ps 
	join public.users as pu on pu.id = ps.sender_id
	where chat_id = ')";
  sql += makeStringForSQL(chat_id) + "';";

  try {
    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getChatMessagesSQL");

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {

      for (int i = 0; i < quantity; ++i) {

        MessageDTO messageDTO;
        messageDTO.senderLogin = PQgetvalue(result, i, 5);

        messageDTO.chatId = static_cast<std::size_t>(std::stoull(PQgetvalue(result, i, 1)));
        messageDTO.messageId = static_cast<std::int64_t>(std::stoll(PQgetvalue(result, i, 0)));
        messageDTO.timeStamp = static_cast<std::int64_t>(std::stoll(PQgetvalue(result, i, 4)));

        MessageContentDTO temContent;
        temContent.messageContentType = MessageContentType::Text;

        temContent.payload = std::string(PQgetvalue(result, i, 3));

        messageDTO.messageContent.push_back(temContent);

        value.messageDTO.push_back(messageDTO);
      }

      PQclear(result);
      return value;

    } else {
      PQclear(result);
      throw exc::SQLSelectException(", getChatMessagesSQL");
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;

    return std::nullopt;
  }
}

std::optional<std::vector<UserDTO>> getUsersByTextPartSQL(PGconn *conn, const UserLoginPasswordDTO &packet) {

  PGresult *result = nullptr;
  std::vector<UserDTO> value;

  std::string textToFindLower = TextToLower(packet.passwordhash);
  std::string loginExclude = TextToLower(packet.login);

  std::string sql = "";
  sql = R"(select * 
	from public.users 
	where (lower(login) similar to '%)";

  sql += makeStringForSQL(packet.passwordhash);

  sql += R"(%'
  or lower(name) similar to '%)";

  sql += makeStringForSQL(textToFindLower);

  sql += R"(%') and login <>')";
  sql += makeStringForSQL(loginExclude);

  sql += "';";

  try {
    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getUsersByTextPartSQL");

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {

      for (int i = 0; i < quantity; ++i) {

        UserDTO userDTO;
        userDTO.login = PQgetvalue(result, i, 1);
        userDTO.userName = PQgetvalue(result, i, 2);
        userDTO.email = PQgetvalue(result, i, 3);
        userDTO.phone = PQgetvalue(result, i, 4);

        value.push_back(userDTO);
      }

      PQclear(result);
      return value;

    } else {
      PQclear(result);
      throw exc::SQLSelectException(", getUsersByTextPartSQL");
    }
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;

    return std::nullopt;
  }
}

std::optional<std::vector<UserDTO>> getSeveralUsersDTOFromSrvSQL(PGconn *conn, const std::vector<std::string> &logins) {

  PGresult *result = nullptr;
  std::vector<UserDTO> value;

  std::string sql = "";

  try {

    sql = "with users_list(login) as (values\n";

    // добавляем все логины в запрос
    bool first = true;
    for (const auto &login : logins) {

      if (!first)
        sql += ",\n"; // запятая только между элементами
      sql += "('" + makeStringForSQL(login) + "')";
      first = false;
    }

    sql += ")\n";
    sql += R"(
	select pu.login, name, email, phone 
	from public.users pu
	join users_list ul on pu.login = ul.login;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", getSeveralUsersDTOFromSrvSQL");

    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {

      auto quantity = PQntuples(result);

      if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {

        for (int i = 0; i < quantity; ++i) {

          UserDTO userDTO;
          userDTO.login = PQgetvalue(result, i, 0);
          userDTO.userName = PQgetvalue(result, i, 1);
          userDTO.email = PQgetvalue(result, i, 2);
          userDTO.phone = PQgetvalue(result, i, 3);

          value.push_back(userDTO);
        }

        PQclear(result);
        return value;

      } else {
        PQclear(result);
        throw exc::SQLSelectException(", getSeveralUsersDTOFromSrvSQL");
      }
    } else
      throw exc::SQLSelectException(", getSeveralUsersDTOFromSrvSQL");
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

bool setLastReadMessageSQL(PGconn *conn, const MessageDTO &messageDTO) {

  PGresult *result = nullptr;
  bool value = false;

  std::string sql = "";

  try {
    sql = R"(WITH 
		user_record AS (
  		SELECT id AS u_id 
  		FROM public.users
  		WHERE login = ')";

    sql += makeStringForSQL(messageDTO.senderLogin);

    sql += R"('
	),
	time_stamp_record AS (
		SELECT time_stamp AS m_time_stamp 
		FROM public.messages
		WHERE id = )";
    sql += std::to_string(messageDTO.messageId) + " \n";

    sql += R"(),
	messages_record_list(message_id) AS (
  	SELECT m.id
  		FROM public.messages m
  		JOIN public.message_status ms 
    		ON ms.message_id = m.id
		WHERE m.chat_id = )";

    sql += std::to_string(messageDTO.chatId) + " \n";

    sql += "AND m.time_stamp <= ";

    sql += R"((select m_time_stamp from time_stamp_record) )";

    sql += R"(AND ms.status <> 'READ'
    	AND ms.user_id = (SELECT u_id FROM user_record)
	),
	message_status_update AS (
  		UPDATE public.message_status ms 
  			SET status = 'READ'
  		WHERE ms.user_id = (SELECT u_id FROM user_record)
    		AND ms.message_id IN (SELECT message_id FROM messages_record_list)
  	RETURNING 1 AS cnt
	),
	last_read_update AS (
  		UPDATE public.participants AS pp
  			SET last_read_message_id = )";

    sql += std::to_string(messageDTO.messageId) + " \n";

    sql += "WHERE pp.chat_id = ";

    sql += std::to_string(messageDTO.chatId) + " \n";

    sql += R"(AND pp.user_id = (SELECT u_id FROM user_record)
	RETURNING 1 AS cnt
	)
	SELECT (SELECT count(*) FROM message_status_update)
     + (SELECT count(*) FROM last_read_update) AS total_updated;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", setLastReadMessageSQL");

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {

      int quantity = static_cast<int>(std::strtol(PQgetvalue(result, 0, 0), nullptr, 10));

      if (quantity == 0)
        throw exc::SQLSelectException(", setLastReadMessageSQL");

      PQclear(result);
      value = true;
      return value;
    } else {
      PQclear(result);
      return false;
    }

  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }
}

bool createUserSQL(PGconn *conn, const UserDTO &userDTO) {

  PGresult *result = nullptr;

  std::string sql = "";

  try {

    sql = R"(with
	user_insert as (
		insert into public.users (login, name, email, phone)
	values (')";

    sql += makeStringForSQL(userDTO.login) + "', '";
    sql += makeStringForSQL(userDTO.userName) + "', '";
    sql += makeStringForSQL(userDTO.email) + "', '";
    sql += makeStringForSQL(userDTO.phone) + "') \n";

    sql += R"(returning id
	),
	passhash_insert as (
		insert into public.users_passhash (user_id, password_hash)
		select user_insert.id, ')";

    sql += makeStringForSQL(userDTO.passwordhash) + "'\n";

    sql += R"(from user_insert
		returning user_id
		)
		SELECT (SELECT count(*) FROM user_insert)
     	+ (SELECT count(*) FROM passhash_insert) AS total_updated;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", createUserSQL");

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
      PQclear(result);
      throw exc::SQLSelectException(", createUserSQL");
    }

    if (PQntuples(result) != 1) {
      PQclear(result);
      throw exc::SQLSelectException(", createUserSQL");
    }

    int quantity = std::atoi(PQgetvalue(result, 0, 0));

    if (quantity == 0) {
      PQclear(result);
      throw exc::SQLSelectException(", createUserSQL");
    }

    PQclear(result);

  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }
  return true;
}

std::vector<std::string> createChatAndMessageSQL(PGconn *conn, const ChatDTO &chatDTO,
                                                 const MessageChatDTO &messageChatDTO) {

  PGresult *result = nullptr;
  std::vector<std::string> value;

  std::string sql = "";

  try {

    sql = R"(with 
		user_record as (
			select id as sender_id
			from public.users
			where login = ')";

    sql += makeStringForSQL(chatDTO.senderLogin) + "'),\n ";

    sql += R"(users_list(login) as (
		values
		)";

    // добавляем все логины в запрос
    bool first = true;
    for (const auto &login : chatDTO.participants) {

      if (!first)
        sql += ",\n"; // запятая только между элементами
      sql += "('" + makeStringForSQL(login.login) + "')";
      first = false;
    }

    sql += "),\n";

    sql += R"(users_records as(
	select id as user_id
		from public.users
	where login in (
		select login from users_list)
	),
	chat_created as (
		insert into public.chats default values
		returning id as chat_id	
	),
	message_insert as (
  		insert into public.messages (chat_id, sender_id, message_text, time_stamp)
  		select c.chat_id, ur.sender_id, ')";

    sql += makeStringForSQL(messageChatDTO.messageDTO[0].messageContent[0].payload) + "', ";

    sql += makeStringForSQL(std::to_string(messageChatDTO.messageDTO[0].timeStamp)) + "\n";

    sql += R"(from chat_created c cross join user_record ur
		  returning id as message_id, id, chat_id, sender_id, message_text, time_stamp
	),
	message_status_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message_insert.message_id, user_record.sender_id, 'READ', false
		from message_insert, user_record
	returning message_id
	),
	message_status_part_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
	  	select message_insert.message_id, users_records.user_id, 'NOT DELIVERED', false
  		from message_insert, users_records, user_record
  		where users_records.user_id <> user_record.sender_id
  	returning message_id
	),
	participants_insert_sender as (
		insert into public.participants (chat_id, user_id, last_read_message_id, deleted_from_chat)
		select c.chat_id, ur.sender_id, mi.message_id, false
		from chat_created c, user_record ur, message_insert mi
	returning chat_id
	),
	participants_insert_others as (
		insert into public.participants (chat_id, user_id, last_read_message_id, deleted_from_chat)
		select c.chat_id, u.user_id, null, false
	from chat_created c
		join users_records u on true
		join user_record ur on true
	where u.user_id <> ur.sender_id
	returning chat_id
	)
	select id, chat_id, sender_id, message_text, time_stamp
	from message_insert;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", createChatSQL");

    if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK)
      throw exc::SQLSelectException(", createChatSQL");

    value.push_back(PQgetvalue(result, 0, 0));
    value.push_back(PQgetvalue(result, 0, 1));

    PQclear(result);
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    value.clear();
    return value;
  }
  return value;
}

std::size_t createMessageSQL(PGconn *conn, const MessageDTO &messageDTO) {

  PGresult *result = nullptr;
  std::size_t value;

  std::string sql = "";

  try {

    sql = R"(with 
		user_record as (
		select id as sender_id
			from public.users
		where login = ')";

    sql += makeStringForSQL(messageDTO.senderLogin) + "'),\n ";

    sql += R"(chat_record as(
		select id as chat_id
			from public.chats
		where id = ')";

    sql += makeStringForSQL(std::to_string(messageDTO.chatId)) + "'),\n ";

    sql += R"(participants_list as (
		select pp.user_id 
			from public.participants pp
		where pp.chat_id = (
			select chat_record.chat_id from chat_record)
		),
		message_insert as (
  			insert into public.messages (chat_id, sender_id, message_text, time_stamp)
  				select cr.chat_id, ur.sender_id, ')";

    sql += makeStringForSQL(messageDTO.messageContent[0].payload) + "', ";

    sql += makeStringForSQL(std::to_string(messageDTO.timeStamp)) + "\n";

    sql += R"(from user_record ur cross join chat_record cr
  returning id as message_id, chat_id, sender_id, message_text, time_stamp
),
messages_prev_list(message_id) as (
	select pm.id 
	from public.messages pm
		join public.message_status ms
		on ms.message_id = pm.id
	where ms.user_id = (
		select sender_id from user_record)
  	and ms.status != 'READ'
  	and pm.chat_id = (
  		select chat_record.chat_id from chat_record)
  	and pm.time_stamp < (
  		select message_insert.time_stamp from message_insert)
),
message_st_update as (
	update message_status ms
		set status = 'READ'
	where ms.user_id = (select sender_id from user_record)
	and ms.message_id in (
		select message_id from messages_prev_list
	)
),
message_status_record as (
	insert into public.message_status (message_id, user_id, status, status_deleted)
	select message_insert.message_id, user_record.sender_id, 'READ', false
	from message_insert, user_record
	returning message_id
),
message_status_part_record as (
  insert into public.message_status (message_id, user_id, status, status_deleted)
  select message_insert.message_id, participants_list.user_id, 'NOT DELIVERED', false
  from message_insert, participants_list, user_record
  where participants_list.user_id <> user_record.sender_id
  returning message_id
),
participants_update as (
  update public.participants 
  	set last_read_message_id = (
  		select message_id from message_insert)
  	where chat_id = (
  		select chat_id from chat_record)
  	and user_id = (
  		select sender_id from user_record)
)
select message_id, chat_id, sender_id, message_text, time_stamp
from message_insert;)";

    result = execSQL(conn, sql);

    if (result == nullptr)
      throw exc::SQLSelectException(", createMessageSQL");

    if (PQresultStatus(result) != PGRES_COMMAND_OK && PQresultStatus(result) != PGRES_TUPLES_OK)
      throw exc::SQLSelectException(", createMessageSQL");

    value = static_cast<size_t>(std::stoul(PQgetvalue(result, 0, 0)));

    PQclear(result);
  } // try
  catch (const exc::SQLSelectException &ex) {
    if (result != nullptr)
      PQclear(result);
    std::cerr << "Сервер: " << ex.what() << std::endl;
    value = 0;
    return value;
  }
  return value;
}
