#include "client_session.h"

#include <QString>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "dto/dto_struct.h"
#include "exceptions_cpp/login_exception.h"
#include "exceptions_qt/exception_login.h"
#include "exceptions_qt/exception_network.h"
#include "exceptions_qt/exception_router.h"
#include "message/message_content_struct.h"
#include "system/date_time_utils.h"
#include "system/picosha2.h"
#include "system/serialize.h"
#include "user/user.h"
#include "user/user_chat_list.h"

#ifndef POLLRDHUP
#define POLLRDHUP 0
#endif

// constructor
ClientSession::ClientSession(ChatSystem &chat_system, QObject *parent)
    : QObject(parent), _instance(chat_system), _core(chat_system, this),
      _dtoSetter(*this), _dtoBuilder(*this),
      _createObjects(*this, _core, _dtoBuilder),
      _modifyObjects(*this, _core) {
  QObject::connect(&_core, &ClientCore::serverStatusChanged, this,
                   &ClientSession::serverStatusChanged);
}

ClientSession::~ClientSession() { stopConnectionThreadCl(); }

bool ClientSession::getIsServerOnlineCl() const noexcept {
  return _core.getIsServerOnlineCore();
}

std::optional<std::multimap<std::int64_t, ChatDTO, std::greater<std::int64_t>>>
ClientSession::getChatListCl() {
  const auto &active_user = _instance.getActiveUser();
  if (!active_user) {
    return std::nullopt;
  }

  const auto &chat_list = active_user->getUserChatList();
  if (!chat_list) {
    return std::nullopt;
  }

  auto chats = chat_list->getChatFromList();

  std::multimap<std::int64_t, ChatDTO, std::greater<std::int64_t>> result;

  if (chats.empty()) {
    return result;
  }

  for (const auto &chat : chats) {
    auto chat_ptr = chat.lock();
    if (!chat_ptr) {
      continue;
    }

    auto chat_dto = _dtoBuilder.fillChatDTOProcessing(chat_ptr);
    if (!chat_dto.has_value()) {
      continue;
    }

    const auto &messages = chat_ptr->getMessages();
    std::int64_t last_message_time_stamp = 0;

    if (!messages.empty()) {
      last_message_time_stamp = messages.rbegin()->first;
    }

    result.insert({last_message_time_stamp, chat_dto.value()});
  }

  return result;
}

// bool ClientSession::CreateAndSendNewChatQt(std::shared_ptr<Chat> &chat_ptr,
// std::vector<std::string> &participants, Message &newMessage) {
bool ClientSession::CreateAndSendNewChatCl(std::shared_ptr<Chat> &chat_ptr,
                                           std::vector<UserDTO> &participants,
                                           Message &newMessage) {
  bool result = true;

  ChatDTO chatDTO;
  chatDTO.chatId = 0;
  chatDTO.senderLogin = getActiveUserCl()->getLogin();

  for (const auto &participant : participants) {
    // временная структура для заполнения
    ParticipantsDTO participantsDTO;

    // заполняем данные на юзера для регистрации в системе
    participantsDTO.login = participant.login;

    // заполняем lastReadMessage
    participantsDTO.lastReadMessage = 0;

    // заполняем deletedMessageIds
    participantsDTO.deletedMessageIds.clear();

    participantsDTO.deletedFromChat = false;

    chatDTO.participants.push_back(participantsDTO);
  } // for

  MessageDTO messageDTO;
  messageDTO.messageId = 0;
  messageDTO.chatId = 0;
  messageDTO.senderLogin = chatDTO.senderLogin;
  messageDTO.timeStamp = newMessage.getTimeStamp();

  // получаем контент
  MessageContentDTO temContent;
  temContent.messageContentType = MessageContentType::Text;
  auto contentElement = newMessage.getContent().front();

  auto contentTextPtr =
      std::dynamic_pointer_cast<MessageContent<TextContent>>(contentElement);

  if (contentTextPtr) {
    auto contentText = contentTextPtr->getMessageContent();
    temContent.payload = contentText._text;
  }

  messageDTO.messageContent.push_back(temContent);

  MessageChatDTO messageChatDTO;
  messageChatDTO.chatId = messageDTO.chatId;
  messageChatDTO.messageDTO.push_back(messageDTO);

  result = createNewChatCl(chat_ptr, chatDTO, messageChatDTO);

  // добавляем участников в чат
  if (result) {
    const auto newMessageId =
        chat_ptr->getMessages().begin()->second->getMessageId();

    chat_ptr->addParticipant(this->getActiveUserCl(), newMessageId, false);

    for (const auto &participant : participants) {
      if (participant.login != this->getActiveUserCl()->getLogin()) {
        const auto &user_ptr =
            this->getInstanceCl().findUserByLogin(participant.login);

        if (user_ptr == nullptr) {
          auto newUser_ptr = std::make_shared<User>(
              UserData(participant.login, participant.userName, "-1",
                       participant.email, participant.phone,
                       participant.disable_reason, participant.is_active,
                       participant.disabled_at, participant.ban_until));

          this->getInstanceCl().addUserToSystem(newUser_ptr);
          chat_ptr->addParticipant(newUser_ptr, 0, false);
        } // if
        else
          chat_ptr->addParticipant(user_ptr, 0, false);
      } // if activeuser
    } // for

    // затем добавляем чат в систему
    this->getInstanceCl().addChatToInstance(chat_ptr);
  } // if

  return result;
}

