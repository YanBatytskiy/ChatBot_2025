#include "server_session.h"
#include "chat/chat.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "exception/validation_exception.h"
#include "message/message.h"
#include "message/message_content_struct.h"
#include "system/date_time_utils.h"
#include "system/serialize.h"
#include "system/system_function.h"
#include "user/user.h"
#include "user/user_chat_list.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>

ServerSession::ServerSession(ChatSystem &server) : _instance(server) {}

// getters

ServerConnectionConfig &ServerSession::getServerConnectionConfig() { return _serverConnectionConfig; }

int &ServerSession::getConnection() { return _connection; }
const int &ServerSession::getConnection() const { return _connection; }

const std::shared_ptr<User> ServerSession::getActiveUserSrv() const { return _instance.getActiveUser(); }

ChatSystem &ServerSession::getInstance() { return _instance; }

// setters
void ServerSession::setActiveUserSrv(const std::shared_ptr<User> &user) { _instance.setActiveUser(user); }

void ServerSession::setConnection(const std::uint8_t &connection) { _connection = connection; }
//
//
// transport

bool ServerSession::isConnected() const { return _connection > 0 && fcntl(_connection, F_GETFD) != -1; }

void ServerSession::runServer(int socketFd) {
  try {

    // если соединение уже есть — проверим его валидность
    if (_connection > 0) {
      // Проверим валидность дескриптора
      if (fcntl(_connection, F_GETFD) != -1) {
        // Соединение уже активно — ничего делать не нужно
        return;
      } else {
        // Соединение есть, но оно невалидно — закрываем
        close(_connection);
        _connection = -1;
      }
    }
    // создаем новое соединение
    struct sockaddr_in client {};
    socklen_t client_len = sizeof(client);

    _connection = accept(socketFd, (struct sockaddr *)&client, &client_len);
    if (_connection < 0) {
      throw exc::ConnectNotAcceptException();
    }

    // сохранили номер сессии
    std::cout << "[Инфо] Новое соединение установлено" << std::endl;

  } catch (const exc::ConnectNotAcceptException &ex) {
    std::cerr << "Сервер. " << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Сервер. Неизвестная ошибка. " << ex.what() << std::endl;
  }
}
//
//
//
void ServerSession::listeningClients() {

  try {
    // std::vector<std::uint8_t> buffer(MESSAGE_LENGTH);

    // // проверка
    // // std::cerr << "[DEBUG] buffer.size() = " << buffer.size() << std::endl;

    // if (buffer.empty()) {
    //   std::cerr << "[Ошибка] Буфер пустой (buffer.size() == 0)" << std::endl;
    //   std::exit(1);
    // }

    // if (buffer.data() == nullptr) {
    //   std::cerr << "[Ошибка] buffer.data() == nullptr" << std::endl;
    //   std::exit(1);
    // }

    if (_connection <= 0) {
      throw exc::SocketInvalidException();
    }

    // проверка
    // if (MESSAGE_LENGTH == 0)
    //   throw exc::CreateBufferException();

    if (_connection <= 0 || fcntl(_connection, F_GETFD) == -1) {
      throw exc::SocketInvalidException();
    }
    // проверка

    // std::cerr << "[DEBUG] connection fd = " << connection << std::endl;

    // 1. читаем 4 байта длины
    std::uint8_t lenBuf[4];
    std::size_t total = 0;
    while (total < 4) {
      ssize_t r = recv(_connection, lenBuf + total, 4 - total, 0);
      if (r <= 0)
        throw exc::ReceiveDataException();
      total += r;
    }

    uint32_t len = 0;
    std::memcpy(&len, lenBuf, 4);
    len = ntohl(len);

    // 2. читаем len байт данных
    std::vector<std::uint8_t> buffer(len);
    std::size_t bytesReceived = 0;
    while (bytesReceived < len) {
      ssize_t r = recv(_connection, buffer.data() + bytesReceived, len - bytesReceived, 0);
      if (r <= 0)
        throw exc::ReceiveDataException();
      bytesReceived += r;
    }

    std::vector<PacketDTO> packetListReceivedVector = deSerializePacketList(buffer);

    if (packetListReceivedVector.empty())
      throw exc::EmptyPacketException();

    // берем первый пакет - хедер
    PacketDTO firstPacket = packetListReceivedVector.front();

    if (firstPacket.structDTOClassType != StructDTOClassType::userLoginPasswordDTO)
      throw exc::HeaderWrongTypeException();

    const auto &headerPacket = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(*firstPacket.structDTOPtr)
                                   .getStructDTOClass();

    if (headerPacket.passwordhash != "UserHeder")
      throw exc::HeaderWrongTypeException();

    if (headerPacket.login.empty())
      throw exc::HeaderWrongDataException();

    const auto &requestType = firstPacket.requestType;

    packetListReceivedVector.erase(packetListReceivedVector.begin());

    if (packetListReceivedVector.empty())
      throw exc::EmptyPacketException();

    PacketListDTO packetListReceived;
    for (const auto &packet : packetListReceivedVector)
      packetListReceived.packets.push_back(packet);

    routingRequestsFromClient(packetListReceived, requestType, _connection);
  } // try
  catch (const exc::SocketInvalidException &ex) {
    std::cerr << "Сервер: " << ex.what() << " = " << _connection << std::endl;
    close(_connection);
    _connection = -1;
    return;
  } catch (const exc::CreateBufferException &ex) {
    std::cerr << "Сервер: " << ex.what() << " = " << MESSAGE_LENGTH << std::endl;
  } catch (const exc::ReceiveDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    _connection = -1;
    return;
  } catch (const exc::HeaderWrongDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    _connection = -1;
    return;
  }
}
//
//
//
bool ServerSession::sendPacketListDTO(PacketListDTO &packetListForSend, int connection) {
ssize_t bytesSent = 0;
  try {

    // Проверка валидности соединения
    if (connection <= 0 || fcntl(connection, F_GETFD) == -1)
      throw exc::SocketInvalidException();


    // Сериализуем пакеты

    auto PacketSendBinary = serializePacketList(packetListForSend.packets);

    // Проверка: не отправлять пустой буфер
    if (PacketSendBinary.empty())
      throw exc::SendDataException();

    // проверка
    // std::cerr << "[DEBUG] Отправка пакета. Размер: " << PacketSendBinary.size() << " байт" << std::endl;
    // for (std::size_t i = 0; i < PacketSendBinary.size(); ++i) {
    //   std::cerr << static_cast<int>(PacketSendBinary[i]) << " ";
    // }
    // std::cerr << std::endl;

    // Отправка данных
    uint32_t len = htonl(PacketSendBinary.size());

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;

    send(connection, &len, 4, 0);

    // std::cerr << "[SERVER DEBUG] requestType of first packet = "
    //           << static_cast<int>(packetListForSend.packets[0].requestType) << std::endl;
    bytesSent = send(connection, PacketSendBinary.data(), PacketSendBinary.size(), 0);

    // Проверка результата
    if (bytesSent <= 0)
      throw exc::SendDataException();

    if (bytesSent != static_cast<ssize_t>(PacketSendBinary.size()))
      throw exc::SendDataException();
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    std::cerr << "Send failed. Bytes sent = " << bytesSent << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер sendPacketListDTO: " << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cerr << "Сервер: Неизвестная ошибка. sendPacketListDTO" << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
// Request processing

bool ServerSession::routingRequestsFromClient(PacketListDTO &packetListReceived, const RequestType &requestType,
                                              int connection) {

  const auto packetDTOrequestType = packetListReceived.packets[0].requestType;

  // check and registry User
  switch (packetDTOrequestType) {
  case RequestType::RqFrClientCheckLogin:
  case RequestType::RqFrClientCheckLogPassword:
  case RequestType::RqFrClientRegisterUser:
  case RequestType::RqFrClientFindUserByPart:
  case RequestType::RqFrClientSetLastReadMessage:
  case RequestType::RqFrClientFindUserByLogin: {

    processingCheckAndRegistryUser(packetListReceived, packetDTOrequestType, connection);
    break;
  }
  // create objects
  case RequestType::RqFrClientCreateUser:
  case RequestType::RqFrClientCreateChat:
  case RequestType::RqFrClientCreateMessage: {
    if (!processingCreateObjects(packetListReceived, packetDTOrequestType, connection))
      return false;
    break;
  }
    // get indexes and user Data
  case RequestType::RqFrClientGetUsersData: {
    processingGetIndexes(packetListReceived, packetDTOrequestType, connection);
    break;
  }
  default:
    break;
  }
  return true;
}
bool ServerSession::processingCheckAndRegistryUser(PacketListDTO &packetListReceived, const RequestType &requestType,
                                                   int connection) {

  // создали структуру вектора пакетов для отправки
  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  switch (requestType) {
  case RequestType::RqFrClientCheckLogin: {

    // отдельный пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = requestType;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    const auto user_ptr = _instance.findUserByLogin(packet.login);

    // пакет для отправки
    ResponceDTO responceDTO;
    responceDTO.anyNumber = 0;

    if (user_ptr != nullptr) {
      responceDTO.reqResult = true;
      responceDTO.anyString = packet.login;
    } else {
      responceDTO.reqResult = false;
      responceDTO.anyString = "@";
    }

    packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

    packetDTOListForSend.packets.push_back(packetDTOForSend);

    break;
  }
  case RequestType::RqFrClientCheckLogPassword: {
    // отдельный пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = requestType;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    const auto &packet = static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                             *packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    const auto user_ptr = _instance.findUserByLogin(packet.login);

    // пакет для отправки
    ResponceDTO responceDTO;
    responceDTO.anyNumber = 0;

    if (user_ptr != nullptr) {
      responceDTO.reqResult = packet.passwordhash == user_ptr->getPasswordHash();
      responceDTO.anyString = packet.login;
    } else {
      responceDTO.reqResult = false;
      responceDTO.anyString = "@";
    }

    packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

    packetDTOListForSend.packets.push_back(packetDTOForSend);

    break;
  }
  case RequestType::RqFrClientRegisterUser: {

    const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    const auto user_ptr = _instance.findUserByLogin(packet.login);

    const auto packetListDTOVector = registerOnDeviceDataSrv(user_ptr);

    if (!packetListDTOVector.has_value())
      return false;

    packetDTOListForSend = packetListDTOVector.value();

    break;
  }
  case RequestType::RqFrClientFindUserByPart: {
    const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    const auto usersFound = _instance.findUserByTextPart(packet.login);

    // пакет для отправки

    for (const auto &user_ptr : usersFound) {
      if (user_ptr) {

        PacketDTO packetDTOForSend;
        packetDTOForSend.requestType = RequestType::RqFrClientFindUserByPart;
        packetDTOForSend.structDTOClassType = StructDTOClassType::userDTO;
        packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

        auto userDTO = FillForSendUserDTOFromSrv(user_ptr->getLogin(), false);

        packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

        packetDTOListForSend.packets.push_back(packetDTOForSend);
      }
    }

    break;
  }
  case RequestType::RqFrClientSetLastReadMessage: {
    const auto &packet = static_cast<const StructDTOClass<MessageDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                             .getStructDTOClass();

    const auto &chatId = packet.chatId;
    const auto &messageId = packet.messageId;
    const auto &userLogin = packet.senderLogin;

    const auto user_ptr = _instance.findUserByLogin(userLogin);
    const auto chat_ptr = _instance.getChatById(chatId);

    ResponceDTO responceDTO;
    responceDTO.anyNumber = 0;
    responceDTO.anyString = "";

    // пакет для отправки
    PacketDTO packetDTOForSend;
    packetDTOForSend.requestType = RequestType::RqFrClientSetLastReadMessage;
    packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
    packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

    if (user_ptr == nullptr || chat_ptr == nullptr) {
      std::cerr << "Сервер: RqFrClientSetLastReadMessage. Нет юзера или чата" << std::endl;
      responceDTO.reqResult = false;
    } else {
      chat_ptr->setLastReadMessageId(user_ptr, messageId);
      responceDTO.reqResult = true;
    }

    packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);
    packetDTOListForSend.packets.push_back(packetDTOForSend);
    break;
  }

  default:
    throw exc::HeaderWrongTypeException();
    break;
  }

  sendPacketListDTO(packetDTOListForSend, connection);
  //   } // try
  //   catch (const exc::UserNotFoundException &ex) {
  //     return false;
  //   } catch (const exc::ChatNotFoundException &ex) {
  //     std::cerr << "Сервер: RqFrClientSetLastReadMessage. " << ex.what() << std::endl;
  //     return false;
  //   }
  return true;
}
//
//
//
bool ServerSession::processingCreateObjects(PacketListDTO &packetListReceived, const RequestType &requestType,
                                            int connection) { // создали структуру вектора пакетов для отправки

  PacketListDTO packetDTOListForSend;
  packetDTOListForSend.packets.clear();

  try {
    switch (requestType) {
    case RequestType::RqFrClientCreateUser: {

      if (packetListReceived.packets.empty() ||
          packetListReceived.packets[0].structDTOClassType != StructDTOClassType::userDTO) {
        throw exc::EmptyPacketException();
      }

      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      const auto &packet = static_cast<const StructDTOClass<UserDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

      // пакет для отправки
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;
      responceDTO.anyString = "";

      if (createUserSrv(packet)) {
        responceDTO.reqResult = true;
      } else
        responceDTO.reqResult = false;

      packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

      packetDTOListForSend.packets.push_back(packetDTOForSend);

      break;
    }
    case RequestType::RqFrClientCreateChat: {
      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      // добавляем чат
      const auto &packet = static_cast<const StructDTOClass<ChatDTO> &>(*packetListReceived.packets[0].structDTOPtr)
                               .getStructDTOClass();

      // пакет для отправки
      ResponceDTO responceDTO;

      auto chat_ptr = std::make_shared<Chat>();

      if (createNewChatSrv(chat_ptr, packet)) {

        // добавляем cообщение
        auto &packet = static_cast<StructDTOClass<MessageChatDTO> &>(*packetListReceived.packets[1].structDTOPtr)
                           .getStructDTOClass();

        packet.chatId = chat_ptr->getChatId();

        if (createNewMessageChatSrv(chat_ptr, packet)) {
          responceDTO.reqResult = true;
          responceDTO.anyNumber = chat_ptr->getChatId();
          responceDTO.anyString = std::to_string(chat_ptr->getMessages().begin()->second->getMessageId());

          packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

          packetDTOListForSend.packets.push_back(packetDTOForSend);
        } // if message
        else {
          responceDTO.reqResult = false;
          responceDTO.anyString = "Message false";
        }
      } // if chat
      else {
        responceDTO.reqResult = false;
        responceDTO.anyNumber = -1;
      }
      break;
    }
    case RequestType::RqFrClientCreateMessage: {

      // отдельный пакет для отправки
      PacketDTO packetDTOForSend;
      packetDTOForSend.requestType = requestType;
      packetDTOForSend.structDTOClassType = StructDTOClassType::responceDTO;
      packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;

      // пакет для отправки
      ResponceDTO responceDTO;
      responceDTO.anyNumber = 0;

      // доcтаем пакерт
      const auto &messageDTO = static_cast<const StructDTOClass<MessageDTO> &>(
                                   *packetListReceived.packets[0].structDTOPtr)
                                   .getStructDTOClass();

      const auto &result = createNewMessageSrv(messageDTO);

      if (result) {
        responceDTO.reqResult = true;
        responceDTO.anyString = std::to_string(result);

        packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO);

        packetDTOListForSend.packets.push_back(packetDTOForSend);
      } // if result
      else {
        responceDTO.reqResult = false;
        responceDTO.anyString = "0";
      }

      break;
    }
    default:
      throw exc::HeaderWrongTypeException();
      break;
    }
    // }// for
    sendPacketListDTO(packetDTOListForSend, connection);
  } // try
  catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;

  } catch (const std::bad_cast &ex) {
    std::cerr << "Сервер: Неправильный тип пакета. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ServerSession::processingGetIndexes(PacketListDTO &packetListReceived, const RequestType &requestType,
                                         int connection) {
  // создали структуру вектора пакетов для отправки
  PacketListDTO packetDTOListForSend;

  try {
    for (const auto &packetDTOReceived : packetListReceived.packets) {

      if (packetDTOReceived.requestType != RequestType::RqFrClientGetUsersData)
        throw exc::HeaderWrongTypeException();

      const auto &packet = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetDTOReceived.structDTOPtr)
                               .getStructDTOClass();

      const auto user_ptr = _instance.findUserByLogin(packet.login);

      if (!user_ptr)
        throw exc::UserNotFoundException();

      UserDTO userDTO = FillForSendUserDTOFromSrv(user_ptr->getLogin(), false);

      // формируем пользователя
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientGetUsersData;
      packetDTO.structDTOClassType = StructDTOClassType::userDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

      packetDTOListForSend.packets.push_back(packetDTO);
    }

    sendPacketListDTO(packetDTOListForSend, connection);
  } // try
  catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ServerSession::processingReceivedQueue(const std::string &userLogin) {

  // auto &dequeReceived = _instance.getPacketReceivedDeque();
  // auto &dequeForSend = _instance.getPacketForSendDeque();

  // if (dequeReceived.empty())
  //   return false;

  // while (!dequeReceived.empty()) {
  //   const auto &[packetDTO, userLogin] = dequeReceived.front();

  //   PacketListDTO packetListForSend;

  //   if (processingRequestToToSendToServer(packetDTO, packetListForSend))
  //     dequeForSend.push_back({packetListForSend, userLogin});
  //   dequeReceived.pop_front();
  //   ;
  // }
  return true;
}
//
//
//
bool ServerSession::processingSendQueue() {

  // auto &dequeForSend = _instance.getPacketForSendDeque();
  // try {
  //   while (!dequeForSend.empty()) {
  //     auto &[packetListDTO, userLogin] = dequeForSend.front();

  //     auto PacketSendBinary = serializePacketList(packetListDTO.packets);
  //     auto it = _loginToConnectionMap.find(userLogin);

  //     if (it == _loginToConnectionMap.end()) {
  //       std::cerr << "Сервер: логин " << userLogin
  //                 << " не найден в loginToConnectionMap. Пропускаем
  //                 отправку."
  //                 << std::endl;
  //       dequeForSend.pop_front();
  //       continue;
  //     }

  //     int connection = it->second;

  //     ssize_t bytesSent =
  //         send(connection, PacketSendBinary.data(),
  //         PacketSendBinary.size(), 0);
  //     if (bytesSent <= 0) {
  //       throw exc::SendDataException();
  //     } else {
  //       dequeForSend.pop_front();
  //     }
  //   }
  // } // try
  // catch (const exc::SendDataException &ex) {
  //   std::cerr << "Сервер: " << ex.what() << std::endl;
  //   return false;
  // }

  return true;
}
//
//
//

