#include "0_menu_system.h"
#include "client/client_session.h"
#include "core/exception/network_exception.h"
#include "core/exception/validation_exception.h"
#include "core/system/system_function.h"
#include "core/user/user_chat_list.h"
#include <cstdint>
#include <iostream>
#include <memory>

bool CreateAndSendNewMessage(ClientSession &clientSession, std::shared_ptr<Chat> &chat_ptr) {
  bool result = false;

  while (true) {
    try {
      auto newMessage = inputNewMessage(clientSession, chat_ptr);

      if (!newMessage.has_value()) { // the user decided not to type a message
        break;
      } else {

        if (clientSession.createMessageCl(newMessage.value(), chat_ptr, clientSession.getInstance().getActiveUser())) {

          result = true;
        } else
          result = false;
        break;
      }; // else

    } // try
    catch (const exc::ValidationException &ex) {
      std::cout << " ! " << ex.what() << std::endl;
    } catch (const exc::CreateMessageException &ex) {
      std::cout << "Client. CreateAndSendNewChat: " << ex.what() << std::endl;
      result = false;
      ;
    }
  } // while

  return result;
}

void loginMenu_2EditChat(ClientSession &clientSession,
                         std::shared_ptr<Chat> &chat_ptr /*, std::size_t unReadCountIndex*/) {

  std::string userChoice;
  size_t userChoiceNumber;
  bool exit = true;

  while (exit) {

    chat_ptr->printChat(clientSession.getActiveUserCl());

    // The user opened the chat and read everything, so reset the unread count
    const auto &lastMessageId = chat_ptr->getMessages().rbegin()->second->getMessageId();
    const auto &lastReadMessageId = chat_ptr->getLastReadMessageId(clientSession.getInstance().getActiveUser());

    if (lastMessageId != lastReadMessageId) {
      chat_ptr->setLastReadMessageId(clientSession.getActiveUserCl(), lastMessageId);
      std::cout << std::endl;

      clientSession.sendLastReadMessageFromClient(chat_ptr, lastMessageId);
    }

    std::cout << std::endl;
    std::cout << "What should we do? " << std::endl;
    std::cout << "1 - write a message" << std::endl;
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

        if (userChoiceNumber < 1 || userChoiceNumber > 5)
          throw exc::IndexOutOfRangeException(userChoice);

        switch (userChoiceNumber) {
          case 1: {
            CreateAndSendNewMessage(clientSession, chat_ptr);
            std::cout << std::endl;

            //   chat_ptr->printChat(clientSession.getActiveUserCl());
            std::cout << std::endl;

            exit2 = false;
            break; // case 1
          }
          default:
            break; // default
        } // switch
      } catch (const exc::ValidationException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      }

    } // second while
  } // first while
}

/**
 * @brief Displays the list of chats for the active user.
 * @param chatSystem Reference to the chat system.
 * @throws EmptyInputException If input is empty.
 * @throws IndexOutOfRangeException If input is not a valid chat index or 0.
 * @throws ChatNotFoundException If the selected chat cannot be accessed.
 * @details Shows the user's chat list and allows selection of a chat to edit or
 * view.
 */
void loginMenu_2ChatList(ClientSession &clientSession) { // show the chat list

  auto chatCount = clientSession.getActiveUserCl()->getUserChatList()->getChatFromList().size();

  std::cout << std::endl;

  if (clientSession.getActiveUserCl()->getUserChatList()->getChatFromList().empty()) {
    std::cout << "The user does not have any chats yet" << std::endl;
    return;
  } else {
    clientSession.getActiveUserCl()->printChatList(clientSession.getActiveUserCl());
    std::cout << std::endl;
  }

  std::string userChoice;
  bool exit2 = true;

  while (true) {

    std::cout << "Select a menu item: " << std::endl;
    std::cout << "From 1 to " << chatCount << " - Enter the chat number to open it (total " << chatCount
              << " chat(s)): " << std::endl;
    std::cout << "f - Search across chats. Under construction." << std::endl;
    std::cout << "0 - Return to the previous menu" << std::endl;

    while (exit2) {
      std::getline(std::cin, userChoice);

      try {

        if (userChoice.empty())
          throw exc::EmptyInputException();

        if (userChoice == "0")
          return;

        if (userChoice == "f") {
          std::cout << "f - Search across chats. Under construction." << std::endl;
          exit2 = false;
          continue;
        }

        // Enter a specific chat
        int userChoiceNumber = parseGetlineToInt(userChoice);

        if (userChoiceNumber <= 0 || userChoiceNumber > static_cast<int>(chatCount))
          throw exc::IndexOutOfRangeException(userChoice);

        // Here we extract the unread-message count from the vector
        // for on-screen display
        auto chatList = clientSession.getActiveUserCl()
                          ->getUserChatList()
                          ->getChatFromList(); // weak_ptr to the vector

        auto activeChat_weak = chatList[userChoiceNumber - 1];
        auto activeChat_ptr = activeChat_weak.lock();

        if (!activeChat_ptr)
          throw exc::ChatNotFoundException();
        else
          loginMenu_2EditChat(clientSession, activeChat_ptr);
        return;
      } catch (const exc::NonDigitalCharacterException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      } catch (const exc::IndexOutOfRangeException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      } catch (const exc::ChatNotFoundException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      } catch (const exc::EmptyInputException &ex) {
        std::cout << " ! " << ex.what() << " Try again." << std::endl;
        continue;
      };
    } // second while

  } // first while
}