bool ClientSession::changeUserPasswordCl(UserDTO userDTO) {
  (void)userDTO;
  return false;
}

bool ClientSession::blockUnblockUserCl(const std::string &login, bool isBlocked,
                                       const std::string &disableReason) {

  try {

    UserDTO userDTO;

    userDTO.login = login;
    userDTO.userName = "";
    userDTO.email = "";
    userDTO.phone = "";
    userDTO.passwordhash = "";
    userDTO.ban_until = 0;
    userDTO.disabled_at = 0;

    PacketDTO packetDTO;
    packetDTO.structDTOClassType = StructDTOClassType::userDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;

    if (isBlocked) {
      userDTO.is_active = false;
      userDTO.disable_reason = disableReason;
      packetDTO.requestType = RequestType::RqFrClientBlockUser;
    } else {
      userDTO.is_active = true;
      userDTO.disable_reason = "";
      packetDTO.requestType = RequestType::RqFrClientUnBlockUser;
    }

    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

    std::vector<PacketDTO> packetDTOListSend;
    packetDTOListSend.push_back(packetDTO);

    PacketListDTO packetListDTOresult;
    packetListDTOresult.packets.clear();

    packetListDTOresult = _core.processingRequestToServerCore(
        packetDTOListSend, packetDTO.requestType);

    if (packetListDTOresult.packets.empty() ||
        !packetListDTOresult.packets.front().structDTOPtr) {
      return false;
    }

    const auto &packet = static_cast<const StructDTOClass<ResponceDTO> &>(
                             *packetListDTOresult.packets.front().structDTOPtr)
                             .getStructDTOClass();

    return packet.reqResult;

  } // try
  catch (const exc::UserNotFoundException &ex) {
    const auto time_sdtamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto timeStampQt = QString::fromStdString(time_sdtamp);
    const auto userLoginQt = QString::fromStdString(login);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [AUTH]   [user=%2]   changeUserPasswordQt   ")
            .arg(timeStampQt, userLoginQt));
    return false;
  }
}