// поиск пользователя
const std::vector<UserDTO> ServerSession::findUserByTextPartFromSrv(const std::string &textToFind) const {

  const auto &users = this->_instance.findUserByTextPart(textToFind);
  std::vector<UserDTO> userDTO;

  if (!users.empty()) {
    for (const auto& user : users) {
      userDTO.push_back(FillForSendUserDTOFromSrv(user->getLogin(), true));
    }
  } else
    userDTO.clear();
  return userDTO;
}

bool ServerSession::checkUserLoginSrv(const UserLoginDTO &userLoginDTO) const {

  if (this->_instance.findUserByLogin(userLoginDTO.login) != nullptr)
    return true;
  else
    return false;
}
//
//
//
bool ServerSession::checkUserPasswordSrv(const UserLoginPasswordDTO &userLoginPasswordDTO) const {

  return this->_instance.checkPasswordValidForUser(userLoginPasswordDTO.passwordhash, userLoginPasswordDTO.login);
}
//
//
//
UserDTO ServerSession::FillForSendUserDTOFromSrv(const std::string &userLogin, bool loginUser) const {

  UserDTO userDTO;
  auto user = this->_instance.findUserByLogin(userLogin);
  if (!user)
    throw exc::UserNotFoundException();

  userDTO.login = user->getLogin();
  userDTO.userName = user->getUserName();
  userDTO.email = user->getEmail();
  userDTO.phone = user->getPhone();
  userDTO.passwordhash = loginUser ? user->getPasswordHash() : "-1";

  return userDTO;
}
//
//
//
// получаем один чат пользователя
std::optional<ChatDTO> ServerSession::FillForSendOneChatDTOFromSrv(const std::shared_ptr<Chat> &chat_ptr,
                                                                   const std::shared_ptr<User> &user) {
  ChatDTO chatDTO;

  // взяли chatId
  chatDTO.chatId = chat_ptr->getChatId();
  chatDTO.senderLogin = user->getLogin();

  try {
    // получаем список участников
    auto participants = chat_ptr->getParticipants();

    // перебираем участников
    for (const auto &participant : participants) {

      // получили указатель на юзера
      const auto user_ptr = participant._user.lock();

      // временная структура для заполнения
      ParticipantsDTO participantsDTO;

      if (user_ptr) {

        // заполняем данные на юзера для регистрации в системе

        participantsDTO.login = user_ptr->getLogin();

        participantsDTO.lastReadMessage = chat_ptr->getLastReadMessageId(user_ptr);

        // заполняем deletedMessageIds
        const auto &delMsgMap = chat_ptr->getDeletedMessagesMap();
        const auto it = delMsgMap.find(participantsDTO.login);

        if (it != delMsgMap.end()) {

          for (const auto &delMsgId : it->second)
            participantsDTO.deletedMessageIds.push_back(delMsgId);
        }

        participantsDTO.deletedFromChat = participant._deletedFromChat;

        chatDTO.participants.push_back(participantsDTO);

      } // if user_ptr
      else
        throw exc::UserNotFoundException();
    } // for participants
  }
  // try
  catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: FillForSendOneChatDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }

  return chatDTO;
}
//
//
// получаем все чаты пользователя
std::optional<std::vector<ChatDTO>> ServerSession::FillForSendAllChatDTOFromSrv(const std::shared_ptr<User> &user) {
  // взяли чат лист
  auto chatList = user->getUserChatList();
  std::vector<ChatDTO> chatDTOVector;
  // перебираем чаты в чат листе
  for (const auto &chat : chatList->getChatFromList()) {

    // получили указатель на чат
    auto chat_ptr = chat.lock();

    if (!chat_ptr) {
      std::cerr << "Сервер: FillForSendAllChatDTOFromSrv. Чат не доступен" << std::endl;
      continue;
    } else {
      auto tempDTO = FillForSendOneChatDTOFromSrv(chat_ptr, user);

      if (tempDTO.has_value())
        chatDTOVector.push_back(tempDTO.value());
      else {
        std::cerr << "Сервер: FillForSendAllChatDTOFromSrv. Чат не заполнен" << std::endl;
        continue;
      }
    } // first if chat_ptr
  } // first for

  return chatDTOVector;
}
//
//
// получаем одно конкретное сообщение пользователя
std::optional<MessageDTO> ServerSession::FillForSendOneMessageDTOFromSrv(const std::shared_ptr<Message> &message,
                                                                         const std::size_t &chatId) {

  MessageDTO messageDTO;
  auto user_ptr = message->getSender().lock();

  messageDTO.senderLogin = user_ptr ? user_ptr->getLogin() : "";

  messageDTO.chatId = chatId;
  messageDTO.messageId = message->getMessageId();
  messageDTO.timeStamp = message->getTimeStamp();

  // получаем контент
  MessageContentDTO temContent;
  temContent.messageContentType = MessageContentType::Text;
  auto contentElement = message->getContent().front();

  auto contentTextPtr = std::dynamic_pointer_cast<MessageContent<TextContent>>(contentElement);

  if (contentTextPtr) {
    auto contentText = contentTextPtr->getMessageContent();
    temContent.payload = contentText._text;
  }

  messageDTO.messageContent.push_back(temContent);

  return messageDTO;
}
//
//
// получаем сообщения пользователя конкретного чата
std::optional<MessageChatDTO> ServerSession::fillForSendChatMessageDTOFromSrv(const std::shared_ptr<Chat> &chat) {

  MessageChatDTO messageChatDTO;

  // взяли chatId
  messageChatDTO.chatId = chat->getChatId();
  try {
    for (const auto &message : chat->getMessages()) {

      const auto &messageDTO = FillForSendOneMessageDTOFromSrv(message.second, messageChatDTO.chatId);

      if (!messageDTO.has_value())
        throw exc::MessagesNotFoundException();

      messageChatDTO.messageDTO.push_back(messageDTO.value());

    } // for message
  } // try
  catch (const exc::MessagesNotFoundException &ex) {
    std::cerr << "Сервер: fillForSendChatMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }
  return messageChatDTO;
}
//
//
// получаем все сообщения пользователя
std::optional<std::vector<MessageChatDTO>> ServerSession::fillForSendAllMessageDTOFromSrv(
    const std::shared_ptr<User> &user) {

  std::vector<MessageChatDTO> messageChatDTOVector;
  try {
    // взяли чат лист
    auto chatList = user->getUserChatList();

    // перебираем чаты в чат листе
    for (const auto &chat : chatList->getChatFromList()) {

      // получили указатель на чат
      auto chat_ptr = chat.lock();

      if (chat_ptr) {

        // вызываем получение сообщений чата

        const auto &messageChatDTO = fillForSendChatMessageDTOFromSrv(chat_ptr);

        if (!messageChatDTO.has_value())
          throw exc::CreateMessageException();

        messageChatDTOVector.push_back(messageChatDTO.value());

      } // if chat_ptr
      else
        throw exc::ChatNotFoundException();
    } // first if chat_ptr
  } // try
  catch (const exc::ChatNotFoundException &ex) {
    std::cerr << "Сервер: fillForSendAllMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  } catch (const exc::CreateMessageException &ex) {
    std::cerr << "Сервер: fillForSendAllMessageDTOFromSrv. " << ex.what() << std::endl;
    return std::nullopt;
  }
  return messageChatDTOVector;
}
//
//
//

// utilities

void ServerSession::runUDPServerDiscovery(std::uint16_t listenPort) {
  int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

  std::cout << "[DEBUG] runUDPServerDiscovery стартовал" << std::endl;

  try {
    if (udpSocket < 0)
      throw exc::CreateSocketTypeException();

    // ОБЯЗАТЕЛЬНО: разрешить reuse + broadcast
    int enable = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(listenPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
      std::cerr << "UDP: Failed to bind\n";
      close(udpSocket);
      return;
    }

    std::cout << "[UDP] Сервер слушает UDP discovery на порту " << listenPort << std::endl;

    while (true) {
      char buffer[128] = {0};
      sockaddr_in clientAddr{};
      socklen_t addrLen = sizeof(clientAddr);

      ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0,
                                       (sockaddr *)&clientAddr, &addrLen);

      if (bytesReceived > 0 && std::string(buffer) == "ping?") {
        const char *reply = "pong";
        sendto(udpSocket, reply, std::strlen(reply), 0, (sockaddr *)&clientAddr, addrLen);
      }
    }

    close(udpSocket);
  } catch (const exc::CreateSocketTypeException &ex) {
    std::cerr << "UDP: " << ex.what() << std::endl;
    close(udpSocket);
  }
}

