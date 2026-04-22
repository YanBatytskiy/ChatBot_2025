#include "server_session.h"
#include "dto/dto_struct.h"
#include "exceptions_cpp/login_exception.h"
#include "exceptions_cpp/network_exception.h"
#include "exceptions_cpp/sql_exception.h"
#include "exceptions_cpp/validation_exception.h"
#include "message/message.h"
#include "sql_commands/sql_commands.h"
#include "session_transport/session_transport.h"
#include "processors/user_account_update_processor.h"
#include "processors/user_ban_block_processor.h"
#include "processors/user_database_init_processor.h"
#include "processors/user_object_creation_processor.h"
#include "processors/user_registration_processor.h"
#include "processors/user_data_query_processor.h"
#include "system/serialize.h"
#include "system/system_function.h"
#include <arpa/inet.h>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <libpq-fe.h>
#include <memory>
#include <optional>
#include <utility>
#include <unistd.h>
#include <vector>

ServerSession::ServerSession(SQLRequests sql_requests)
    : packet_parser_(),
      _serverConnectionConfig(),
      _pqConnection(nullptr),
      sql_requests_(std::move(sql_requests)),
      user_sql_reader_(sql_requests_),
      user_sql_writer_(sql_requests_),
      chat_sql_reader_(sql_requests_),
      chat_sql_writer_(sql_requests_),
      message_sql_writer_(sql_requests_),
      database_sql_manager_(sql_requests_),
      user_ban_block_processor_(user_sql_writer_),
      user_account_update_processor_(user_sql_writer_),
      user_database_init_processor_(database_sql_manager_),
      user_registration_processor_(user_sql_reader_, message_sql_writer_),
      user_object_creation_processor_(user_sql_writer_, chat_sql_writer_,
                                      message_sql_writer_),
      user_data_query_processor_(user_sql_reader_) {}

// getters
PGconn *ServerSession::getPGConnection() { return _pqConnection; }
const PGconn *ServerSession::getPGConnection() const { return _pqConnection; }

ServerConnectionConfig &ServerSession::getServerConnectionConfig() {
  return _serverConnectionConfig;
}

// setters
void ServerSession::setPgConnection(PGconn *connection) { _pqConnection = connection; }

//
//
// transport

