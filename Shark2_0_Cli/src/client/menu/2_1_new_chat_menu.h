#pragma once
#include "client/client_session.h"
// #include "chat_system/chat_system.h"
#include "system/system_function.h"

UserDTO chooseOneParticipant(ClientSession &clientSession, std::shared_ptr<Chat> &chat_ptr);

bool LoginMenu_1NewChatChooseParticipants(ClientSession &clientSession, std::shared_ptr<Chat> &chat,
                                          MessageTarget target); // создение нового сообщения путем выбора пользователей

/**
 * @brief Creates and sends a message to a new chat.
 * @param chatSystem Reference to the chat system.
 * @param activeUserIndex Index of the active user.
 * @param sendToAll True if the message should be sent to all users.
 * @details Handles the creation of a new chat and sending a message, supporting different sending modes.
 */
bool CreateAndSendNewChat(ClientSession &clientSession,
                          MessageTarget target); // общая функция для отправки сообщения в новый чат тремя способами

/**
 * @brief Initiates the creation of a new chat.
 * @param chatSystem Reference to the chat system.
 * @details Provides the interface for starting the process of creating a new chat.
 */
void LoginMenu_1NewChat(ClientSession &clientSession); // создание нового сообщения