bool ServerSession::createUserSrv(const UserDTO &userDTO) const {

  auto user = std::make_shared<User>(
      UserData(userDTO.login, userDTO.userName, userDTO.passwordhash, userDTO.email, userDTO.phone));

  user->createChatList(std::make_shared<UserChatList>(user));
  this->_instance.addUserToSystem(user);

  return true;
}
//
//
//
std::size_t ServerSession::createNewChatSrv(std::shared_ptr<Chat> &chat_ptr, ChatDTO chatDTO) const {

  auto newChatId = _instance.createNewChatId(chat_ptr);
  try {
    // инициализируем чат и находим юзера
    //   auto user_ptr = this->_instance.findUserByLogin(chatDTO.login);

    // добавляем поля
    // создаем глобальный номер чата
    chat_ptr->setChatId(newChatId);

    // добавляем участников
    // добавляем чат в чат лист каждому участнику

    for (const auto &participant : chatDTO.participants) {
      auto participant_ptr = this->_instance.findUserByLogin(participant.login);

      if (participant_ptr == nullptr) {
        throw exc::UserNotFoundException();
      } else {
        chat_ptr->addParticipant(participant_ptr, participant.lastReadMessage, participant.deletedFromChat);
      }
    }

    // добавляем чат в систему
    this->_instance.addChatToInstance(chat_ptr);

    // ⬇⬇⬇ Проверка
    // std::cout << "\n[DEBUG] Добавлен чат на сервер:\n";

    // const auto &chats = this->_instance.getChats();
    // std::shared_ptr<Chat> chat_ptr_test;

    // for (const auto &chat : chats)
    //   if (chat->getChatId() == newChatId) {
    //     chat_ptr_test = chat;
    //   }

    // if (!chat_ptr_test)
    //   throw exc::InternalDataErrorException();

    // std::cout << "участники:" << std::endl;
    // for (const auto &participant : chat_ptr->getParticipants()) {
    //   auto user = participant._user.lock();
    //   if (user) {
    //     std::cout << "- " << user->getLogin() << " | deleted: " << participant._deletedFromChat << std::endl;
    //   } else {
    //     throw exc::UserNotFoundException();
    //   }
    // }
  } // try
  catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: createNewChatSrv Пользователя уже удалили. " << ex.what() << std::endl;
    _instance.moveToFreeChatIdSrv(newChatId);
    return 0;
  }
  return newChatId;
}
//
//
//
std::size_t ServerSession::createNewMessageSrv(const MessageDTO &messageDTO) const {
  try {
    const auto &chat_ptr = this->_instance.getChatById(messageDTO.chatId);
    if (!chat_ptr)
      throw exc::ChatNotFoundException();

    auto sender_ptr = this->_instance.findUserByLogin(messageDTO.senderLogin);
    if (!sender_ptr)
      throw exc::UserNotFoundException();

    if (messageDTO.messageContent.empty())
      throw exc::CreateMessageException();

    auto message_ptr = std::make_shared<Message>(
        createOneMessage(messageDTO.messageContent[0].payload, sender_ptr, messageDTO.timeStamp, 0));

    chat_ptr->addMessageToChat(message_ptr, sender_ptr, true);

    const auto &t = message_ptr->getMessageId();

    return t;

  } catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Сервер: createNewMessageSrv: пользователь не найден: " << ex.what() << std::endl;
    return 0;
  } catch (const exc::ChatNotFoundException &ex) {
    std::cerr << "Сервер: createNewMessageSrv: чат не найден: " << ex.what() << std::endl;
    return 0;
  } catch (const exc::CreateMessageException &ex) {
    std::cerr << "Сервер: createNewMessageSrv: сообщение пустое: " << ex.what() << std::endl;
    return 0;
  }
}

