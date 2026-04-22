#include "2_4_user_profile.h"
#include "1_1_creation.h"
#include "chat_system/chat_system.h"
#include "client/client_session.h"
#include "exception/validation_exception.h"
#include "system/system_function.h"
#include <iostream>

/**
 * @brief Changes the username of the active user.
 * @param clientSession Reference to the chat system.
 * @details Prompts for a new username, validates it, and updates the user's
 * name.
 */
void userNameChange(ClientSession &clientSession) { // смена имени пользователя

  UserData userData;

  std::string newName = inputNewName();
  if (newName.empty())
    return;

  clientSession.getActiveUserCl()->setUserName(newName);

  std::cout << "Имя изменено. Логин  = " << clientSession.getActiveUserCl()->getLogin()
            << " и Имя = " << clientSession.getActiveUserCl()->getUserName() << std::endl;
}

/**
 * @brief Changes the password of the active user.
 * @param clientSession Reference to the chat system.
 * @details Prompts for a new password, validates it, and updates the user's
 * password.
 */
void userPasswordChange(ClientSession &clientSession) { // смена пароля пользователя

  UserData userData;

  std::string newPassword = inputNewPassword();
  if (newPassword.empty())
    return;

  clientSession.getActiveUserCl()->setPassword(newPassword);

  std::cout << "Пароль изменен. Логин = " << clientSession.getActiveUserCl()->getLogin()
            << " и Имя = " << clientSession.getActiveUserCl()->getUserName()
            << " и Пароль = " << clientSession.getActiveUserCl()->getPasswordHash() << std::endl;
}

/**
 * @brief Deletes all chats associated with the user (under construction).
 * @param clientSession Reference to the chat system.
 * @details Placeholder for functionality to remove all user chats, with
 * confirmation prompt (commented out).
 */
void userChatDeleteAll(ClientSession &clientSession) {

  //   std::cout << "Вы уверены, что надо удалить у Вас все чаты? (1 - да; 0 -
  //   нет)";

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

  //       // удалить чат из пользователя

  //       // проверить - если это был последний активнный пользователь чата -
  //       удалить сам

  //     } // try
  //     catch (const exc::ValidationException &ex) {
  //       std::cout << " ! " << ex.what() << " Попробуйте еще раз." <<
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
    std::cout << "Добрый день, пользователь " << clientSession.getActiveUserCl()->getUserName() << std::endl;
    std::cout << std::endl;
    std::cout << "Выберите пункт меню: " << std::endl;
    std::cout << "1 - Сменить имя пользователя (не логин) - временно отключено" << std::endl;
    std::cout << "2 - Сменить пароль  - временно отключено" << std::endl;
    std::cout << "0 - Выйти в предыдущее меню" << std::endl;

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
          std::cout << "1 - Сменить имя пользователя (не логин) - временно отключено" << std::endl;
          //   userNameChange(clientSession); // 2 - Сменить пароль
          exit2 = false;
          break; // case 1 MainMenu
        case 2:
          std::cout << "2 - Сменить пароль  - временно отключено" << std::endl;
          //   userPasswordChange(clientSession);
          exit2 = false;
          break; // case 2 MainMenu
        default:
          break; // default MainMenu
        } // switch

      } // try
      catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
        continue;
      }
    } // second while
  }
}
