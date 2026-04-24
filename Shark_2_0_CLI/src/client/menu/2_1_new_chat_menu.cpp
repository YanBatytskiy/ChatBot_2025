#include "2_1_new_chat_menu.h"
#include "0_menu_system.h"
#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "client/client_session.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "message/message.h"
#include "system/system_function.h"
#include "user/user.h"
#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>

UserDTO chooseOneParticipant(ClientSession &clientSession, [[maybe_unused]] std::shared_ptr<Chat> &chat_ptr) {

  std::string inputData;
  int userChoiceNumber;
  std::vector<UserDTO> userListDTO;
  UserDTO userDTOresult{"", "", "", "", ""};

  bool exit = true;
  while (exit) {
    try {
      // Get the list of recipients through user input
      std::cout << "Enter part of a name or login in upper or lower case to "
                   "search, or 0 to exit: "
                << std::endl;

      getline(std::cin, inputData);

      if (inputData.empty())
        throw exc::EmptyInputException();

      if (inputData == "0") {
        userDTOresult.passwordhash = "0";
        return userDTOresult;
      }

      // Check for disallowed characters

      // Validate only against English letters and digits
      if (!engAndFiguresCheck(inputData))
        throw exc::InvalidCharacterException("");

      // Find users
      userListDTO = clientSession.findUserByTextPartOnServerCl(inputData);

      if (userListDTO.size() == 0)
        throw exc::UserNotFoundException();

      // Print the found users
      int index = 1;
      std::cout << "Here is what we found (Name : Login), choose one:" << std::endl;
      for (const auto &userDTO : userListDTO) {
        std::cout << index << ". " << userDTO.userName << " : " << userDTO.login << std::endl;
        ++index;
      }
      --index;

      // Pick the desired user from the list
      bool exit2 = true;

      while (exit2) {
        // Pick the user
        std::cout << "Enter the user number, or 0 to exit: " << std::endl;

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
      std::cout << " ! " << ex.what() << " Try again." << std::endl;
      continue;
    } catch (const exc::UserNotFoundException &ex) {
      std::cout << " ! " << ex.what() << " Try again." << std::endl;
      continue;
    } catch (const exc::EmptyInputException &ex) {
      std::cout << " ! " << ex.what() << " Try again." << std::endl;
      continue;
    } catch (const exc::IndexOutOfRangeException &ex) {
      std::cout << " ! " << ex.what() << " Try again." << std::endl;
      continue;
    }
    exit = false;
  } // first while exit
  return userDTOresult;
}

bool LoginMenu_1NewChatChooseParticipants(
  ClientSession &clientSession, std::shared_ptr<Chat> &chat,
  MessageTarget target) { // create a new message by picking users

  switch (target) {

    case MessageTarget::One: {
      // Message to a single user

      const auto userDTO = chooseOneParticipant(clientSession, chat);

      if (userDTO.passwordhash == "0")
        return false;

      // Add the sender to the participant vector
      // Add the sender
      chat->addParticipant(clientSession.getActiveUserCl(), 0, false);

      // Check whether the recipient is already in the system
      auto user_ptr = clientSession.getInstance().findUserByLogin(userDTO.login);

      if (user_ptr == nullptr) {

        user_ptr = std::make_shared<User>(
          UserData(userDTO.login, userDTO.userName, userDTO.passwordhash, userDTO.email, userDTO.phone));

        clientSession.getInstance().addUserToSystem(user_ptr);
      }
      chat->addParticipant(user_ptr, 0, false);

      // debug checks
      std::cout << "Chat participants: " << std::endl;
      for (const auto &user : chat->getParticipants()) {
        auto participant_ptr = user._user.lock();
        if (participant_ptr) {
          std::cout << participant_ptr->getLogin() << " aka " << participant_ptr->getUserName() << std::endl;
        }
      }
      break;
    } // case One

    case MessageTarget::Several: {

      // Message to a specific subset of users

      bool exit1 = true;
      std::unordered_map<std::string, UserDTO> participants;

      // This loop sequentially searches for users and adds them to the recipient login vector
      // while 1
      while (exit1) {
        const auto userDTO = chooseOneParticipant(clientSession, chat);

        if (userDTO.passwordhash == "0" && participants.size() == 0)
          return false;
        else if (userDTO.passwordhash == "0")
          exit1 = false;
        else if (!participants.count(userDTO.login))
          participants.insert({userDTO.login, userDTO});

      } // while 1

      // Fill the participant vector
      for (const auto &participant : participants) {

        // Check whether the recipient is already in the system

        auto user_ptr = clientSession.getInstance().findUserByLogin(participant.first);
        if (user_ptr == nullptr) {

          user_ptr = std::make_shared<User>(UserData(participant.second.login, participant.second.userName,
                                                     participant.second.passwordhash, participant.second.email,
                                                     participant.second.phone));

          clientSession.getInstance().addUserToSystem(user_ptr);
        }

        chat->addParticipant(user_ptr, 0, 0);
      }

      // debug checks

      std::cout << "Chat participants: " << std::endl;
      for (const auto &user : chat->getParticipants()) {
        auto user_ptr = user._user.lock();
        std::cout << user_ptr->getLogin() << " aka " << user_ptr->getUserName() << std::endl;
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

  // Created a new chat
  auto chat_ptr = std::make_shared<Chat>();
  auto newChatId = clientSession.getInstance().createNewChatId(chat_ptr);
  chat_ptr->setChatId(newChatId);

  // Pass a reference to the chat and fill its data inside
  if (LoginMenu_1NewChatChooseParticipants(clientSession, chat_ptr, target)) {

    // Create a message
    std::cout << std::endl
              << "Here is your chat. It currently has 0 messages. " << std::endl;

    bool exitCase2 = true;

    while (exitCase2) {
      try {
        auto newMessage = inputNewMessage(clientSession, chat_ptr);

        if (!newMessage.has_value()) { // the user decided not to type a message
          exitCase2 = false;
        } else {

          chat_ptr->addMessageToChat(std::make_shared<Message>(newMessage.value()), clientSession.getActiveUserCl(),
                                     false);

          if (!clientSession.createNewChatCl(chat_ptr))
            throw exc::CreateChatException();

          // debug checks
          chat_ptr->printChat(clientSession.getActiveUserCl());
          std::cout << std::endl;
          exitCase2 = false;
        }; // else
      } // try
      catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << std::endl;
      } catch (const exc::CreateChatException &ex) {
        std::cout << "Client. CreateAndSendNewChat: " << ex.what() << std::endl;
        result = false;
        ;

      } catch (const exc::CreateMessageException &ex) {
        std::cout << "Client. CreateAndSendNewChat: " << ex.what() << std::endl;
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
void LoginMenu_1NewChat(ClientSession &clientSession) { // create a new message

  std::string userChoice;
  size_t userChoiceNumber;
  bool exit = true;

  while (exit) {
    std::cout << "What would you like to do: " << std::endl
              << "1. Find a user and send them a message" << std::endl
              << "2. Pick several users and send them a message" << std::endl
              << "0. Return to the previous menu" << std::endl;

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

        case 1: { // 1. Find a user and send them a message
          CreateAndSendNewChat(clientSession, MessageTarget::One);

          exit = false; // exit to the upper menu since the new chat is no longer new
          break;        // case 1
        }
        case 2: { // 2. Show the list of users and send a message to several
                  // users

          CreateAndSendNewChat(clientSession, MessageTarget::Several);

          exit = false; // exit to the upper menu since the new chat is no longer new
          break;        // case 2
        }
        default:
          break; // default
      } // switch
    } // try
    catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << " Try again." << std::endl;
      continue;
    }
  } // while
}
