#include "1_1_creation.h"
#include "client/client_session.h"
#include "client/menu/0_menu_system.h"
#include "system/picosha2.h"
#include "user/user.h"
#include "user/user_chat_list.h"
#include <cctype>
#include <cstddef>
#include <iostream>
#include <string>
#include <cstdint>

std::string inputNewLogin(ClientSession &clientSession) {
  while (true) {
    std::size_t dataLengthMin = 5;
    std::size_t dataLengthMax = 15;
    std::string prompt;

    prompt = "Введите новый Логин либо 0 для выхода. Не менее " +
             std::to_string(dataLengthMin) + " символов и не более " +
             std::to_string(dataLengthMax) +
             ". Только латинские буквы и цифры - ";

    std::string newLogin =
        inputDataValidation(prompt, dataLengthMin, dataLengthMax, false, true);

    if (newLogin == "0") {
      return newLogin;
    }

    if (clientSession.checkUserLoginCl(newLogin)) {
      std::cerr << "Логин уже занят.\n";
      continue;
    }

    return newLogin;
  }
}

/**
 * @brief Prompts and validates a new user password.
 * @param chatSystem Reference to the chat system.
 */
std::string inputNewPassword() {

  std::size_t dataLengthMin = 5;
  std::size_t dataLengthMax = 10;
  std::string prompt;

  prompt = "Введите новый Пароль либо 0 для выхода. Не менее " +
           std::to_string(dataLengthMin) + " символов и не более " +
           std::to_string(dataLengthMax) +
           ". Мнимум одна заглавная и одна цифра (только латинские буквы) - ";

  std::string newPassword =
      inputDataValidation(prompt, dataLengthMin, dataLengthMax, true, true);

  if (newPassword == "0")
    newPassword.clear();

        auto passHash = picosha2::hash256_hex_string(newPassword);

  return passHash;
}

/**
 * @brief Prompts and validates a new user display name.
 * @param chatSystem Reference to the chat system.
 */
std::string inputNewName() {
  std::size_t dataLengthMin = 3;
  std::size_t dataLengthMax = 10;
  std::string prompt;

  prompt = "Введите желаемое Имя для отображениялибо 0 для выхода. Не менее " +
           std::to_string(dataLengthMin) + "символов и не более " +
           std::to_string(dataLengthMax) +
           ". Только латинские буквы и цифры - ";

  std::string newName =
      inputDataValidation(prompt, dataLengthMin, dataLengthMax, false, true);

  if (newName == "0")
    newName.clear();

  return newName;
}

//
//
//
void userCreation(ClientSession &clientSession) {
  std::cout << "Регистрация нового пользователя." << std::endl;
  UserData userData;

  std::string newLogin = inputNewLogin(clientSession);
  if (newLogin.empty() || newLogin == "0")
    return;

  std::string passwordHash = inputNewPassword();
  if (passwordHash.empty())
    return;

  std::string newName = inputNewName();
  if (newName.empty() || newName == "0")
    return;

  auto newUser = std::make_shared<User>(
      UserData(newLogin, newName, passwordHash, "...@gmail.com", "+111"));

  clientSession.createUserCl(newUser);

  newUser->showUserData();
}