bool ClientSession::bunUnbunUserCl(const std::string &login, bool isBanned,
                                   std::int64_t bunnedTo) {
 try {

    UserDTO userDTO;

    userDTO.login = login;
    userDTO.userName = "";
    userDTO.email = "";
    userDTO.phone = "";
    userDTO.passwordhash = "";
    userDTO.disabled_at = 0;
	userDTO.disable_reason = "";
	userDTO.is_active = true;
	
    PacketDTO packetDTO;
    packetDTO.structDTOClassType = StructDTOClassType::userDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;

    if (isBanned) {
		userDTO.ban_until = bunnedTo;
      packetDTO.requestType = RequestType::RqFrClientBunUser;
    } else {
		userDTO.ban_until = 0;
      packetDTO.requestType = RequestType::RqFrClientUnBunUser;
    }

    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

    std::vector<PacketDTO> packetDTOListSend;
    packetDTOListSend.push_back(packetDTO);

    PacketListDTO packetListDTOresult;
    packetListDTOresult.packets.clear();

    packetListDTOresult = _core.processingRequestToServerCore(
        packetDTOListSend, packetDTO.requestType);

    if (packetListDTOresult.packets.empty() ||
        !packetListDTOresult.packets.front().structDTOPtr) {
      return false;
    }

    const auto &packet = static_cast<const StructDTOClass<ResponceDTO> &>(
                             *packetListDTOresult.packets.front().structDTOPtr)
                             .getStructDTOClass();

    return packet.reqResult;

  } // try
  catch (const exc::UserNotFoundException &ex) {
    const auto time_sdtamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto timeStampQt = QString::fromStdString(time_sdtamp);
    const auto userLoginQt = QString::fromStdString(login);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [AUTH]   [user=%2]   changeUserPasswordQt   ")
            .arg(timeStampQt, userLoginQt));
    return false;
  }
}

// threads
void ClientSession::startConnectionThreadCl() {
  if (connection_thread_running_.exchange(true, std::memory_order_acq_rel)) {
    return;
  }
  connection_thread_ = std::thread([this] { connectionMonitorLoopCl(); });
}

void ClientSession::stopConnectionThreadCl() {
  if (!connection_thread_running_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }
  if (connection_thread_.joinable()) {
    connection_thread_.join();
  }
}

// getters

ServerConnectionConfig &ClientSession::getserverConnectionConfigCl() {
  return _core.getServerConnectionConfigCore();
}

const ServerConnectionConfig &
ClientSession::getserverConnectionConfigCl() const {
  return _core.getServerConnectionConfigCore();
}

ServerConnectionMode &ClientSession::getserverConnectionModeCl() {
  return _core.getServerConnectionModeCore();
}

const ServerConnectionMode &ClientSession::getserverConnectionModeCl() const {
  return _core.getServerConnectionModeCore();
}

const std::shared_ptr<User> ClientSession::getActiveUserCl() const {
  return _instance.getActiveUser();
}

ChatSystem &ClientSession::getInstanceCl() { return _instance; }

// int ClientSession::getSocketFdCl() const {
//   return _core.getSocketFdCore();
// }

// void ClientSession::setSocketFdCl(int socket_fd) {
//   _core.setSocketFdCore(socket_fd);
// }

