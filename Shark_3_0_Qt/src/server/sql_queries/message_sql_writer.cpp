#include "server/sql_queries/message_sql_writer.h"

#include "exceptions_cpp/sql_exception.h"
#include "system/system_function.h"

#include <cstdlib>
#include <iostream>

MessageSqlWriter::MessageSqlWriter(SQLRequests &executor)
    : executor_(executor) {}

bool MessageSqlWriter::SetLastReadMessageSQL(
    const MessageDTO &message_dto, PGconn *conn) {
  PGresult *result = nullptr;

  try {
    std::string sql = R"(WITH 
		user_record AS (
  		SELECT id AS u_id 
  		FROM public.users
  		WHERE login = ')";

    sql += makeStringForSQL(message_dto.senderLogin);

    sql += R"('
	),
	time_stamp_record AS (
		SELECT time_stamp AS m_time_stamp 
		FROM public.messages
		WHERE id = )";
    sql += std::to_string(message_dto.messageId) + " \n";

    sql += R"(),
	messages_record_list(message_id) AS (
  	SELECT m.id
  		FROM public.messages m
  		JOIN public.message_status ms 
    		ON ms.message_id = m.id
		WHERE m.chat_id = )";

    sql += std::to_string(message_dto.chatId) + " \n";

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

    sql += std::to_string(message_dto.messageId) + " \n";

    sql += "WHERE pp.chat_id = ";

    sql += std::to_string(message_dto.chatId) + " \n";

    sql += R"(AND pp.user_id = (SELECT u_id FROM user_record)
	RETURNING 1 AS cnt
	)
	SELECT (SELECT count(*) FROM message_status_update)
     + (SELECT count(*) FROM last_read_update) AS total_updated;)";

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", SetLastReadMessageSQL");
    }

    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
      int quantity =
          static_cast<int>(std::strtol(PQgetvalue(result, 0, 0), nullptr, 10));

      if (quantity == 0) {
        throw exc::SQLSelectException(", SetLastReadMessageSQL");
      }

      PQclear(result);
      return true;
    }

    PQclear(result);
    return false;
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }
}

std::size_t MessageSqlWriter::CreateMessageSQL(
    const MessageDTO &message_dto, PGconn *conn) {
  PGresult *result = nullptr;
  std::size_t value = 0;

  try {
    std::string sql = R"(with 
		user_record as (
		select id as sender_id
			from public.users
		where login = ')";

    sql += makeStringForSQL(message_dto.senderLogin) + "'),\n ";

    sql += R"(chat_record as(
		select id as chat_id
			from public.chats
		where id = ')";

    sql += makeStringForSQL(std::to_string(message_dto.chatId)) + "'),\n ";

    sql += R"(participants_list as (
		select pp.user_id 
			from public.participants pp
		where pp.chat_id = (
			select chat_record.chat_id from chat_record)
		),
		message_insert as (
  			insert into public.messages (chat_id, sender_id, message_text, time_stamp)
  				select cr.chat_id, ur.sender_id, ')";

    sql += makeStringForSQL(message_dto.messageContent[0].payload) + "', ";

    sql += makeStringForSQL(std::to_string(message_dto.timeStamp)) + "\n";

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

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", CreateMessageSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK &&
        PQresultStatus(result) != PGRES_TUPLES_OK) {
      throw exc::SQLSelectException(", CreateMessageSQL");
    }

    value = static_cast<size_t>(std::stoul(PQgetvalue(result, 0, 0)));

    PQclear(result);
    return value;
  } catch (const exc::SQLSelectException &ex) {
    if (result != nullptr) {
      PQclear(result);
    }
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return 0;
  }
}
