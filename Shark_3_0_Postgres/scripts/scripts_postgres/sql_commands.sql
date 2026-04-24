1. Удалить все объекты (схемы, таблицы, функции и т.д.) в базе:

DROP SCHEMA public CASCADE;
CREATE SCHEMA public;

2. Удалить все данные во всех таблицах, сохранив их структуру:

TRUNCATE TABLE chats, users, messages, message_status, participants, users_passhash RESTART IDENTITY CASCADE;

TRUNCATE TABLE chats, messages, message_status, participants RESTART IDENTITY CASCADE;