void ServerSession::ProcessIncoming(SessionTransport &transport) {
  try {
    if (!transport.IsConnected()) {
      throw exc::SocketInvalidException();
    }

    auto buffer = transport.ReceiveFrame();
    auto parsed = packet_parser_.ParseIncoming(buffer);
    auto packetListReceived = std::move(parsed.payload);

    routingRequestsFromClient(transport, packetListReceived,
                              parsed.request_type, transport.Connection());
  } catch (const exc::SocketInvalidException &ex) {
    std::cerr << "Сервер: " << ex.what() << " = "
              << transport.Connection() << std::endl;
    transport.Reset();
  } catch (const exc::CreateBufferException &ex) {
    std::cerr << "Сервер: " << ex.what() << " = " << MESSAGE_LENGTH
              << std::endl;
  } catch (const exc::ReceiveDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    transport.Reset();
  } catch (const exc::HeaderWrongTypeException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    transport.Reset();
  } catch (const exc::HeaderWrongDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    transport.Reset();
  }
}
//
//
//
bool ServerSession::sendPacketListDTO(SessionTransport &transport,
                                      PacketListDTO &packetListForSend,
                                      int connection) {
  try {
    auto packetBinary = serializePacketList(packetListForSend.packets);

    if (packetBinary.empty()) {
      throw exc::SendDataException();
    }

    transport.SendFrame(packetBinary, connection);
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер sendPacketListDTO: " << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cerr << "Сервер: Неизвестная ошибка. sendPacketListDTO"
              << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
// Request processing

bool ServerSession::routingRequestsFromClient(SessionTransport &transport,
                                              PacketListDTO &packetListReceived,
                                              const RequestType &requestType,
                                              int connection) {

  const auto packetDTOrequestType = packetListReceived.packets[0].requestType;

  // check and registry User
  switch (packetDTOrequestType) {
  case RequestType::RqFrClientChangeUserData:
  case RequestType::RqFrClientChangeUserPassword: {
    if (!user_account_update_processor_.Process(*this, transport,
                                                packetListReceived,
                                                packetDTOrequestType,
                                                connection))
      return false;
    break;
  }

  case RequestType::RqFrClientReInitializeBase: {
    if (!user_database_init_processor_.Process(*this, transport,
                                               packetListReceived,
                                               packetDTOrequestType,
                                               connection))
      return false;
    break;
  }
  case RequestType::RqFrClientCheckLogin:
  case RequestType::RqFrClientCheckLogPassword:
  case RequestType::RqFrClientRegisterUser:
  case RequestType::RqFrClientFindUserByPart:
  case RequestType::RqFrClientSetLastReadMessage:
  case RequestType::RqFrClientFindUserByLogin: {
    if (!user_registration_processor_.Process(*this, transport,
                                              packetListReceived,
                                              packetDTOrequestType,
                                              connection))
      return false;
    break;
  }
  // create objects
  case RequestType::RqFrClientCreateUser:
  case RequestType::RqFrClientCreateChat:
  case RequestType::RqFrClientCreateMessage: {
    if (!user_object_creation_processor_.Process(*this, transport,
                                                 packetListReceived,
                                                 packetDTOrequestType,
                                                 connection))
      return false;
    break;
  }
    // get indexes and user Data
  case RequestType::RqFrClientGetUsersData: {
    if (!user_data_query_processor_.Process(*this, transport,
                                            packetListReceived,
                                            packetDTOrequestType,
                                            connection))
      return false;
    break;
  }
  case RequestType::RqFrClientBlockUser:
  case RequestType::RqFrClientUnBlockUser:
  case RequestType::RqFrClientBunUser:
  case RequestType::RqFrClientUnBunUser: {
    if (!user_ban_block_processor_.Process(*this, transport,
                                           packetListReceived,
                                           packetDTOrequestType,
                                           connection))
      return false;
    break;
  }
  default:
    break;
  }
  return true;
}

std::optional<UserDTO> ServerSession::FillForSendUserDTOFromSrvSQL(const std::string &login, bool loginUser) {
  (void)loginUser;
  return user_sql_reader_.GetUserWithPasshashSQL(login,
                                                 this->getPGConnection());
}

std::optional<std::vector<UserDTO>> ServerSession::FillForSendSeveralUsersDTOFromSrvSQL(
  const std::vector<std::string> logins) {
  auto value =
      user_sql_reader_.GetSeveralUsersSQL(logins, this->getPGConnection());

  if (!value.has_value())
    return std::nullopt;

  return value;
}

//
//
//
// получаем один чат пользователя
std::optional<ChatDTO> ServerSession::FillForSendOneChatDTOFromSrvSQL(const std::string &chat_id,
                                                                      const std::string &login) {
  ChatDTO chatDTO;

  // взяли chatId  и логин
  chatDTO.chatId = static_cast<std::size_t>(std::stoull(chat_id));
  chatDTO.senderLogin = login;

  // получаем список участников
  auto participants = chat_sql_reader_.GetChatParticipantsSQL(
      chat_id, this->getPGConnection());

  try {

    if (!participants.has_value()) {
      throw exc::ChatListNotFoundException(login);
    }

    auto deletedMessagesMultiset =
        chat_sql_reader_.GetChatMessagesDeletedStatusSQL(
            chat_id, this->getPGConnection());

    if (deletedMessagesMultiset.has_value() && deletedMessagesMultiset.value().size() > 0) {

      // перебираем участников
      for (auto &participant : participants.value()) {

        // берем конкретного участника
        const auto &participantLogin = participant.login;

        // берем массив его значений
        const auto &range = deletedMessagesMultiset.value().equal_range({participantLogin, 0});

        // перебираем все сообщения пользователя и добавляем его в вектор для отправки
        if (std::distance(range.first, range.second)) {
          for (auto it = range.first; it != range.second; ++it) {

            // заполняем deletedMessageIds
            participant.deletedMessageIds.push_back(it->second);
          } // for range

        } // if distance

      } // for participant

    } // if deletedMessagesMultiset

  } // try
  catch (const exc::ChatListNotFoundException &ex) {
    std::cerr << "Сервер: FillForSendOneChatDTOFromSrvSQL. " << ex.what() << std::endl;
    return std::nullopt;
  }

  chatDTO.participants = participants.value();

  return chatDTO;
}
//
//
// получаем все чаты пользователя
std::optional<std::vector<ChatDTO>> ServerSession::FillForSendAllChatDTOFromSrvSQL(const std::string &login) {

  // взяли чат лист
  auto chatList =
      chat_sql_reader_.GetChatIdsForUserSQL(login, this->getPGConnection());

  if (chatList.size() == 0)
    return std::nullopt;

  std::vector<ChatDTO> chatDTOResultVector;

  // перебираем чаты в чат листе
  for (const auto &chat : chatList) {

    auto tempDTO = FillForSendOneChatDTOFromSrvSQL(chat, login);

    if (tempDTO.has_value())
      chatDTOResultVector.push_back(tempDTO.value());
    else {
      std::cerr << "Сервер: FillForSendAllChatDTOFromSrvSQL. Chat_id " << chat << " не заполнен" << std::endl;
      continue;
    }
  } // first for

  return chatDTOResultVector;
}
//
//
// получаем сообщения пользователя конкретного чата
std::optional<MessageChatDTO> ServerSession::fillForSendChatMessageDTOFromSrvSQL(const std::string &chat_id) {
  auto messageChatDTO =
      chat_sql_reader_.GetChatMessagesSQL(chat_id, this->getPGConnection());

  if (!messageChatDTO.has_value())
    return std::nullopt;

  return messageChatDTO.value();
}
//
//
// получаем все сообщения пользователя
std::optional<std::vector<MessageChatDTO>> ServerSession::fillForSendAllMessageDTOFromSrvSQL(const std::string login) {

  std::vector<MessageChatDTO> messageChatDTOVector;

  // взяли чат лист
  auto chatList =
      chat_sql_reader_.GetChatIdsForUserSQL(login, this->getPGConnection());

  if (chatList.size() == 0)
    return std::nullopt;

  // перебираем чаты в чат листе
  for (const auto &chat_id : chatList) {

    const auto &messageChatDTO = fillForSendChatMessageDTOFromSrvSQL(chat_id);

    if (messageChatDTO.has_value())
      messageChatDTOVector.push_back(messageChatDTO.value());
  } // for

  if (messageChatDTOVector.size() == 0)
    return std::nullopt;
  else
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

      ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&clientAddr, &addrLen);

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
//
//
// формируем и отдаем в ответ на запрос юзера, чаты и сообщения
std::optional<PacketListDTO> ServerSession::registerOnDeviceDataSrvSQL(const std::string login) {

  PacketListDTO packetListDTO;

  //   const auto login = user->getLogin();

  UserDTO userDTO;

  auto const &tempUserDTO = FillForSendUserDTOFromSrvSQL(login, true);

  if (tempUserDTO.has_value())
    userDTO = tempUserDTO.value();
  else
    return std::nullopt;

  // формируем пользователя
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  packetListDTO.packets.push_back(packetDTO);

  // формируем чаты

  auto chatDTOVector = FillForSendAllChatDTOFromSrvSQL(login);

  if (chatDTOVector.has_value()) {

    for (const auto &pct : chatDTOVector.value()) {
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientRegisterUser;
      packetDTO.structDTOClassType = StructDTOClassType::chatDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(pct);

      packetListDTO.packets.push_back(packetDTO);
    }
  }

  // формируем сообщения

  const auto &messageChatDTO = fillForSendAllMessageDTOFromSrvSQL(login);

  if (messageChatDTO.has_value()) {

    for (const auto &pct : messageChatDTO.value()) {
      PacketDTO packetDTO;
      packetDTO.requestType = RequestType::RqFrClientRegisterUser;
      packetDTO.structDTOClassType = StructDTOClassType::messageChatDTO;
      packetDTO.reqDirection = RequestDirection::ClientToSrv;
      packetDTO.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(pct);

      packetListDTO.packets.push_back(packetDTO);
    }
  }

  for (std::size_t i = 0; i < packetListDTO.packets.size(); ++i) {
    std::cerr << "[PACKET " << i << "] type = " << static_cast<int>(packetListDTO.packets[i].structDTOClassType)
              << std::endl;
  }
  return packetListDTO;
}
