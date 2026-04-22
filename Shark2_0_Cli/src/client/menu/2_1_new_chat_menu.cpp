#include "2_1_new_chat_menu.h"
#include "0_menu_system.h"
#include "chat/chat.h"
#include "client/client_session.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "exception/validation_exception.h"
#include "system/system_function.h"
#include "user/user_chat_list.h"
#include <cctype>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>

UserDTO chooseOneParticipant(ClientSession &clientSession, std::shared_ptr<Chat> &chat_ptr) {

  std::string inputData;
  int userChoiceNumber;
  std::vector<UserDTO> userListDTO;
  UserDTO userDTOresult{"", "", "", "", ""};

  bool exit = true;
  while (exit) {
    try {
      // получаем список получателей через ввод списка
      std::cout << "Введите часть имени или логина для поиска большими или "
                   "маленькими буквами или 0 для выхода: "
                << std::endl;

      getline(std::cin, inputData);

      if (inputData.empty())
        throw exc::EmptyInputException();

      if (inputData == "0") {
        userDTOresult.passwordhash = "0";
        return userDTOresult;
      }

      // проверяем на наличие недопустимых символов

      // проверяем только на англ буквы и цифры
      if (!engAndFiguresCheck(inputData))
        throw exc::InvalidCharacterException("");

      // найти пользователей
      userListDTO = clientSession.findUserByTextPartOnServerCl(inputData);

      if (userListDTO.size() == 0)
        throw exc::UserNotFoundException();

      // выводим найденных пользователей на экран
      int index = 1;
      std::cout << "Вот, кого мы нашли (Имя : Логин), выберите одного:" << std::endl;
      for (const auto &userDTO : userListDTO) {
        std::cout << index << ". " << userDTO.userName << " : " << userDTO.login << std::endl;
        ++index;
      }
      --index;

      // выбрать нужного из списка
      bool exit2 = true;

      while (exit2) {
        // выбираем пользователя
        std::cout << "Введите номер пользователя 0 для выхода: " << std::endl;

        getline(std::cin, inputData);

        if (inputData.empty())
          throw exc::EmptyInputException();

        if (inputData == "0") {
          userDTOresult.passwordhash = "0";
          exit = false;
          break;
        }

        userChoiceNumber = parseGetlineToInt(inputData);

        if (userChoiceNumber > index || userChoiceNumber < 1)
          throw exc::IndexOutOfRangeException(inputData);

        userDTOresult = userListDTO[userChoiceNumber - 1];

        exit2 = false;
        break;
      } // second while exit2
    } // try
    catch (const exc::InvalidCharacterException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    } catch (const exc::UserNotFoundException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    } catch (const exc::EmptyInputException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    } catch (const exc::IndexOutOfRangeException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
    exit = false;
  } // first while exit
  return userDTOresult;
}

bool LoginMenu_1NewChatChooseParticipants(
    ClientSession &clientSession, std::shared_ptr<Chat> &chat,
    MessageTarget target) { // создение нового сообщения путем выбора пользователей

  switch (target) {

  case MessageTarget::One: {
    // сообщение только одному пользователю

    const auto userDTO = chooseOneParticipant(clientSession, chat);

    if (userDTO.passwordhash == "0")
      return false;

    // добавляем отправителя в вектор участников
    // добавляем отправителя
    chat->addParticipant(clientSession.getActiveUserCl(), 0, false);

    // проверяем есть ли получатель уже в системе
    auto user_ptr = clientSession.getInstance().findUserByLogin(userDTO.login);

    if (user_ptr == nullptr) {

      user_ptr = std::make_shared<User>(
          UserData(userDTO.login, userDTO.userName, userDTO.passwordhash, userDTO.email, userDTO.phone));

      clientSession.getInstance().addUserToSystem(user_ptr);
    }
    chat->addParticipant(user_ptr, 0, false);

    // проверки
    std::cout << "Участники чата: " << std::endl;
    for (const auto &user : chat->getParticipants()) {
      auto participant_ptr = user._user.lock();
      if (participant_ptr) {
        std::cout << participant_ptr->getLogin() << " ака " << participant_ptr->getUserName() << std::endl;
      }
    }
    break;
  } // case One

  case MessageTarget::Several: {

    // сообщение только некоторым пользователям

    bool exit1 = true;
    std::unordered_map<std::string, UserDTO> participants;

    // этот цикл по очереди ищет пользователей и добавляет в вектор получателей логинов
    // while 1
    while (exit1) {
      const auto userDTO = chooseOneParticipant(clientSession, chat);

      if (userDTO.passwordhash == "0" && participants.size() == 0)
        return false;
      else if (userDTO.passwordhash == "0")
        exit1 = false;
      else 	if (!participants.count(userDTO.login))
        participants.insert({userDTO.login, userDTO});

    } // while 1

    // заполняем вектор участников
    for (const auto &participant : participants) {

      // проверяем есть ли получатель уже в системе

      auto user_ptr = clientSession.getInstance().findUserByLogin(participant.first);
      if (user_ptr == nullptr) {

        user_ptr = std::make_shared<User>(UserData(participant.second.login, participant.second.userName,
                                                   participant.second.passwordhash, participant.second.email,
                                                   participant.second.phone));

        clientSession.getInstance().addUserToSystem(user_ptr);
      }

      chat->addParticipant(user_ptr, 0, 0);
    }

    // проверки

    std::cout << "Участники чата: " << std::endl;
    for (const auto &user : chat->getParticipants()) {
      auto user_ptr = user._user.lock();
      std::cout << user_ptr->getLogin() << " ака " << user_ptr->getUserName() << std::endl;
    }
    break;
  } // case Severall
  default:
    break;
  } // switch
  return true;
}

/**
 * @brief Creates and sends a message to a new chat.
 * @param clientSession Reference to the chat system.
 * @param chat shared pointer to the new chat.
 * @param activeUserIndex Index of the active user.
 * @param target Target type for the message (e.g., individual, several, or
 * all).
 * @throws BadWeakException If a weak_ptr cannot be locked.
 * @throws ValidationException If message input fails validation.
 * @details Manages participant selection, message input, and chat integration
 * into the system.
 */
bool CreateAndSendNewChat(ClientSession &clientSession, MessageTarget target) {

  bool result = true;

  // создали новый чат
  auto chat_ptr = std::make_shared<Chat>();
  auto newChatId = clientSession.getInstance().createNewChatId(chat_ptr);
  chat_ptr->setChatId(newChatId);

  // передаем ссылку на чат и там сразу заполняем данные
  if (LoginMenu_1NewChatChooseParticipants(clientSession, chat_ptr, target)) {

    // создаем сообщение
    std::cout << std::endl << "Вот твой чат. В нем всего 0 сообщения(ий). " << std::endl;

    bool exitCase2 = true;


    while (exitCase2) {
      try {
        auto newMessage = inputNewMessage(clientSession, chat_ptr);

        if (!newMessage.has_value()) { // пользователь решмил не вводить сообщение
          exitCase2 = false;
        } else {

          chat_ptr->addMessageToChat(std::make_shared<Message>(newMessage.value()), clientSession.getActiveUserCl(),
                                     false);

          if (!clientSession.createNewChatCl(chat_ptr))
            throw exc::CreateChatException();

          // проверки
          chat_ptr->printChat(clientSession.getActiveUserCl());
          std::cout << std::endl;
          exitCase2 = false;
        }; // else
      } // try
      catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << std::endl;
      } catch (const exc::CreateChatException &ex) {
        std::cout << "Клиент. CreateAndSendNewChat: " << ex.what() << std::endl;
        result = false;
        ;

      } catch (const exc::CreateMessageException &ex) {
        std::cout << "Клиент. CreateAndSendNewChat: " << ex.what() << std::endl;
        result = false;
        ;
      }
    } // while case 2
  } // if LoginMenu_1NewChatChooseParticipants
  else
    result = false;
  if (!result) {
    chat_ptr->clearChat();
    clientSession.getInstance().moveToFreeChatIdSrv(newChatId);
    return false;
  } else
    return true;
}

