#include "2_4_user_profile.h"
#include "1_1_creation.h"
#include "client/client_session.h"
#include "exception/validation_exception.h"
#include "system/system_function.h"
#include "user/user.h"
#include <iostream>

/**
 * @brief Changes the username of the active user.
 * @param clientSession Reference to the chat system.
 * @details Prompts for a new username, validates it, and updates the user's
 * name.
 */
void userNameChange(ClientSession &clientSession) { // change the user's display name

  std::string newName = inputNewName();
  if (newName.empty())
    return;

  clientSession.getActiveUserCl()->setUserName(newName);

  std::cout << "Name changed. Login = " << clientSession.getActiveUserCl()->getLogin()
            << " and Name = " << clientSession.getActiveUserCl()->getUserName() << std::endl;
}

/**
 * @brief Changes the password of the active user.
 * @param clientSession Reference to the chat system.
 * @details Prompts for a new password, validates it, and updates the user's
 * password.
 */
void userPasswordChange(ClientSession &clientSession) { // change the user's password

  std::string newPassword = inputNewPassword();
  if (newPassword.empty())
    return;

  clientSession.getActiveUserCl()->setPassword(newPassword);

  std::cout << "Password changed. Login = " << clientSession.getActiveUserCl()->getLogin()
            << " and Name = " << clientSession.getActiveUserCl()->getUserName()
            << " and Password = " << clientSession.getActiveUserCl()->getPasswordHash() << std::endl;
}

/**
 * @brief Deletes all chats associated with the user (under construction).
 * @param clientSession Reference to the chat system.
 * @details Placeholder for functionality to remove all user chats, with
 * confirmation prompt (commented out).
 */
void userChatDeleteAll([[maybe_unused]] ClientSession &clientSession) {

  //   std::cout << "Are you sure you want to delete all your chats? (1 - yes; 0 -
  //   no)";

  //   std::string userChoice;

  //   while (true) {
  //     std::getline(std::cin, userChoice);
  //     try {

  //       if (userChoice.empty())
  //         throw exc::EmptyInputException();

  //       if (userChoice == "0")
  //         return;

  //       if (userChoice != "1")
  //         throw exc::IndexOutOfRangeException(userChoice);

  //       // remove the chat from the user

  //       // check: if this was the last active user in the chat - remove the
  //       chat itself

  //     } // try
  //     catch (const exc::ValidationException &ex) {
  //       std::cout << " ! " << ex.what() << " Try again." <<
  //       std::endl;
  //     } // catch
  //   } // first while
}

/**
 * @brief Displays and manages the user profile menu.
 * @param clientSession Reference to the chat system.
 * @throws EmptyInputException If input is empty.
 * @throws IndexOutOfRangeException If input is not 0, 1, 2, 3, 4, 5, or 6.
 * @details Shows profile options and handles user actions like changing name or
 * password; some features are under construction.
 */
void loginMenu_4UserProfile(ClientSession &clientSession) {
  int userChoiceNumber;
  std::string userChoice;

  while (true) {
    std::cout << std::endl;
    std::cout << "Good day, user " << clientSession.getActiveUserCl()->getUserName() << std::endl;
    std::cout << std::endl;
    std::cout << "Select a menu item: " << std::endl;
    std::cout << "1 - Change display name (not login) - temporarily disabled" << std::endl;
    std::cout << "2 - Change password - temporarily disabled" << std::endl;
    std::cout << "0 - Return to the previous menu" << std::endl;

    bool exit2 = true;
    while (exit2) {
      std::getline(std::cin, userChoice);
      try {

        if (userChoice.empty())
          throw exc::EmptyInputException();

        if (userChoice == "0")
          return;

        userChoiceNumber = parseGetlineToInt(userChoice);

        if (userChoiceNumber < 0 || userChoiceNumber > 6)
          throw exc::IndexOutOfRangeException(userChoice);

        switch (userChoiceNumber) {
          case 1:
            std::cout << "1 - Change display name (not login) - temporarily disabled" << std::endl;
            //   userNameChange(clientSession); // 2 - Change password
            exit2 = false;
            break; // case 1 MainMenu
          case 2:
            std::cout << "2 - Change password - temporarily disabled" << std::endl;
            //   userPasswordChange(clientSession);
            exit2 = false;
            break; // case 2 MainMenu
          default:
            break; // default MainMenu
        } // switch

      } // try
      catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      }
    } // second while
  }
}