const std::vector<UserDTO>
ClientSession::findUserByTextPartOnServerCl(const std::string &text_to_find) {
  std::vector<UserDTO> result;
  result.clear();

  const auto &active_user = _instance.getActiveUser();
  if (!active_user) {
    return result;
  }

  UserLoginPasswordDTO user_login_password_dto;
  user_login_password_dto.login = active_user->getLogin();
  user_login_password_dto.passwordhash = text_to_find;

  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientFindUserByPart;
  packet_dto.structDTOClassType = StructDTOClassType::userLoginPasswordDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(
          user_login_password_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto response_packet_list = _core.processingRequestToServerCore(
      packet_list_send, packet_dto.requestType);

  for (const auto &packet : response_packet_list.packets) {
    if (packet.requestType != RequestType::RqFrClientFindUserByPart) {
      continue;
    }

    if (!packet.structDTOPtr) {
      continue;
    }

    switch (packet.structDTOClassType) {
    case StructDTOClassType::userDTO: {
      const auto &packet_user_dto =
          static_cast<const StructDTOClass<UserDTO> &>(*packet.structDTOPtr)
              .getStructDTOClass();
      result.push_back(packet_user_dto);
      break;
    }
    case StructDTOClassType::responceDTO: {
      const auto &response =
          static_cast<const StructDTOClass<ResponceDTO> &>(*packet.structDTOPtr)
              .getStructDTOClass();
      if (!response.reqResult) {
        result.clear();
        return result;
      }
      break;
    }
    default:
      break;
    }
  }

  return result;
}
bool ClientSession::checkUserLoginCl(const std::string &user_login) {
  const auto is_on_client_device = _instance.findUserByLogin(user_login);
  if (is_on_client_device != nullptr) {
    return true;
  }

  UserLoginDTO user_login_dto;
  user_login_dto.login = user_login;

  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientCheckLogin;
  packet_dto.structDTOClassType = StructDTOClassType::userLoginDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<UserLoginDTO>>(user_login_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto packet_list_result = _core.processingRequestToServerCore(
      packet_list_send, packet_dto.requestType);

  try {
    if (packet_list_result.packets.size() != 1) {
      throw exc_qt::WrongPacketSizeException();
    }

    if (packet_list_result.packets[0].requestType !=
        RequestType::RqFrClientCheckLogin) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &response = static_cast<const StructDTOClass<ResponceDTO> &>(
                               *packet_list_result.packets[0].structDTOPtr)
                               .getStructDTOClass();
    return response.reqResult;
  } catch (const std::exception &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);
    const auto user_login_qt = QString::fromStdString(user_login);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=]   "
                       "[msg=]   checkUserLoginCl")
            .arg(time_stamp_qt, user_login_qt));
    return false;
  }
}
bool ClientSession::checkUserPasswordCl(const std::string &user_login,
                                        const std::string &password) {

  const auto password_hash = picosha2::hash256_hex_string(password);

  UserLoginPasswordDTO user_login_password_dto;
  user_login_password_dto.login = user_login;
  user_login_password_dto.passwordhash = password_hash;

  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientCheckLogPassword;
  packet_dto.structDTOClassType = StructDTOClassType::userLoginPasswordDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(
          user_login_password_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto packet_list_result = _core.processingRequestToServerCore(
      packet_list_send, packet_dto.requestType);

  try {
    if (packet_list_result.packets.size() != 1) {
      throw exc_qt::WrongPacketSizeException();
    }

    if (packet_list_result.packets[0].requestType !=
        RequestType::RqFrClientCheckLogPassword) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &response = static_cast<const StructDTOClass<ResponceDTO> &>(
                               *packet_list_result.packets[0].structDTOPtr)
                               .getStructDTOClass();

    return response.reqResult;
  } catch (const std::exception &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);
    const auto user_login_qt = QString::fromStdString(user_login);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=]   "
                       "[msg=]   checkUserPasswordCl")
            .arg(time_stamp_qt, user_login_qt));
    return false;
  }
}

void ClientSession::clearChatSystemCl() {
  _instance.clear_chat_system();
}

bool ClientSession::reInitilizeBaseCl() {
  UserLoginDTO user_login_dto;
  user_login_dto.login = "reInitilizeBase";

  PacketDTO packet_dto;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOClassType = StructDTOClassType::userLoginDTO;
  packet_dto.requestType = RequestType::RqFrClientReInitializeBase;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<UserLoginDTO>>(user_login_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto packet_list_result = _core.processingRequestToServerCore(
      packet_list_send, packet_dto.requestType);

  try {
    if (packet_list_result.packets.empty()) {
      throw exc_qt::WrongresponceTypeException();
    }

    if (packet_list_result.packets[0].requestType != packet_dto.requestType) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &packet = static_cast<const StructDTOClass<ResponceDTO> &>(
                             *packet_list_result.packets[0].structDTOPtr)
                             .getStructDTOClass();
    return packet.reqResult;
  } catch (const exc_qt::WrongresponceTypeException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral(
            "[%1]   [ERROR]   [NETWORK]   [user=]   [chat_Id=]   [msg=]   "
            "reInitilizeBaseCl   wrong type of answer from server")
            .arg(time_stamp_qt));
    return false;
  }
}