/**
 * @brief Initiates the creation of a new chat.
 * @param clientSession Reference to the chat system.
 * @throws EmptyInputException If input is empty.
 * @throws IndexOutOfRangeException If input is not 0, 1, 2, or 3.
 * @details Provides a menu for selecting the type of new chat (one user,
 * several users, or all users).
 */
void LoginMenu_1NewChat(ClientSession &clientSession) { // создание нового сообщения

  std::string userChoice;
  size_t userChoiceNumber;
  bool exit = true;

  while (exit) {
    std::cout << "Хотите: " << std::endl
              << "1. Найти пользователя и отправить ему сообщение" << std::endl
              << "2. Выбрать несколько пользователей и отправить им сообщение" << std::endl
              << "0. Для выхода в предыдущее меню" << std::endl;

    std::getline(std::cin, userChoice);

    try {

      if (userChoice.empty())
        throw exc::EmptyInputException();

      if (userChoice == "0")
        return;

      userChoiceNumber = parseGetlineToInt(userChoice);

      if (userChoiceNumber != 1 && userChoiceNumber != 2 && userChoiceNumber != 3)
        throw exc::IndexOutOfRangeException(userChoice);

      switch (userChoiceNumber) {

      case 1: { // 1. Найти пользователя и отправить ему сообщение
        CreateAndSendNewChat(clientSession, MessageTarget::One);

        exit = false; // выход в верхнее меню так как новый чат уже не новый
        break;        // case 1
      }
      case 2: { // 2. Вывести список пользователей и отправить нескольким
                // пользователям

        CreateAndSendNewChat(clientSession, MessageTarget::Several);

        exit = false; // выход в верхнее меню так как новый чат уже не новый
        break;        // case 2
      }
      default:
        break; // default
      } // switch
    } // try
    catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Попробуйте еще раз." << std::endl;
      continue;
    }
  } // while
}
