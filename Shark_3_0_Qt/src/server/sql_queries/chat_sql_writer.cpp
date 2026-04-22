#include "server/sql_queries/chat_sql_writer.h"

#include "exceptions_cpp/sql_exception.h"
#include "system/system_function.h"

#include <cstdlib>
#include <iostream>

ChatSqlWriter::ChatSqlWriter(SQLRequests &executor)
    : executor_(executor) {}

std::vector<std::string> ChatSqlWriter::CreateChatWithInitialMessageSQL(
    const ChatDTO &chat_dto, const MessageChatDTO &message_chat_dto,
    PGconn *conn) {
  PGresult *result = nullptr;
  std::vector<std::string> value;

  try {
    std::string sql = R"(with 
		user_record as (
			select id as sender_id
			from public.users
			where login = ')";

    sql += makeStringForSQL(chat_dto.senderLogin) + "'),\n ";

    sql += R"(users_list(login) as (
		values
		)";

    bool first = true;
    for (const auto &login : chat_dto.participants) {
      if (!first) {
        sql += ",\n";
      }
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

    sql += makeStringForSQL(
        message_chat_dto.messageDTO[0].messageContent[0].payload) + "', ";

    sql += makeStringForSQL(
        std::to_string(message_chat_dto.messageDTO[0].timeStamp)) + "\n";

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

    result = executor_.execSQL(conn, sql);

    if (result == nullptr) {
      throw exc::SQLSelectException(", CreateChatWithInitialMessageSQL");
    }

    if (PQresultStatus(result) != PGRES_COMMAND_OK &&
        PQresultStatus(result) != PGRES_TUPLES_OK) {
      throw exc::SQLSelectException(", CreateChatWithInitialMessageSQL");
    }

    value.push_back(PQgetvalue(result, 0, 0));
    value.push_back(PQgetvalue(result, 0, 1));

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
