#include "0_init_system.h"
#include "chat/chat.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "exception/validation_exception.h"
#include "message/message_content_struct.h"
#include "server_session.h"
#include "system/picosha2.h"
#include "user/user.h"
#include "user/user_chat_list.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <ctime>

/**
 * @brief Updates the last read message index for the sender in a chat.
 * @param user shared pointer to the user (sender).
 * @param chat shared pointer to the chat.
 */
void changeLastReadIndexForSenderInit(const std::shared_ptr<User> &user, const std::shared_ptr<Chat> &chat) {

  chat->setLastReadMessageId(user, chat->getMessages().rbegin()->second->getMessageId());
}

/**
 * @brief Adds a message to a chat using initial data.
 * @param initDataArray Structure containing message data.
 * @param chat shared pointer to the chat.
 * @throws UnknownException If the sender is null.
 */
bool addMessageToChatInit(ServerSession &serverSession, const InitDataArray &initDataArray,
                          std::shared_ptr<Chat> &chat_ptr) {

  std::vector<std::shared_ptr<IMessageContent>> iMessageContent;
  TextContent textContent(initDataArray._messageText);
  MessageContent<TextContent> messageContentText(textContent);
  iMessageContent.push_back(std::make_shared<MessageContent<TextContent>>(messageContentText));

  try {
    if (!initDataArray._sender) {
      throw exc::UserNotFoundException();
    } else {

      //   auto messageId = chat_ptr->createNewMessageId(true);

      Message message(iMessageContent, initDataArray._sender, initDataArray._timeStamp, 0);

      chat_ptr->addMessageToChat(std::make_shared<Message>(message), initDataArray._sender, true);
    };
  } catch (const exc::CreateMessageIdException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    return false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cout << " ! " << ex.what()
              << "Отправитель отсутствует. Сообщение не будет "
                 "создано. addMessageToChatInit"
              << std::endl;
    return false;
  } catch (const exc::ValidationException &ex) {
    std::cout << " ! " << ex.what() << std::endl;
    return false;
  }
  return true;
}

/**
 * @brief make timeStamp for initialization
 *
 */
std::int64_t makeTimeStamp(int year, int month, int day, int hour, int minute, int second) {
  tm tm = {};
  tm.tm_year = year - 1900; // годы от 1900
  tm.tm_mon = month - 1;    // месяцы от 0
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  time_t timeT = mktime(&tm);                // локальное время
  return static_cast<int64_t>(timeT) * 1000; // миллисекунды
}

/**
 * @brief Constructor for InitDataArray.
 * @param messageText The text content of the message.
 * @param timeStamp The timestamp of the message.
 * @param sender shared pointer to the sender user.
 * @param _recipients Vector of shared pointers to recipient users.

 * @details Initializes the InitDataArray struct with message details.
 */
InitDataArray::InitDataArray(std::string messageText, std::int64_t timeStamp, std::shared_ptr<User> sender,
                             std::vector<std::shared_ptr<User>> _recipients, std::size_t messageId)
    : _messageText(messageText), _timeStamp(timeStamp), _sender(sender), _recipients(_recipients),
      _messageId(messageId) {}

/**
 * @brief Initializes the chat system with test data.
 * @param _instanceSession Reference to the ChatSystem object to be initialized.
 * @details Creates users, adds them to the system, creates chat lists, and sets
 * up two chats (one private, one group) with sample messages.
 */

// const std::string initUserPassword[] = {"User01", "User02", "User03",
// "User04", "User05",
//                                         "User06", "User07", "User08"};
const std::string initUserPassword[] = {"1", "1", "1", "1", "1", "1", "1", "1"};

// const std::string initUserLogin[] = {"alex1980", "elena1980", "serg1980",
// "vit1980",
//                                      "mar1980",  "fed1980",   "vera1980",
//                                      "yak1980"};

const std::string initUserLogin[] = {"a", "e", "s", "v", "m", "f", "ver", "y"};

