#pragma once
#include <memory>

class Chat;
class ClientSession;
enum class MessageTarget;
struct UserDTO;

UserDTO chooseOneParticipant(ClientSession &clientSession, std::shared_ptr<Chat> &chat_ptr);

bool LoginMenu_1NewChatChooseParticipants(ClientSession &clientSession, std::shared_ptr<Chat> &chat,
                                          MessageTarget target); // create a new message by picking users

/**
 * @brief Creates and sends a message to a new chat.
 * @param chatSystem Reference to the chat system.
 * @param activeUserIndex Index of the active user.
 * @param sendToAll True if the message should be sent to all users.
 * @details Handles the creation of a new chat and sending a message, supporting different sending modes.
 */
bool CreateAndSendNewChat(ClientSession &clientSession,
                          MessageTarget target); // shared function for sending a message to a new chat in three ways

/**
 * @brief Initiates the creation of a new chat.
 * @param chatSystem Reference to the chat system.
 * @details Provides the interface for starting the process of creating a new chat.
 */
void LoginMenu_1NewChat(ClientSession &clientSession); // create a new message
