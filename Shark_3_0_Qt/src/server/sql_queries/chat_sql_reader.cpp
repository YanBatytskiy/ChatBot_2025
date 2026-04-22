#include "server/sql_queries/chat_sql_reader.h"

#include "exceptions_cpp/sql_exception.h"
#include "system/system_function.h"

#include <cstdlib>
#include <iostream>

ChatSqlReader::ChatSqlReader(SQLRequests &executor)
    : executor_(executor) {}

std::vector<std::string> ChatSqlReader::GetChatIdsForUserSQL(
    const std::string &login, PGconn *conn) {
  PGresult *result = nullptr;
  std::vector<std::string> value;
  value.clear();

  try {
    std::string sql = R"(with user_record as (
		select id as user_id from public.users where login = ')";

    const std::string login_escaped = makeStringForSQL(login);
    sql += login_escaped + "')\n";

    sql += R"(select distinct chat_id from public.participants as pa
		join user_record as ur
		on pa.user_id = ur.user_id
		where pa.user_id = ur.user_id;)";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetChatIdsForUserSQL");
    }

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
      PQclear(result);
      throw exc::SQLSelectException(", GetChatIdsForUserSQL");
    }

    int quantity = PQntuples(result);

    if (quantity == 0) {
      PQclear(result);
      value.clear();
      return value;
    }

    for (int i = 0; i < quantity; ++i) {
      value.push_back(PQgetvalue(result, i, 0));
    }

    PQclear(result);
    return value;
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    value.clear();
    return value;
  }
}

std::optional<std::vector<ParticipantsDTO>> ChatSqlReader::GetChatParticipantsSQL(
    const std::string &chat_id, PGconn *conn) {
  PGresult *result = nullptr;
  std::vector<ParticipantsDTO> value;
  value.clear();

  try {
    std::string sql = R"(select us.login, pa.last_read_message_id, pa.deleted_from_chat
         from public.participants pa
         join public.users us on us.id = pa.user_id
         where pa.chat_id = ')" +
                        makeStringForSQL(chat_id) + "';";
    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetChatParticipantsSQL");
    }

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {
      for (int i = 0; i < quantity; ++i) {
        ParticipantsDTO participants_dto;

        participants_dto.login = PQgetvalue(result, i, 0);

        if (PQgetisnull(result, i, 1) == 0) {
          participants_dto.lastReadMessage = static_cast<std::int64_t>(
              std::stoll(PQgetvalue(result, i, 1)));
        } else {
          participants_dto.lastReadMessage = 0;
        }

        participants_dto.deletedFromChat = (PQgetvalue(result, i, 2)[0] == 't');
        value.push_back(participants_dto);
      }
      PQclear(result);
      return value;
    }

    PQclear(result);
    throw exc::SQLSelectException(", GetChatParticipantsSQL");
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}

std::optional<std::multiset<std::pair<std::string, std::size_t>>>
ChatSqlReader::GetChatMessagesDeletedStatusSQL(const std::string &chat_id,
                                                   PGconn *conn) {
  PGresult *result = nullptr;
  std::multiset<std::pair<std::string, std::size_t>> value;
  value.clear();

  try {
    std::string sql = R"(select message_id, user_id, login, status_deleted, chat_id 
		from 
		public.message_status as ms_st
		join messages as ms on ms_st.message_id = ms.id
		join users as us on ms_st.user_id = us.id
		where ms.chat_id = ')";

    sql += makeStringForSQL(chat_id) + "' and ms_st.status_deleted = true ";
    sql += "order by us.login;";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetChatMessagesDeletedStatusSQL");
    }

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
      if (quantity > 0) {
        for (int i = 0; i < quantity; ++i) {
          value.insert({PQgetvalue(result, i, 2),
                        static_cast<std::size_t>(
                            std::stoll(PQgetvalue(result, i, 0)))});
        }
      }
      PQclear(result);
      result = nullptr;
    } else {
      PQclear(result);
      result = nullptr;
      throw exc::SQLSelectException(", GetChatMessagesDeletedStatusSQL");
    }
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
  return value;
}

std::optional<MessageChatDTO> ChatSqlReader::GetChatMessagesSQL(
    const std::string &chat_id, PGconn *conn) {
  PGresult *result = nullptr;
  MessageChatDTO value;
  value.chatId = static_cast<std::size_t>(std::stoull(chat_id));

  try {
    std::string sql = R"(select ps.id, chat_id, sender_id, message_text, time_stamp, login 
	from public.messages as ps 
	join public.users as pu on pu.id = ps.sender_id
	where chat_id = ')";
    sql += makeStringForSQL(chat_id) + "';";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", GetChatMessagesSQL");
    }

    auto quantity = PQntuples(result);

    if (PQresultStatus(result) == PGRES_TUPLES_OK && quantity > 0) {
      for (int i = 0; i < quantity; ++i) {
        MessageDTO message_dto;
        message_dto.senderLogin = PQgetvalue(result, i, 5);
        message_dto.chatId = static_cast<std::size_t>(
            std::stoull(PQgetvalue(result, i, 1)));
        message_dto.messageId = static_cast<std::size_t>(
            std::stoull(PQgetvalue(result, i, 0)));
        message_dto.timeStamp = static_cast<std::size_t>(
            std::stoull(PQgetvalue(result, i, 4)));

        MessageContentDTO message_content_dto;
        message_content_dto.messageContentType = MessageContentType::Text;
        message_content_dto.payload = PQgetvalue(result, i, 3);

        message_dto.messageContent.push_back(message_content_dto);
        value.messageDTO.push_back(message_dto);
      }

      PQclear(result);
      return value;
    }

    PQclear(result);
    throw exc::SQLSelectException(", GetChatMessagesSQL");
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return std::nullopt;
  }
}