bool ClientSession::registerClientToSystemCl(const std::string &login) {
  UserLoginDTO user_login_dto;
  user_login_dto.login = login;

  PacketDTO packet_dto;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOClassType = StructDTOClassType::userLoginDTO;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<UserLoginDTO>>(user_login_dto);
  packet_dto.requestType = RequestType::RqFrClientRegisterUser;

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto packet_list_result = _core.processingRequestToServerCore(
      packet_list_send, packet_dto.requestType);

  try {
    for (const auto &packet : packet_list_result.packets) {
      if (packet.requestType != packet_dto.requestType) {
        throw exc_qt::WrongresponceTypeException();
      }
    }

    for (const auto &packet : packet_list_result.packets) {
      if (!packet.structDTOPtr) {
        continue;
      }

      switch (packet.structDTOClassType) {
      case StructDTOClassType::userDTO: {
        const auto &user_dto =
            static_cast<const StructDTOClass<UserDTO> &>(*packet.structDTOPtr)
                .getStructDTOClass();
        _dtoSetter.setActiveUserDTOFromSrvProcessing(user_dto);
        break;
      }
      case StructDTOClassType::chatDTO: {
        const auto &chat_dto =
            static_cast<const StructDTOClass<ChatDTO> &>(*packet.structDTOPtr)
                .getStructDTOClass();
        _dtoSetter.setOneChatDTOFromSrvProcessing(chat_dto);
        break;
      }
      case StructDTOClassType::messageChatDTO: {
        const auto &message_chat_dto =
            static_cast<const StructDTOClass<MessageChatDTO> &>(
                *packet.structDTOPtr)
                .getStructDTOClass();
        _dtoSetter.setOneChatMessageDTOProcessing(message_chat_dto);
        break;
      }
      case StructDTOClassType::responceDTO: {
        const auto &response = static_cast<const StructDTOClass<ResponceDTO> &>(
                                   *packet.structDTOPtr)
                                   .getStructDTOClass();
        if (!response.reqResult) {
          return false;
        }
        break;
      }
      default:
        break;
      }
    }
  } catch (const std::exception &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);
    const auto login_qt = QString::fromStdString(login);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   "
                       "registerClientToSystemCl")
            .arg(time_stamp_qt, login_qt));
    return false;
  }

  return true;
}

bool ClientSession::changeUserDataCl(const UserDTO &user_dto) {
  return _modifyObjects.changeUserDataProcessing(user_dto);
}

bool ClientSession::createUserCl(const UserDTO &user_dto) {
  return _createObjects.createUserProcessing(user_dto);
}

bool ClientSession::createNewChatCl(std::shared_ptr<Chat> &chat,
                                    ChatDTO &chat_dto,
                                    MessageChatDTO &message_chat_dto) {
  return _createObjects.createNewChatProcessing(chat, chat_dto,
                                                message_chat_dto);
}

std::size_t ClientSession::createMessageCl(const Message &message,
                                           std::shared_ptr<Chat> &chat_ptr,
                                           const std::shared_ptr<User> &user) {
  return _createObjects.createMessageProcessing(message, chat_ptr, user);
}

