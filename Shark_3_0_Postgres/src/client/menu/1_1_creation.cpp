#include "1_1_creation.h"
#include "client/client_session.h"
#include "client/menu/0_menu_system.h"
#include "core/system/picosha2.h"
#include "core/user/user.h"
#include "core/user/user_chat_list.h"
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

    prompt = "Enter a new login or 0 to exit. At least " +
             std::to_string(dataLengthMin) + " characters and no more than " +
             std::to_string(dataLengthMax) +
             ". Latin letters and digits only - ";

    std::string newLogin =
      inputDataValidation(prompt, dataLengthMin, dataLengthMax, false, true);

    if (newLogin == "0") {
      return newLogin;
    }

    if (clientSession.checkUserLoginCl(newLogin)) {
      std::cerr << "Login is already taken.\n";
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

  prompt = "Enter a new password or 0 to exit. At least " +
           std::to_string(dataLengthMin) + " characters and no more than " +
           std::to_string(dataLengthMax) +
           ". Minimum one uppercase letter and one digit (Latin letters only) - ";

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

  prompt = "Enter the desired display name or 0 to exit. At least " +
           std::to_string(dataLengthMin) + " characters and no more than " +
           std::to_string(dataLengthMax) +
           ". Latin letters and digits only - ";

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
  std::cout << "Register a new user." << std::endl;
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
