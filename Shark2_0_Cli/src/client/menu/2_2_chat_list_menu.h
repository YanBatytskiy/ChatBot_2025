#pragma once
#include "chat_system/chat_system.h"
#include "client/client_session.h"
#include <cstdint>

bool CreateAndSendNewMessage(ClientSession &clientSession, std::shared_ptr<Chat>& chat_ptr);

/**
 * @brief Displays the list of chats for the active user.
 * @param chatSystem Reference to the chat system.
 * @details Shows available chats and allows the user to interact with them.
 */
void loginMenu_2ChatList(ClientSession &clientSession);

/**
 * @brief Edits or manages a specific chat.
 * @param chatSystem Reference to the chat system.
 * @param chat shared pointer to the chat to be edited.
 * @details Provides options to modify or interact with the specified chat.
 */
void loginMenu_2EditChat(ClientSession &clientSession,
                          std::shared_ptr<Chat> &chat /*, std::size_t unReadCountIndex*/);