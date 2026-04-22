#pragma once
#include "client/client_session.h"

/**
 * @brief Displays the main authorization menu.
 * @return The user's menu selection as a short integer.
 */
short entranceMenu();

/**
 * @brief Handles the login menu flow and user input.
 * @param chatSystem Reference to the chat system.
 */

bool userLoginInsystem(ClientSession &clientSession);

void loginMenuChoice(ClientSession &clientSession);