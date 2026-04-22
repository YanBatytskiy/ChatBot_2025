set local search_path = public;

create table if not exists public.chats (
    id bigserial primary key
);

create table if not exists public.users (
    id bigserial primary key,
    login varchar(30) not null unique,
    name varchar(30) not null,
    email varchar(100),
    phone varchar(100)
);

create table if not exists public.messages (
    id bigserial not null,
    chat_id bigint not null references public.chats(id) on delete cascade,
    sender_id bigint not null references public.users(id) on delete restrict,
    message_text text not null,
    time_stamp bigint not null,
    constraint messages_pk primary key (chat_id, id),
    constraint messages_id_uq unique (id)
);

create table if not exists public.message_status (
    message_id bigint not null references public.messages(id) on delete cascade,
    user_id bigint not null references public.users(id) on delete cascade,
    status varchar(20) not null check (status in ('NOT DELIVERED', 'DELIVERED', 'READ')),
    status_deleted boolean not null default false,
    constraint message_status_pk primary key (message_id, user_id)
);

create table if not exists public.users_passhash (
    user_id bigint primary key references public.users(id) on delete cascade,
    password_hash varchar(255)
);

create table if not exists public.participants (
    chat_id bigint not null references public.chats(id) on delete cascade,
    user_id bigint not null references public.users(id) on delete cascade,
    last_read_message_id bigint null,
    deleted_from_chat boolean not null default false,
    constraint participants_pk primary key (chat_id, user_id),
    constraint participants_last_read_fk foreign key (chat_id, last_read_message_id)
        references public.messages(chat_id, id)
);

create or replace function public.new_user_create()
returns trigger
language plpgsql
as $function$
begin
    insert into public.users_passhash (user_id, password_hash)
    values (new.id, null);
    return new;
end;
$function$;

create trigger trg_new_user_create
after insert on public.users
for each row
execute function public.new_user_create();

insert into public.users (login, name, email, phone) values ('a', 'Alex', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('e', 'Elena', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('s', 'Sergei', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('v', 'Vitaliy', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('m', 'Mariya', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('f', 'Fedor', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('ver', 'Vera', '...@gmail.com','111');
insert into public.users (login, name, email, phone) values ('y', 'Yakov', '...@gmail.com','111');

update public.users_passhash
set password_hash = '6b86b273ff34fce19d6b804eff5a3f5747ada4eaa22f1d49c01e52ddb7875b4b';