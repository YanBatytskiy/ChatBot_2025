#pragma once
#include "client/client_session.h"

// доделвть - добавить везде unordered map для процедур логина и поиска юзера по
// логину
// unordered_map<std::string, std::shared_ptr<User>> в ChatSystem

/**
 * @brief Prompts and validates a new user login.
 * @param chatSystem Reference to the chat system for uniqueness checks.
 */
std::string inputNewLogin( ClientSession &clientSession);

/**
 * @brief Prompts and validates a new user password.
 */
std::string inputNewPassword();

/**
 * @brief Prompts and validates a new user display name.
 * @param chatSystem Reference to the chat system.
 */
std::string inputNewName();

void userCreation(ClientSession &clientSession);

