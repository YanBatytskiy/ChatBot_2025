#pragma once

class ClientSession;

/**
 * @brief Changes the username of the active user.
 * @param chatSystem Reference to the chat system.
 * @details Prompts the user to input a new username and updates it in the system.
 */
void userNameChange(ClientSession &clientSession); // change the user's display name

/**
 * @brief Changes the password of the active user.
 * @param chatSystem Reference to the chat system.
 * @details Prompts the user to input a new password and updates it in the system.
 */
void userPasswordChange(ClientSession &clientSession); // change the user's password

/**
 * @brief Displays and manages the user profile menu.
 * @param chatSystem Reference to the chat system.
 * @details Shows the user's profile information and provides options to modify it.
 */
void loginMenu_4UserProfile(ClientSession &clientSession); // user profile