void systemInitForTest(ServerSession &serverSession) {
  // Создание пользователей
  std::string passwordHash = picosha2::hash256_hex_string(initUserPassword[0]);
  auto Alex2104_ptr = std::make_shared<User>(
      UserData(initUserLogin[0], "Sasha", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[1]);
  auto Elena1510_ptr = std::make_shared<User>(
      UserData(initUserLogin[1], "Elena", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[2]);
  auto Serg0101_ptr = std::make_shared<User>(
      UserData(initUserLogin[2], "Sergei", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[3]);
  auto Vit2504_ptr = std::make_shared<User>(
      UserData(initUserLogin[3], "Vitaliy", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[4]);
  auto mar1980_ptr = std::make_shared<User>(
      UserData(initUserLogin[4], "Mariya", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[5]);
  auto fed1980_ptr = std::make_shared<User>(UserData(initUserLogin[5], "Fedor", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[6]);
  auto vera1980_ptr = std::make_shared<User>(UserData(initUserLogin[6], "Vera", passwordHash, "...@gmail.com", "+111"));
  passwordHash = picosha2::hash256_hex_string(initUserPassword[7]);
  auto yak1980_ptr = std::make_shared<User>(UserData(initUserLogin[7], "Yakov", passwordHash, "...@gmail.com", "+111"));

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

  // Добавление пользователей в систему
  serverSession.getInstance().addUserToSystem(Alex2104_ptr);
  serverSession.getInstance().addUserToSystem(Elena1510_ptr);
  serverSession.getInstance().addUserToSystem(Serg0101_ptr);
  serverSession.getInstance().addUserToSystem(Vit2504_ptr);
  serverSession.getInstance().addUserToSystem(mar1980_ptr);
  serverSession.getInstance().addUserToSystem(fed1980_ptr);
  serverSession.getInstance().addUserToSystem(vera1980_ptr);
  serverSession.getInstance().addUserToSystem(yak1980_ptr);

  // Создание списков чатов для пользователей
  auto Alex2104_ChatList_ptr = std::make_shared<UserChatList>(Alex2104_ptr);
  auto Elena1510_ChatList_ptr = std::make_shared<UserChatList>(Elena1510_ptr);
  auto Serg0101_ChatList_ptr = std::make_shared<UserChatList>(Serg0101_ptr);
  auto Vit2504_ChatList_ptr = std::make_shared<UserChatList>(Vit2504_ptr);
  auto mar1980_ChatList_ptr = std::make_shared<UserChatList>(mar1980_ptr);
  auto fed1980_ChatList_ptr = std::make_shared<UserChatList>(fed1980_ptr);
  auto vera1980_ChatList_ptr = std::make_shared<UserChatList>(vera1980_ptr);
  auto yak1980_ChatList_ptr = std::make_shared<UserChatList>(yak1980_ptr);

  Alex2104_ptr->createChatList(Alex2104_ChatList_ptr);
  Elena1510_ptr->createChatList(Elena1510_ChatList_ptr);
  Serg0101_ptr->createChatList(Serg0101_ChatList_ptr);
  Vit2504_ptr->createChatList(Vit2504_ChatList_ptr);
  mar1980_ptr->createChatList(mar1980_ChatList_ptr);
  fed1980_ptr->createChatList(fed1980_ChatList_ptr);
  vera1980_ptr->createChatList(vera1980_ChatList_ptr);
  yak1980_ptr->createChatList(yak1980_ChatList_ptr);

  // Создание первого чата: Sasha и Elena (один на один)
  std::vector<std::shared_ptr<User>> recipients;
  std::vector<std::weak_ptr<User>> participants;

  recipients.push_back(Alex2104_ptr);
  participants.push_back(Elena1510_ptr);
  participants.push_back(Alex2104_ptr);

  auto chat_ptr = std::make_shared<Chat>();

  chat_ptr->addParticipant(Elena1510_ptr, 0, false);
  chat_ptr->setLastReadMessageId(Elena1510_ptr, 0);
  chat_ptr->addParticipant(Alex2104_ptr, 0, false);
  chat_ptr->setLastReadMessageId(Alex2104_ptr, 0);

  auto chatId = serverSession.getInstance().createNewChatId(chat_ptr);
  chat_ptr->setChatId(chatId);

  serverSession.getInstance().addChatToInstance(chat_ptr);

  int64_t ts = makeTimeStamp(2025, 4, 01, 12, 0, 0);

  InitDataArray Elena_Alex1("Привет", ts, Elena1510_ptr, recipients, 0);

  addMessageToChatInit(serverSession, Elena_Alex1, chat_ptr);

  recipients.clear();
  recipients.push_back(Elena1510_ptr);

  ts = makeTimeStamp(2025, 4, 01, 12, 05, 0);

  InitDataArray Elena_Alex2("Хай! как делишки?", ts, Alex2104_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex2, chat_ptr);

  recipients.clear();
  recipients.push_back(Alex2104_ptr);

  ts = makeTimeStamp(2025, 4, 01, 12, 07, 0);

  InitDataArray Elena_Alex3("Хорошо, как насчет кофе?", ts, Elena1510_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex3, chat_ptr);

  //   changeLastReadIndexForSenderInit(Elena1510_ptr, chat_ptr);

  // проверки
  chat_ptr->printChat(Elena1510_ptr);

  // Создание второго чата: Elena, Sasha и Сергей (групповой чат)
  chat_ptr.reset();
  recipients.clear();
  participants.clear();

  recipients.push_back(Alex2104_ptr);
  recipients.push_back(Serg0101_ptr);
  recipients.push_back(mar1980_ptr);
  recipients.push_back(yak1980_ptr);
  participants.push_back(Elena1510_ptr);
  participants.push_back(Alex2104_ptr);
  participants.push_back(Serg0101_ptr);
  participants.push_back(mar1980_ptr);
  participants.push_back(yak1980_ptr);

  chat_ptr = std::make_shared<Chat>();
  chat_ptr->addParticipant(Elena1510_ptr, 0, false);
  chat_ptr->setLastReadMessageId(Elena1510_ptr, 0);
  chat_ptr->addParticipant(Alex2104_ptr, 0, false);
  chat_ptr->setLastReadMessageId(Alex2104_ptr, 0);
  chat_ptr->addParticipant(Serg0101_ptr, 0, false);
  chat_ptr->setLastReadMessageId(Serg0101_ptr, 0);
  chat_ptr->addParticipant(mar1980_ptr, 0, false);
  chat_ptr->setLastReadMessageId(mar1980_ptr, 0);
  chat_ptr->addParticipant(yak1980_ptr, 0, false);
  chat_ptr->setLastReadMessageId(yak1980_ptr, 0);

  chatId = serverSession.getInstance().createNewChatId(chat_ptr);
  chat_ptr->setChatId(chatId);

  serverSession.getInstance().addChatToInstance(chat_ptr);

  ts = makeTimeStamp(2025, 4, 01, 13, 0, 0);

  InitDataArray Elena_Alex_Serg1("Всем Привееет!?", ts, Elena1510_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex_Serg1, chat_ptr);

  recipients.clear();
  recipients.push_back(Elena1510_ptr);
  recipients.push_back(Serg0101_ptr);
  recipients.push_back(mar1980_ptr);
  recipients.push_back(yak1980_ptr);

  ts = makeTimeStamp(2025, 4, 01, 13, 02, 0);

  InitDataArray Elena_Alex_Serg2("И тебе не хворать!?", ts, Alex2104_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex_Serg2, chat_ptr);

  recipients.clear();
  recipients.push_back(Elena1510_ptr);
  recipients.push_back(Alex2104_ptr);
  recipients.push_back(mar1980_ptr);
  recipients.push_back(yak1980_ptr);

  ts = makeTimeStamp(2025, 4, 01, 13, 10, 15);

  InitDataArray Elena_Alex_Serg3("Всем здрассьте.", ts, Serg0101_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex_Serg3, chat_ptr);

  recipients.clear();
  recipients.push_back(Serg0101_ptr);
  recipients.push_back(Alex2104_ptr);
  recipients.push_back(mar1980_ptr);
  recipients.push_back(yak1980_ptr);

  ts = makeTimeStamp(2025, 4, 01, 13, 12, 9);

  InitDataArray Elena_Alex_Serg4("Куда идем?", ts, Elena1510_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex_Serg4, chat_ptr);

  recipients.clear();
  recipients.push_back(Elena1510_ptr);
  recipients.push_back(Alex2104_ptr);
  recipients.push_back(mar1980_ptr);
  recipients.push_back(yak1980_ptr);

  ts = makeTimeStamp(2025, 4, 01, 13, 33, 0);

  InitDataArray Elena_Alex_Serg5("В кино!", ts, Serg0101_ptr, recipients, 0);
  addMessageToChatInit(serverSession, Elena_Alex_Serg5, chat_ptr);

  //   chat_ptr->setLastReadMessageId(Elena1510_ptr,
  //   chat_ptr->getMessages().size() - 1);

  // проверки
  //    changeLastReadIndexForSender(Elena1510_ptr, chat_ptr);
  chat_ptr->printChat(Elena1510_ptr);
}