bool ClientSession::sendLastReadMessageFromClientCl(
    const std::shared_ptr<Chat> &chat_ptr, std::size_t message_id) {
  MessageDTO message_dto;
  message_dto.chatId = chat_ptr->getChatId();
  message_dto.messageId = message_id;
  if (const auto active_user = _instance.getActiveUser()) {
    message_dto.senderLogin = active_user->getLogin();
  }
  message_dto.messageContent = {};
  message_dto.timeStamp = 0;

  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientSetLastReadMessage;
  packet_dto.structDTOClassType = StructDTOClassType::messageDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<MessageDTO>>(message_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  auto response_packet_list = _core.processingRequestToServerCore(
      packet_list_send, RequestType::RqFrClientSetLastReadMessage);

  try {
    if (response_packet_list.packets.empty()) {
      throw exc_qt::EmptyPacketException();
    }

    if (response_packet_list.packets[0].requestType !=
        RequestType::RqFrClientSetLastReadMessage) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &response = static_cast<const StructDTOClass<ResponceDTO> &>(
                               *response_packet_list.packets[0].structDTOPtr)
                               .getStructDTOClass();

    if (!response.reqResult) {
      throw exc_qt::LastReadMessageException();
    }

    return true;
  } catch (const std::exception &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);
    const auto sender_login_qt =
        QString::fromStdString(message_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(message_dto.chatId));
    const auto message_id_qt =
        QString::number(static_cast<long long>(message_dto.messageId));

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   sendLastReadMessageFromClientCl")
            .arg(time_stamp_qt, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  }
}

bool ClientSession::checkAndAddParticipantToSystemCl(
    const std::vector<std::string> &participants) {
  try {
    bool need_request = false;
    std::vector<PacketDTO> packet_list_send;

    for (const auto &participant : participants) {
      const auto &user_ptr = _instance.findUserByLogin(participant);
      if (!user_ptr) {
        need_request = true;

        UserLoginDTO user_login_dto;
        user_login_dto.login = participant;

        PacketDTO packet_dto;
        packet_dto.requestType = RequestType::RqFrClientGetUsersData;
        packet_dto.structDTOClassType = StructDTOClassType::userLoginDTO;
        packet_dto.reqDirection = RequestDirection::ClientToSrv;
        packet_dto.structDTOPtr =
            std::make_shared<StructDTOClass<UserLoginDTO>>(user_login_dto);

        packet_list_send.push_back(packet_dto);
      }
    }

    if (need_request) {
      auto packet_list_result = _core.processingRequestToServerCore(
          packet_list_send, RequestType::RqFrClientGetUsersData);

      for (const auto &response_packet : packet_list_result.packets) {
        if (response_packet.requestType !=
            RequestType::RqFrClientGetUsersData) {
          continue;
        }

        if (response_packet.structDTOClassType != StructDTOClassType::userDTO) {
          throw exc_qt::WrongresponceTypeException();
        }

        const auto &user_dto = static_cast<const StructDTOClass<UserDTO> &>(
                                   *response_packet.structDTOPtr)
                                   .getStructDTOClass();

        auto new_user = std::make_shared<User>(UserData(
            user_dto.login, user_dto.userName, "-1", user_dto.email,
            user_dto.phone, user_dto.disable_reason, user_dto.is_active,
            user_dto.disabled_at, user_dto.ban_until));

        _instance.addUserToSystem(new_user);
      }
    } else {
      const auto time_stamp =
          formatTimeStampToString(getCurrentDateTimeInt(), true);
      const auto time_stamp_qt = QString::fromStdString(time_stamp);

      emit exc_qt::ErrorBus::i().error(
          "",
          QStringLiteral(
              "[%1]   ensureParticipantsAvailable   server answer did not come")
              .arg(time_stamp_qt));

      for (const auto &packet_dto : packet_list_send) {
        const auto &user_login_dto =
            static_cast<const StructDTOClass<UserLoginDTO> &>(
                *packet_dto.structDTOPtr)
                .getStructDTOClass();

        auto new_user = std::make_shared<User>(
            UserData(user_login_dto.login, "Unknown", "-1", "Unknown",
                     "Unknown", "", true, 0, 0));

        _instance.addUserToSystem(new_user);
      }
    }
  } catch (const exc_qt::WrongresponceTypeException &ex) {
    const auto time_stamp =
        formatTimeStampToString(getCurrentDateTimeInt(), true);
    const auto time_stamp_qt = QString::fromStdString(time_stamp);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=]   "
                       "ensureParticipantsAvailable   Wrong responce type")
            .arg(time_stamp_qt));

    return false;
  }
  return true;
}

void ClientSession::connectionMonitorLoopCl() {
  try {
    _core.connectionMonitorLoopCore(connection_thread_running_);
  } catch (const std::exception &) {
    connection_thread_running_.store(false, std::memory_order_release);
  }
}