//
//
//
bool ServerSession::createNewMessageChatSrv(std::shared_ptr<Chat> &chat, MessageChatDTO &messageChatDTO) {
  // добавляем сообщения из чата
  for (auto &messageDTO : messageChatDTO.messageDTO) {
    messageDTO.chatId = messageChatDTO.chatId;
    createNewMessageSrv(messageDTO);
  }
  return true;
}
//
//
// формируем и отдаем в ответ на запрос юзера, чаты и сообщения
std::optional<PacketListDTO> ServerSession::registerOnDeviceDataSrv(const std::shared_ptr<User> &user) {

  PacketListDTO packetListDTO;

  const auto login = user->getLogin();

  UserDTO userDTO;
  userDTO = FillForSendUserDTOFromSrv(login, true);

  // формируем пользователя
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  packetListDTO.packets.push_back(packetDTO);

  // формируем чаты
  //   std::vector<ChatDTO> chatDTO;
  //   std::unordered_set<std::shared_ptr<User>> participantLogins;

  auto chatDTOVector = FillForSendAllChatDTOFromSrv(user);

  if (!chatDTOVector.has_value())
    return std::nullopt;

  for (const auto &pct : chatDTOVector.value()) {
    PacketDTO packetDTO;
    packetDTO.requestType = RequestType::RqFrClientRegisterUser;
    packetDTO.structDTOClassType = StructDTOClassType::chatDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;
    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(pct);

    packetListDTO.packets.push_back(packetDTO);
  }

  // формируем сообщения

  const auto &messageChatDTO = fillForSendAllMessageDTOFromSrv(user);

  if (!messageChatDTO.has_value())
    return std::nullopt;

  for (const auto &pct : messageChatDTO.value()) {
    PacketDTO packetDTO;
    packetDTO.requestType = RequestType::RqFrClientRegisterUser;
    packetDTO.structDTOClassType = StructDTOClassType::messageChatDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;
    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(pct);

    packetListDTO.packets.push_back(packetDTO);
  }

  for (std::size_t i = 0; i < packetListDTO.packets.size(); ++i) {
    std::cerr << "[PACKET " << i << "] type = " << static_cast<int>(packetListDTO.packets[i].structDTOClassType)
              << std::endl;
  }
  return packetListDTO;
}