#include "server/sql_queries/database_init_sql_builder.h"
std::multimap<int, std::string> DatabaseInitSqlBuilder::BuildSchemaSql() const {
  std::multimap<int, std::string> result;
  result.clear();
  std::string t1 = R"(set local search_path = public;
create table if not exists public.chats (
    id bigserial primary key
);
)";
  std::string t2 = R"(create table if not exists public.users (
    id bigserial primary key,
    login varchar(30) not null unique,
    name varchar(30) not null DEFAULT '',
    email varchar(100) not null DEFAULT '',
    phone varchar(100) not null DEFAULT '',
	is_active boolean not null DEFAULT true,
	disabled_at bigint not null DEFAULT 0,
	ban_until bigint  not null DEFAULT 0,
	disable_reason text not null DEFAULT ''
);
)";
  std::string t3 = R"(create table if not exists public.messages (
    id bigserial not null,
    chat_id bigint not null references public.chats(id) on delete cascade,
    sender_id bigint not null references public.users(id) on delete restrict,
    message_text text not null,
    time_stamp bigint not null,
    constraint messages_pk primary key (chat_id, id),
    constraint messages_id_uq unique (id)
);
)";
  std::string t4 = R"(create table if not exists public.message_status (
    message_id bigint not null references public.messages(id) on delete cascade,
    user_id bigint not null references public.users(id) on delete cascade,
    status varchar(20) not null check (status in ('NOT DELIVERED', 'DELIVERED', 'READ')),
    status_deleted boolean not null default false,
    constraint message_status_pk primary key (message_id, user_id)
);
)";
  std::string t5 = R"(create table if not exists public.users_passhash (
    user_id bigint primary key references public.users(id) on delete cascade,
    password_hash varchar(255)
);
)";
  std::string t6 = R"(create table if not exists public.participants (
    chat_id bigint not null references public.chats(id) on delete cascade,
    user_id bigint not null references public.users(id) on delete cascade,
    last_read_message_id bigint null,
    deleted_from_chat boolean not null default false,
    constraint participants_pk primary key (chat_id, user_id),
    constraint participants_last_read_fk foreign key (chat_id, last_read_message_id)
        references public.messages(chat_id, id)
);
)";
  std::string t7 = R"(
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('a', 'Alex', '...@gmail.com','111',true,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('e', 'Elena', '...@gmail.com','111',true,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('s', 'Sergei', '...@gmail.com','111',true,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('v', 'Vitaliy', '...@gmail.com','111',false,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('m', 'Mariya', '...@gmail.com','111',false,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('f', 'Fedor', '...@gmail.com','111',true,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('ver', 'Vera', '...@gmail.com','111',true,0,0,'');
insert into public.users (login, name, email, phone, is_active,	disabled_at, ban_until, disable_reason) 
values ('y', 'Yakov', '...@gmail.com','111',true,0,1759265999000,'');
insert into public.users_passhash (user_id, password_hash)
values 
((select id from public.users where login = 'a'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'e'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 's'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'v'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'm'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'f'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'ver'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b'),
((select id from public.users where login = 'y'), '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b');
)";
  std::string t8 = R"(CREATE TABLE user_bans (
  user_id      bigint NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  start_at     bigint NOT NULL DEFAULT extract(epoch from now())::bigint,
  end_at       bigint,reason       text,
  created_by   bigint,
  created_at   bigint NOT NULL DEFAULT extract(epoch from now())::bigint,
  revoked_at   bigint,
  revoked_by   bigint
);
CREATE INDEX idx_user_bans_user_start ON user_bans(user_id, start_at DESC);
CREATE INDEX idx_user_bans_active ON user_bans(user_id) WHERE revoked_at IS NULL;
;
)";
  result.insert({1, t1});
  result.insert({2, t2});
  result.insert({3, t3});
  result.insert({4, t4});
  result.insert({5, t5});
  result.insert({6, t6});
  result.insert({7, t7});
  result.insert({8, t8});
  return result;
}
std::multimap<int, std::string> DatabaseInitSqlBuilder::BuildChatFirstSql() const {
  std::multimap<int, std::string> result;
  // создаем первый чат
  std::string t1 = R"(with
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
)";
  result.insert({1, t1});
  return result;
}
std::multimap<int, std::string> DatabaseInitSqlBuilder::BuildChatSecondSql() const {
  std::multimap<int, std::string> result;
  // создаем второй чат
  std::string t1 = R"(WITH
  user_elena AS (
    SELECT id AS user_id FROM public.users WHERE login = 'e'
  ),
  user_alex AS (
    SELECT id AS user_id FROM public.users WHERE login = 'a'
  ),
  user_sergei AS (
    SELECT id AS user_id FROM public.users WHERE login = 's'
  ),
  user_mariya AS (
    SELECT id AS user_id FROM public.users WHERE login = 'm'
  ),
  user_yakov AS (
    SELECT id AS user_id FROM public.users WHERE login = 'y'
  ),
  chat_created AS (
    INSERT INTO public.chats DEFAULT VALUES
    RETURNING id AS chat_id
  ),
  message1_insert AS (
    INSERT INTO public.messages (chat_id, sender_id, message_text, time_stamp)
    SELECT chat_created.chat_id, user_elena.user_id, 'Всем Привееет!?', 1743512400000
    FROM chat_created CROSS JOIN user_elena
    RETURNING id AS message_id
  ),
  message1_status_elena AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message1_insert.message_id, user_elena.user_id, 'READ', false
    FROM message1_insert, user_elena
    RETURNING message_id
  ),
  message1_status_alex AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message1_insert.message_id, user_alex.user_id, 'READ', false
    FROM message1_insert, user_alex
    RETURNING message_id
  ),
  message1_status_sergei AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message1_insert.message_id, user_sergei.user_id, 'READ', false
    FROM message1_insert, user_sergei
    RETURNING message_id
  ),
  message1_status_mariya AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message1_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    FROM message1_insert, user_mariya
    RETURNING message_id
  ),
  message1_status_yakov AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message1_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    FROM message1_insert, user_yakov
    RETURNING message_id
  ),
  message2_insert AS (
    INSERT INTO public.messages (chat_id, sender_id, message_text, time_stamp)
    SELECT chat_created.chat_id, user_alex.user_id, 'И тебе не хворать!?', 1743512520000
    FROM chat_created CROSS JOIN user_alex
    RETURNING id AS message_id
  ),
  message2_status_elena AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message2_insert.message_id, user_elena.user_id, 'READ', false
    FROM message2_insert, user_elena
    RETURNING message_id
  ),
  message2_status_alex AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message2_insert.message_id, user_alex.user_id, 'READ', false
    FROM message2_insert, user_alex
    RETURNING message_id
  ),
  message2_status_sergei AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message2_insert.message_id, user_sergei.user_id, 'READ', false
    FROM message2_insert, user_sergei
    RETURNING message_id
  ),
  message2_status_mariya AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message2_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    FROM message2_insert, user_mariya
    RETURNING message_id
  ),
  message2_status_yakov AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message2_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    FROM message2_insert, user_yakov
    RETURNING message_id
  ),
  message3_insert AS (
    INSERT INTO public.messages (chat_id, sender_id, message_text, time_stamp)
    SELECT chat_created.chat_id, user_sergei.user_id, 'Всем здрассьте.', 1743513015000
    FROM chat_created CROSS JOIN user_sergei
    RETURNING id AS message_id
  ),
  message3_status_elena AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message3_insert.message_id, user_elena.user_id, 'READ', false
    FROM message3_insert, user_elena
    RETURNING message_id
  ),
  message3_status_alex AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message3_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    FROM message3_insert, user_alex
    RETURNING message_id
  ),
  message3_status_sergei AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message3_insert.message_id, user_sergei.user_id, 'READ', false
    FROM message3_insert, user_sergei
    RETURNING message_id
  ),
  message3_status_mariya AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message3_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    FROM message3_insert, user_mariya
    RETURNING message_id
  ),
  message3_status_yakov AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message3_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    FROM message3_insert, user_yakov
    RETURNING message_id
  ),
  message4_insert AS (
    INSERT INTO public.messages (chat_id, sender_id, message_text, time_stamp)
    SELECT chat_created.chat_id, user_elena.user_id, 'Куда идем?', 1743513129000
    FROM chat_created CROSS JOIN user_elena
    RETURNING id AS message_id
  ),
  message4_status_elena AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message4_insert.message_id, user_elena.user_id, 'READ', false
    FROM message4_insert, user_elena
    RETURNING message_id
  ),
  message4_status_alex AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message4_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    FROM message4_insert, user_alex
    RETURNING message_id
  ),
  message4_status_sergei AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message4_insert.message_id, user_sergei.user_id, 'READ', false
    FROM message4_insert, user_sergei
    RETURNING message_id
  ),
  message4_status_mariya AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message4_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    FROM message4_insert, user_mariya
    RETURNING message_id
  ),
  message4_status_yakov AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message4_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    FROM message4_insert, user_yakov
    RETURNING message_id
  ),
  message5_insert AS (
    INSERT INTO public.messages (chat_id, sender_id, message_text, time_stamp)
    SELECT chat_created.chat_id, user_sergei.user_id,
           'Пойдем мы с тобою за тридевять земель, в тридесятое царство, искать там мудрость и счастье, чтобы испытать силу свою и судьбу проверить, а что ждёт впереди — лишь дорога покажет',
           1743514380000
    FROM chat_created CROSS JOIN user_sergei
    RETURNING id AS message_id
  ),
  message5_status_elena AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message5_insert.message_id, user_elena.user_id, 'NOT DELIVERED', false
    FROM message5_insert, user_elena
    RETURNING message_id
  ),
  message5_status_alex AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message5_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    FROM message5_insert, user_alex
    RETURNING message_id
  ),
  message5_status_sergei AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message5_insert.message_id, user_sergei.user_id, 'READ', false
    FROM message5_insert, user_sergei
    RETURNING message_id
  ),
  message5_status_mariya AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message5_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    FROM message5_insert, user_mariya
    RETURNING message_id
  ),
  message5_status_yakov AS (
    INSERT INTO public.message_status (message_id, user_id, status, status_deleted)
    SELECT message5_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    FROM message5_insert, user_yakov
    RETURNING message_id
  ),
  participants_insert AS (
    INSERT INTO public.participants (chat_id, user_id, last_read_message_id, deleted_from_chat)
    (
      SELECT chat_created.chat_id, user_elena.user_id, message4_insert.message_id, false
        FROM chat_created, user_elena, message4_insert
      UNION ALL
      SELECT chat_created.chat_id, user_alex.user_id, message2_insert.message_id, false
        FROM chat_created, user_alex, message2_insert
      UNION ALL
      SELECT chat_created.chat_id, user_sergei.user_id, message5_insert.message_id, false
        FROM chat_created, user_sergei, message5_insert
      UNION ALL
      SELECT chat_created.chat_id, user_mariya.user_id, NULL::bigint, false
        FROM chat_created, user_mariya
      UNION ALL
      SELECT chat_created.chat_id, user_yakov.user_id, NULL::bigint, false
        FROM chat_created, user_yakov
    )
    RETURNING 1 AS ok
)
SELECT 1;
)";
  result.insert({1, t1});
  return result;
}
