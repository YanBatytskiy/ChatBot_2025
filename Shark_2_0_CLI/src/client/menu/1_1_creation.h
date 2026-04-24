#pragma once
#include <string>

class ClientSession;

// Potential optimization path:
// keep direct login lookup through unordered_map<std::string, std::shared_ptr<User>>
// in ChatSystem for all registration and login checks.

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
