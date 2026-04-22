with
	user_e_record as (
		select id as user_id
		from public.users
		where login = 'e'
	),
	user_a_record as (
		select id as user_id
		from public.users
		where login = 'a'
	),
	chat_created as (
		insert into public.chats default values
		returning id as chat_id
	),
	message1_insert as (
		insert into public.messages (chat_id, sender_id, message_text, time_stamp)
		select chat_created.chat_id, user_e_record.user_id, 'Привет!', 1743508800000
		from chat_created cross join user_e_record
		returning id as message_id
	),
	message1_status_e_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message1_insert.message_id, user_e_record.user_id, 'READ', false
		from message1_insert, user_e_record
		returning message_id
	),
	message1_status_a_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message1_insert.message_id, user_a_record.user_id, 'READ', false
		from message1_insert, user_a_record
		returning message_id
	),
	message2_insert as (
		insert into public.messages (chat_id, sender_id, message_text, time_stamp)
		select chat_created.chat_id, user_a_record.user_id, 'Хай! как делишки?', 1743509100000
		from chat_created cross join user_a_record
		returning id as message_id
	),
	message2_status_e_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message2_insert.message_id, user_e_record.user_id, 'READ', false
		from message2_insert, user_e_record
		returning message_id
	),
	message2_status_a_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message2_insert.message_id, user_a_record.user_id, 'READ', false
		from message2_insert, user_a_record
		returning message_id
	),
	message3_insert as (
		insert into public.messages (chat_id, sender_id, message_text, time_stamp)
		select chat_created.chat_id, user_e_record.user_id, 'Хорошо, как насчет кофе?', 1743509220000
		from chat_created cross join user_e_record
		returning id as message_id
	),
	message3_status_e_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message3_insert.message_id, user_e_record.user_id, 'READ', false
		from message3_insert, user_e_record
		returning message_id
	),
	message3_status_a_record as (
		insert into public.message_status (message_id, user_id, status, status_deleted)
		select message3_insert.message_id, user_a_record.user_id, 'NOT DELIVERED', false
		from message3_insert, user_a_record
		returning message_id
	),
	participants_insert as (
		insert into public.participants (chat_id, user_id, last_read_message_id, deleted_from_chat)
		select chat_created.chat_id, user_e_record.user_id, message3_insert.message_id, false
		from chat_created, user_e_record, message3_insert
		union all
		select chat_created.chat_id, user_a_record.user_id, message2_insert.message_id, false
		from chat_created, user_a_record, message2_insert
		returning chat_id
	)
select 1;