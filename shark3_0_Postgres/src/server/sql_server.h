#pragma once

#include "dto_struct.h"
#include <libpq-fe.h>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

bool checkEmptyBaseSQL(PGconn *conn);

bool clearBaseSQL(PGconn *conn);

bool checkBaseTablesSQL(PGconn *conn);

// sql command
PGresult *execTransactionToSQL(PGconn *conn, std::multimap<int, std::string> &sqlRequests,
                               std::multimap<int, std::string> &sqlDescription);

PGresult *execSQL(PGconn *conn, const std::string &sql);

std::vector<std::string> getChatListSQL(PGconn *conn, const std::string &login);

std::optional<std::vector<ParticipantsDTO>> getChatParticipantsSQL(PGconn *conn, const std::string &chat_id);

std::optional<std::multiset<std::pair<std::string, std::size_t>>> getChatMessagesDeletedStatusSQL(
    PGconn *conn, const std::string &chat_id);

std::optional<MessageChatDTO> getChatMessagesSQL(PGconn *conn, const std::string &chat_id);

std::optional<std::vector<UserDTO>> getUsersByTextPartSQL(PGconn *conn, const UserLoginPasswordDTO& packet);

std::optional<std::vector<UserDTO>> getSeveralUsersDTOFromSrvSQL(PGconn *conn, const std::vector<std::string>& logins);

bool setLastReadMessageSQL(PGconn *conn, const MessageDTO& messageDTO);


bool createUserSQL(PGconn *conn, const UserDTO &userDTO);

std::vector<std::string> createChatAndMessageSQL(PGconn *conn, const ChatDTO &chatDTO,
                                                 const MessageChatDTO &messageChatDTO);

std::size_t createMessageSQL(PGconn *conn, const MessageDTO &messageDTO);