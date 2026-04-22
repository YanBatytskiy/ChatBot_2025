#include "1_0_entrance_menu.h"
#include "0_menu_system.h"
#include "2_1_new_chat_menu.h"
#include "2_2_chat_list_menu.h"
#include "2_4_user_profile.h"
#include "client/client_session.h"
#include "exception/login_exception.h"
#include "exception/validation_exception.h"
#include "system/picosha2.h"
#include "system/system_function.h"
#include <cctype>
#include <iostream>
#include <ostream>

void showInitForTest() {

  // const std::string initUserPassword[] = {"User01", "User02", "User03",
  // "User04", "User05",
  //                                         "User06", "User07", "User08"};
  const std::string initUserPassword[] = {"1", "1", "1", "1",
                                          "1", "1", "1", "1"};

  // const std::string initUserLogin[] = {"alex1980", "elena1980", "serg1980",
  // "vit1980",
  //                                      "mar1980",  "fed1980",   "vera1980",
  //                                      "yak1980"};

  const std::string initUserLogin[] = {"a", "e", "s",   "v",
                                       "m", "f", "ver", "y"};
  // Создание пользователей
  std::string passwordHash = picosha2::hash256_hex_string(initUserPassword[0]);
  auto Alex2104_ptr = std::make_shared<User>(UserData(
      initUserLogin[0], "Sasha", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[1]);
  auto Elena1510_ptr = std::make_shared<User>(UserData(
      initUserLogin[1], "Elena", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[2]);
  auto Serg0101_ptr = std::make_shared<User>(UserData(
      initUserLogin[2], "Sergei", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[3]);
  auto Vit2504_ptr = std::make_shared<User>(UserData(
      initUserLogin[3], "Vitaliy", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[4]);
  auto mar1980_ptr = std::make_shared<User>(UserData(
      initUserLogin[4], "Mariya", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[5]);
  auto fed1980_ptr = std::make_shared<User>(UserData(
      initUserLogin[5], "Fedor", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[6]);
  auto vera1980_ptr = std::make_shared<User>(UserData(
      initUserLogin[6], "Vera", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[7]);
  auto yak1980_ptr = std::make_shared<User>(UserData(
      initUserLogin[7], "Yakov", passwordHash, "...@gmail.com", "+111"));

  Alex2104_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[0] << std::endl;
  Elena1510_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[1] << std::endl;
  Serg0101_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[2] << std::endl;
  Vit2504_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[3] << std::endl;
  mar1980_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[4] << std::endl;
  fed1980_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[5] << std::endl;
  vera1980_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[6] << std::endl;
  yak1980_ptr->showUserDataInit();
  std::cout << ", Password: " << initUserPassword[7] << std::endl;
}

short entranceMenu() { // вывод главного меню
  while (true) {
    printSystemName();

    std::cout << std::endl << std::endl;
    showInitForTest();
    std::cout << std::endl << std::endl;

    std::cout << "ChatBot 'Shark' Версия 2.0. @2025 \n \n" << std::endl;

    std::cout << "Выберите пункт меню: " << ::std::endl
              << "1. Регистрация пользователя " << std::endl
              << "2. Войти в ЧатБот" << std::endl
              << "0. Завершить программу" << std::endl;

    std::string userChoice;
    int userChoiceNumber;

    while (true) {
      std::getline(std::cin, userChoice);
      try {
        if (userChoice.empty())
          throw exc::EmptyInputException();

        if (userChoice == "0")
          return 0;

        userChoiceNumber = parseGetlineToInt(userChoice);

        if (userChoiceNumber != 1 && userChoiceNumber != 2)
          throw exc::IndexOutOfRangeException(userChoice);

        return userChoiceNumber;
      } catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
        continue;
      }
    } // first while
  } // second while
}

//
//
//

bool userLoginInsystem(ClientSession &clientSession) {

  UserData userDataForLogin;
  std::string userLogin = "";
  std::string userPassword = "";

  // Login validation
  while (true) {
    try {
      userLogin = inputDataValidation("Введите логин или 0 для выхода:", 0, 0,
                                      false, false);
      if (userLogin == "0")
        return false;

      if (!clientSession.checkUserLoginCl(userLogin))
        throw exc::UserNotFoundException();
    } catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
    break;
  }

  // Password validation
  while (true) {
    try {
      userPassword = inputDataValidation(
          "Введите пароль (или 0 для выхода):", 0, 0, true, false);
      if (userPassword == "0")
        return false;

      auto passHash = picosha2::hash256_hex_string(userPassword);

      if (!clientSession.checkUserPasswordCl(userLogin, passHash))
        throw exc::IncorrectPasswordException();

      break;

    } catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
  }

  clientSession.registerClientToSystemCl(userLogin);

  return true;
}

//
//
//
void loginMenuChoice(ClientSession &clientSession) { // вывод главного меню
  int userChoiceNumber;
  std::string userChoice;

  while (true) {
    std::cout << std::endl;
    std::cout << "Добрый день, пользователь "
              << clientSession.getActiveUserCl()->getUserName() << std::endl;
    std::cout << std::endl;
    std::cout << "Выберите пункт меню: " << std::endl;
    std::cout << "1 - Создать новый чат" << std::endl;
    std::cout << "2 - Показать список чатов" << std::endl;
    std::cout << "3 - Показать Профиль пользователя" << std::endl;
    std::cout << "0 - Выйти в предыдущее меню" << std::endl;

    bool exit2 = true;
    while (exit2) {
      std::getline(std::cin, userChoice);
      try {
        if (userChoice.empty())
          throw exc::EmptyInputException();

        if (userChoice == "0") {
          clientSession.setActiveUserCl(nullptr);
          clientSession.resetSessionData();

          return;
        }

        userChoiceNumber = parseGetlineToInt(userChoice);

        if (userChoiceNumber < 0 || userChoiceNumber > 4)
          throw exc::IndexOutOfRangeException(userChoice);

        switch (userChoiceNumber) {
        case 1:
          LoginMenu_1NewChat(clientSession);
          exit2 = false;
          continue; // case 1 MainMenu
        case 2:
          loginMenu_2ChatList(clientSession);
          exit2 = false;
          continue; // case 2 MainMenu
        case 3:
          loginMenu_4UserProfile(clientSession);
          exit2 = false;
          continue; // case 4 MainMenu
        default:
          break; // default MainMenu
        } // switch
      } // try
      catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
        continue;
      }
    } // second
  }
}
