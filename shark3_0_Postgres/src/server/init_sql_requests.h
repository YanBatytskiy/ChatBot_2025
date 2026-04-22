#include <map>
#include <string>
#pragma once

inline std::string checkBaseStructureSrvSQL() { return ""; };

inline std::multimap<int, std::string> createInitTablesSQL() {

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
    name varchar(30) not null,
    email varchar(100),
    phone varchar(100)
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
insert into public.users (login, name, email, phone) values ('a', 'Alex', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('e', 'Elena', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('s', 'Sergei', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('v', 'Vitaliy', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('m', 'Mariya', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('f', 'Fedor', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('ver', 'Vera', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('y', 'Yakov', '...@gmail.com','111');

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

  result.insert({1, t1});
  result.insert({2, t2});
  result.insert({3, t3});
  result.insert({4, t4});
  result.insert({5, t5});
  result.insert({6, t6});
  result.insert({7, t7});

  return result;
}

inline std::multimap<int, std::string> createChatFirstSQL() {

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

inline std::multimap<int, std::string> createChatSecondSQL() {

  std::multimap<int, std::string> result;

  // создаем второй чат

  std::string t1 = R"(with
  user_elena as (
    select id as user_id from public.users where login = 'e'
  ),
  user_alex as (
    select id as user_id from public.users where login = 'a'
  ),
  user_sergei as (
    select id as user_id from public.users where login = 's'
  ),
  user_mariya as (
    select id as user_id from public.users where login = 'm'
  ),
  user_yakov as (
    select id as user_id from public.users where login = 'y'
  ),
  chat_created as (
    insert into public.chats default values
    returning id as chat_id
  ),

  message1_insert as (
    insert into public.messages (chat_id, sender_id, message_text, time_stamp)
    select chat_created.chat_id, user_elena.user_id, 'Всем Привееет!?', 1743512400000
    from chat_created cross join user_elena
    returning id as message_id
  ),
  message1_status_elena as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message1_insert.message_id, user_elena.user_id, 'READ', false
    from message1_insert, user_elena
    returning message_id
  ),
  message1_status_alex as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message1_insert.message_id, user_alex.user_id, 'READ', false
    from message1_insert, user_alex
    returning message_id
  ),
  message1_status_sergei as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message1_insert.message_id, user_sergei.user_id, 'READ', false
    from message1_insert, user_sergei
    returning message_id
  ),
  message1_status_mariya as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message1_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    from message1_insert, user_mariya
    returning message_id
  ),
  message1_status_yakov as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message1_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    from message1_insert, user_yakov
    returning message_id
  ),

  message2_insert as (
    insert into public.messages (chat_id, sender_id, message_text, time_stamp)
    select chat_created.chat_id, user_alex.user_id, 'И тебе не хворать!?', 1743512520000
    from chat_created cross join user_alex
    returning id as message_id
  ),
  message2_status_elena as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message2_insert.message_id, user_elena.user_id, 'READ', false
    from message2_insert, user_elena
    returning message_id
  ),
  message2_status_alex as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message2_insert.message_id, user_alex.user_id, 'READ', false
    from message2_insert, user_alex
    returning message_id
  ),
  message2_status_sergei as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message2_insert.message_id, user_sergei.user_id, 'READ', false
    from message2_insert, user_sergei
    returning message_id
  ),
  message2_status_mariya as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message2_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    from message2_insert, user_mariya
    returning message_id
  ),
  message2_status_yakov as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message2_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    from message2_insert, user_yakov
    returning message_id
  ),

  message3_insert as (
    insert into public.messages (chat_id, sender_id, message_text, time_stamp)
    select chat_created.chat_id, user_sergei.user_id, 'Всем здрассьте.', 1743513015000
    from chat_created cross join user_sergei
    returning id as message_id
  ),
  message3_status_elena as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message3_insert.message_id, user_elena.user_id, 'READ', false
    from message3_insert, user_elena
    returning message_id
  ),
  message3_status_alex as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message3_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    from message3_insert, user_alex
    returning message_id
  ),
  message3_status_sergei as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message3_insert.message_id, user_sergei.user_id, 'READ', false
    from message3_insert, user_sergei
    returning message_id
  ),
  message3_status_mariya as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message3_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    from message3_insert, user_mariya
    returning message_id
  ),
  message3_status_yakov as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message3_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    from message3_insert, user_yakov
    returning message_id
  ),

  message4_insert as (
    insert into public.messages (chat_id, sender_id, message_text, time_stamp)
    select chat_created.chat_id, user_elena.user_id, 'Куда идем?', 1743513129000
    from chat_created cross join user_elena
    returning id as message_id
  ),
  message4_status_elena as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message4_insert.message_id, user_elena.user_id, 'READ', false
    from message4_insert, user_elena
    returning message_id
  ),
  message4_status_alex as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message4_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    from message4_insert, user_alex
    returning message_id
  ),
  message4_status_sergei as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message4_insert.message_id, user_sergei.user_id, 'READ', false
    from message4_insert, user_sergei
    returning message_id
  ),
  message4_status_mariya as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message4_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    from message4_insert, user_mariya
    returning message_id
  ),
  message4_status_yakov as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message4_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    from message4_insert, user_yakov
    returning message_id
  ),

  message5_insert as (
    insert into public.messages (chat_id, sender_id, message_text, time_stamp)
    select chat_created.chat_id, user_sergei.user_id, 'В кино!', 1743514380000
    from chat_created cross join user_sergei
    returning id as message_id
  ),
  message5_status_elena as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message5_insert.message_id, user_elena.user_id, 'NOT DELIVERED', false
    from message5_insert, user_elena
    returning message_id
  ),
  message5_status_alex as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message5_insert.message_id, user_alex.user_id, 'NOT DELIVERED', false
    from message5_insert, user_alex
    returning message_id
  ),
  message5_status_sergei as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message5_insert.message_id, user_sergei.user_id, 'READ', false
    from message5_insert, user_sergei
    returning message_id
  ),
  message5_status_mariya as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message5_insert.message_id, user_mariya.user_id, 'NOT DELIVERED', false
    from message5_insert, user_mariya
    returning message_id
  ),
  message5_status_yakov as (
    insert into public.message_status (message_id, user_id, status, status_deleted)
    select message5_insert.message_id, user_yakov.user_id, 'NOT DELIVERED', false
    from message5_insert, user_yakov
    returning message_id
  ),

  participants_insert as (
    insert into public.participants (chat_id, user_id, last_read_message_id, deleted_from_chat)
    select chat_created.chat_id, user_elena.user_id, message4_insert.message_id, false
    from chat_created, user_elena, message4_insert
    union all
    select chat_created.chat_id, user_alex.user_id, message2_insert.message_id, false
    from chat_created, user_alex, message2_insert
    union all
    select chat_created.chat_id, user_sergei.user_id, message5_insert.message_id, false
    from chat_created, user_sergei, message5_insert
    union all
    select chat_created.chat_id, user_mariya.user_id, null, false
    from chat_created, user_mariya
    union all
    select chat_created.chat_id, user_yakov.user_id, null, false
    from chat_created, user_yakov
  )
select 1;
)";

  result.insert({1, t1});
  return result;
}
