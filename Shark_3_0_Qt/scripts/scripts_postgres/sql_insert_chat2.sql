участники чата
  user_elena AS (SELECT id FROM public.users WHERE login = 'e'),
  user_alex AS (SELECT id FROM public.users WHERE login = 'a'),
  user_sergei AS (SELECT id FROM public.users WHERE login = 's'),
  user_mariya AS (SELECT id FROM public.users WHERE login = 'm'),
  user_yakov AS (SELECT id FROM public.users WHERE login = 'y'),

message1, sender user_elena
'Всем Привееет!?', 1743512400000
message1 message_status
user_elena = READ
user_alex = NOT DELIVERED
user_sergei = NOT DELIVERED
user_mariya = NOT DELIVERED
user_yakov = NOT DELIVERED

message2, sender user_alex
'И тебе не хворать!?', 1743512520000
message2 message_status
user_elena = NOT DELIVERED
user_alex = READ
user_sergei = NOT DELIVERED
user_mariya = NOT DELIVERED
user_yakov = NOT DELIVERED
message1 message_status
user_alex = READ

message3, sender user_sergei
'Всем здрассьте.', 1743513015000
message3 message_status
user_elena = NOT DELIVERED
user_alex = NOT DELIVERED
user_sergei = READ
user_mariya = NOT DELIVERED
user_yakov = NOT DELIVERED
message1 message_status
user_sergei = READ
message2 message_status
user_sergei = READ

message4, sender user_elena
'Куда идем?', 1743513129000
message4 message_status
user_elena = READ
user_alex = NOT DELIVERED
user_sergei = NOT DELIVERED
user_mariya = NOT DELIVERED
user_yakov = NOT DELIVERED
message2 message_status
user_elena = READ
message3 message_status
user_elena = READ

message5, sender user_sergei
'В кино!', 1743514380000
message5 message_status
user_elena = NOT DELIVERED
user_alex = NOT DELIVERED
user_sergei = READ
user_mariya = NOT DELIVERED
user_yakov = NOT DELIVERED
message3 message_status
user_sergei = READ
message4 message_status
user_sergei = READ

last_read_message_id
user_elena = message4
user_alex = message2
user_sergei = message5
user_mariya = null
user_yakov = null

with
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

