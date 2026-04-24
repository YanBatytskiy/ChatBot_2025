#pragma once
#include "client/client_session.h"

// Design note: login-related flows are still candidates for direct lookup via
// unordered_map<std::string, std::shared_ptr<User>> in ChatSystem.

/**
 * @brief Prompts and validates a new user login.
 * @param chatSystem Reference to the chat system for uniqueness checks.
 */
std::string inputNewLogin(ClientSession &clientSession);

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
