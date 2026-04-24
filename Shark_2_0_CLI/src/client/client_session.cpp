#include "client_session.h"
#include "chat/chat.h"
#include "chat_system/chat_system.h"
#include "dto/dto_struct.h"
#include "exception/login_exception.h"
#include "exception/network_exception.h"
#include "message/message.h"
#include "message/message_content.h"
#include "message/message_content_struct.h"
#include "system/serialize.h"
#include "system/system_function.h"
#include "user/user.h"
#include "user/user_chat_list.h"

#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>

struct sockaddr_in serveraddress, client;

ClientSession::ClientSession(ChatSystem &client) : _instance(client), _socketFd() {}

// getters

ServerConnectionConfig &ClientSession::getserverConnectionConfigCl() {
  return _serverConnectionConfig;
}

const ServerConnectionConfig &ClientSession::getserverConnectionConfigCl() const {
  return _serverConnectionConfig;
}

ServerConnectionMode &ClientSession::getserverConnectionModeCl() {
  return _serverConnectionMode;
}

const ServerConnectionMode &ClientSession::getserverConnectionModeCl() const {
  return _serverConnectionMode;
}

const std::shared_ptr<User> ClientSession::getActiveUserCl() const {
  return _instance.getActiveUser();
}

ChatSystem &ClientSession::getInstance() {
  return _instance;
}

const int &ClientSession::getSocketFd() const {
  return _socketFd;
}

// setters

void ClientSession::setActiveUserCl(const std::shared_ptr<User> &user) {
  _instance.setActiveUser(user);
}

void ClientSession::setSocketFd(const int &socketFd) {
  _socketFd = socketFd;
}

//
//
//
// checking and finding
//
//
//
const std::vector<UserDTO> ClientSession::findUserByTextPartOnServerCl(const std::string &textToFind) {

  UserLoginDTO userLoginDTO;
  userLoginDTO.login = textToFind;

  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientFindUserByPart;
  packetDTO.structDTOClassType = StructDTOClassType::userLoginDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(userLoginDTO);

  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTO);

  PacketListDTO responcePacketListDTO;
  responcePacketListDTO.packets.clear();

  responcePacketListDTO = processingRequestToServer(packetDTOListSend, packetDTO.requestType);

  std::vector<UserDTO> result;
  result.clear();

  try {
    if (!responcePacketListDTO.packets.empty()) {

      for (const auto &packet : responcePacketListDTO.packets) {

        if (packet.requestType != RequestType::RqFrClientFindUserByPart)
          throw exc::WrongresponceTypeException();
        else {
          const auto &packetUserDTO = static_cast<const StructDTOClass<UserDTO> &>(*packet.structDTOPtr)
                                        .getStructDTOClass();
          result.push_back(packetUserDTO);
        }
      }
    } else
      throw exc::WrongPacketSizeException();

  } catch (const exc::WrongresponceTypeException &ex) {
    std::cout << "Client. Search users by substring. " << ex.what() << std::endl;
    result.clear();
  } catch (const std::exception &ex) {
    std::cout << "Client. Unknown error. " << ex.what() << std::endl;
    result.clear();
  }

  return result;
}
//
//
//
bool ClientSession::checkUserLoginCl(const std::string &userLogin) {

  const auto isOnClientDevice = _instance.findUserByLogin(userLogin);

  if (isOnClientDevice != nullptr)
    return true;

  UserLoginDTO userLoginDTO;
  userLoginDTO.login = userLogin;

  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientCheckLogin;
  packetDTO.structDTOClassType = StructDTOClassType::userLoginDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(userLoginDTO);

  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTO);

  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  packetListDTOresult = processingRequestToServer(packetDTOListSend, packetDTO.requestType);

  //  std::vector<PacketDTO> responcePacketListDTO;
  try {
    if (packetListDTOresult.packets.size() != 1)
      throw exc::WrongPacketSizeException();

    if (packetListDTOresult.packets[0].requestType != RequestType::RqFrClientCheckLogin)
      throw exc::WrongresponceTypeException();
    else {
      const auto &responceDTO = static_cast<const StructDTOClass<ResponceDTO> &>(
                                  *packetListDTOresult.packets[0].structDTOPtr)
                                  .getStructDTOClass();

      return responceDTO.reqResult;
    }

  } catch (const exc::WrongPacketSizeException &ex) {
    std::cout << "Client. Login check. Wrong number of packets in response." << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cout << "Client. Unknown error. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ClientSession::checkUserPasswordCl(const std::string &userLogin, const std::string &passwordHash) {

  const auto isOnClientDevice = _instance.findUserByLogin(userLogin);

  if (isOnClientDevice != nullptr)
    return _instance.checkPasswordValidForUser(passwordHash, userLogin);

  UserLoginPasswordDTO userLoginPasswordDTO;
  userLoginPasswordDTO.login = userLogin;
  userLoginPasswordDTO.passwordhash = passwordHash;

  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientCheckLogPassword;
  packetDTO.structDTOClassType = StructDTOClassType::userLoginPasswordDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;

  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(userLoginPasswordDTO);

  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTO);

  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  packetListDTOresult = processingRequestToServer(packetDTOListSend, packetDTO.requestType);

  try {
    if (packetListDTOresult.packets.size() != 1)
      throw exc::WrongPacketSizeException();

    if (packetListDTOresult.packets[0].requestType != RequestType::RqFrClientCheckLogPassword)
      throw exc::WrongresponceTypeException();
    else {
      const auto &responceDTO = static_cast<const StructDTOClass<ResponceDTO> &>(
                                  *packetListDTOresult.packets[0].structDTOPtr)
                                  .getStructDTOClass();

      return responceDTO.reqResult;
    }

  } catch (const exc::WrongPacketSizeException &ex) {
    std::cout << "Client. Password check. Wrong number of packets in response." << ex.what() << std::endl;
    return false;
  } catch (const std::exception &ex) {
    std::cout << "Client. Unknown error. " << ex.what() << std::endl;
    return false;
  }
}
// transport
//
//
void ClientSession::reidentifyClientAfterConnection() {

  UserLoginDTO userLoginDTO;
  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientUserConnectMake;
  packetDTO.structDTOClassType = StructDTOClassType::userLoginDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  ;

  if (_instance.getActiveUser())
    userLoginDTO.login = _instance.getActiveUser()->getLogin();
  else
    userLoginDTO.login = "";

  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(userLoginDTO);

  std::vector<PacketDTO> packets{packetDTO};
  processingRequestToServer(packets, packetDTO.requestType);
}
//
//
//
bool ClientSession::findServerAddress(ServerConnectionConfig &serverConnectionConfig,
                                      ServerConnectionMode &serverConnectionMode) {

  // Open a socket for discovery
  int socketTmp = (socket(AF_INET, SOCK_STREAM, 0));

  try {
    if (socketTmp == -1) {
      throw exc::CreateSocketTypeException();
    } else {
      // First try to find the server on the local machine
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(serverConnectionConfig.port);

      addr.sin_addr.s_addr = inet_addr(serverConnectionConfig.addressLocalHost.c_str());

      int result = connect(socketTmp, (sockaddr *)&addr, sizeof(addr));

      if (result == 0) {
        serverConnectionConfig.found = true;
        serverConnectionMode = ServerConnectionMode::Localhost;

        close(socketTmp);
        return true;
      }

      // Then try to discover the server on the LAN
      discoverServerOnLAN(serverConnectionConfig);
      if (serverConnectionConfig.found) {
        std::cout << "Server found. " << serverConnectionConfig.addressLocalNetwork << ":"
                  << serverConnectionConfig.port << std::endl;

        serverConnectionMode = ServerConnectionMode::LocalNetwork;
        close(socketTmp);
        return true;
      }

      if (!serverConnectionConfig.addressInternet.empty()) {
        // Look up the server on the Internet
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int gaiResult = getaddrinfo(serverConnectionConfig.addressInternet.c_str(), nullptr, &hints, &res);
        if (gaiResult != 0 || res == nullptr) {
          addr.sin_addr.s_addr = INADDR_NONE;
        } else {
          sockaddr_in *ipv4 = (sockaddr_in *)res->ai_addr;
          addr.sin_addr = ipv4->sin_addr;
          freeaddrinfo(res);
        }

        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverConnectionConfig.port);

        result = connect(socketTmp, (sockaddr *)&addr, sizeof(addr));
        if (result == 0) {
          std::cout << "Server found. " << serverConnectionConfig.addressInternet << ":"
                    << serverConnectionConfig.port << std::endl;
          serverConnectionConfig.found = true;
          serverConnectionMode = ServerConnectionMode::Internet;
          close(socketTmp);
          return true;
        }
      }
    }
  } // try
  catch (const exc::CreateSocketTypeException &ex) {
    std::cerr << "Client: " << ex.what() << std::endl;
  }

  close(socketTmp);

  std::cout << "Server not found anywhere. " << std::endl;
  return false;
}
//
//
//

int ClientSession::createConnection(ServerConnectionConfig &serverConnectionConfig,
                                    ServerConnectionMode &serverConnectionMode) {

  // Set the port number
  serveraddress.sin_port = htons(serverConnectionConfig.port);
  // Use IPv4
  serveraddress.sin_family = AF_INET;

  // Set the server address
  switch (serverConnectionMode) {
    case ServerConnectionMode::Localhost: {
      serveraddress.sin_addr.s_addr = inet_addr(serverConnectionConfig.addressLocalHost.c_str());
      break;
    }
    case ServerConnectionMode::LocalNetwork: {
      addrinfo hints{}, *res = nullptr;
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;

      int gaiResult = getaddrinfo(serverConnectionConfig.addressLocalNetwork.c_str(), nullptr, &hints, &res);
      if (gaiResult != 0 || res == nullptr) {
        serveraddress.sin_addr.s_addr = INADDR_NONE;
      } else {
        sockaddr_in *ipv4 = (sockaddr_in *)res->ai_addr;
        serveraddress.sin_addr = ipv4->sin_addr;
        freeaddrinfo(res);
      }
      break;
    }
    case ServerConnectionMode::Internet: {
      if (serverConnectionConfig.addressInternet.empty())
        return -1;

      serveraddress.sin_addr.s_addr = inet_addr(serverConnectionConfig.addressInternet.c_str());
      break;
    }
    default:
      break;
  }

  // Open a socket for discovery
  int socketTmp = (socket(AF_INET, SOCK_STREAM, 0));

  try {
    if (socketTmp == -1)
      throw exc::CreateSocketTypeException();
    else {
      if (connect(socketTmp, (sockaddr *)&serveraddress, sizeof(serveraddress)) < 0)
        throw exc::ConnectionToServerException();
      else
        _socketFd = socketTmp;
    }
  } // try
  catch (const exc::NetworkException &ex) {
    std::cerr << "Client: " << ex.what() << std::endl;
    close(socketTmp);
    return -1;
  }
  return _socketFd;
}
//
//
//
bool ClientSession::discoverServerOnLAN(ServerConnectionConfig &serverConnectionConfig) {
  int timeoutMs = 1000;

  try {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
      throw exc::CreateSocketTypeException();
    }

    // Enable broadcast
    int broadcastEnable = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    // Set a timeout
    timeval timeout{};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
    setsockopt(udpSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in broadcastAddr{};
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_port = htons(serverConnectionConfig.port);
    broadcastAddr.sin_addr.s_addr = inet_addr("255.255.255.255");

    std::string ping = "ping?";
    sendto(udpSocket, ping.c_str(), ping.size(), 0, (sockaddr *)&broadcastAddr, sizeof(broadcastAddr));

    char buffer[128] = {0};
    sockaddr_in serverAddr{};
    socklen_t addrLen = sizeof(serverAddr);
    ssize_t bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer) - 1, 0, (sockaddr *)&serverAddr, &addrLen);

    if (bytesReceived > 0) {
      std::string msg(buffer);
      if (msg == "pong") {
        serverConnectionConfig.addressLocalNetwork = inet_ntoa(serverAddr.sin_addr);
        serverConnectionConfig.found = true;
      }
    }

    close(udpSocket);
    return true;
  } catch (const exc::CreateSocketTypeException &ex) {
    std::cerr << "UDP: " << ex.what() << std::endl;
    serverConnectionConfig.found = false;
    return false;
  }
}
//
//
//
bool ClientSession::checkResponceServer() {
  // Check the connection
  int error = 0;
  socklen_t len = sizeof(error);
  int result = getsockopt(_socketFd, SOL_SOCKET, SO_ERROR, &error, &len);

  try {
    if (result != 0 || error != 0) {

      int resultConnection = createConnection(getserverConnectionConfigCl(), getserverConnectionModeCl());
      if (resultConnection <= 0)
        throw exc::LostConnectionException();
    }

  } catch (const exc::LostConnectionException &ex) {
    std::cerr << "Client: " << ex.what() << std::endl;

    return false;
  }
  return true;
}
//
//
//
PacketListDTO ClientSession::getDatafromServer(const std::vector<std::uint8_t> &packetListSend) {

  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  if (!checkResponceServer())
    return packetListDTOresult;

  // debug check
  // for (std::uint8_t b : packetListSend) {
  //   std::cerr << static_cast<int>(b) << " ";
  // }
  // std::cerr << std::endl;

  try {

    // 1. Prepend 4 bytes of length
    std::vector<std::uint8_t> packetWithSize;
    uint32_t len = htonl(packetListSend.size());

    packetWithSize.resize(4 + packetListSend.size());
    std::memcpy(packetWithSize.data(), &len, 4);                                          // first 4 bytes: length
    std::memcpy(packetWithSize.data() + 4, packetListSend.data(), packetListSend.size()); // then the data

    // 2. Send
    // Drain the receive buffer before sending the request

    // Save the current socket mode
    int flags = fcntl(_socketFd, F_GETFL, 0);
    fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK);

    // Drain the incoming buffer
    std::vector<std::uint8_t> drainBuf(4096);
    while (recv(_socketFd, drainBuf.data(), drainBuf.size(), 0) > 0) {
    }

    // Restore the original mode
    fcntl(_socketFd, F_SETFL, flags);

    ssize_t bytesSent = send(_socketFd, packetWithSize.data(), packetWithSize.size(), 0);

    if (bytesSent <= 0 || static_cast<std::size_t>(bytesSent) != packetWithSize.size())
      throw exc::SendDataException();

    // Receive the response
    len = 0;
    std::uint8_t lenBuf[4];
    std::size_t total = 0;
    while (total < 4) {
      ssize_t r = recv(_socketFd, lenBuf + total, 4 - total, 0);
      if (r <= 0)
        throw exc::ReceiveDataException();
      total += r;
    }

    std::memcpy(&len, lenBuf, 4);
    len = ntohl(len);

    std::vector<std::uint8_t> buffer(len);
    ssize_t bytesReceived = 0;

    while (bytesReceived < len) {
      ssize_t bytes = recv(_socketFd, buffer.data() + bytesReceived, len - bytesReceived, 0);
      if (bytes <= 0)
        throw exc::ReceiveDataException();

      bytesReceived += bytes;
    }

    // Manual receive-buffer diagnostics kept for transport troubleshooting.
    // std::cout << "[DEBUG] buffer.size() = " << buffer.size() << std::endl;
    // for (std::size_t i = 0; i < buffer.size(); ++i)
    //   std::cout << std::hex << static_cast<int>(buffer[i]) << " ";
    // std::cout << std::dec << std::endl;

    // Manual receive-size diagnostics kept for transport troubleshooting.
    // std::cerr << "[DEBUG] Received from server: " << bytesReceived << " bytes" << std::endl;
    // for (std::size_t i = 0; i < buffer.size(); ++i) {
    //   std::cerr << static_cast<int>(buffer[i]) << " ";
    // }
    // std::cerr << std::endl;

    std::vector<PacketDTO> responcePacketListDTOVector;
    responcePacketListDTOVector = deSerializePacketList(buffer);

    for (const auto &pct : responcePacketListDTOVector)
      packetListDTOresult.packets.push_back(pct);

    // for (const auto &packet : packetListDTOresult.packets) {
    //   std::cerr << "[CLIENT DEBUG] Received packet requestType = " << static_cast<int>(packet.requestType) <<
    //   std::endl;
    // }

    if (packetListDTOresult.packets.empty())
      throw exc::ReceiveDataException();
  } // try
  catch (const exc::SendDataException &ex) {
    std::cerr << "Client getDatafromServer: " << ex.what() << std::endl;
    packetListDTOresult.packets.clear();
    return packetListDTOresult;
  } catch (const exc::ReceiveDataException &ex) {
    std::cerr << "Client getDatafromServer: " << ex.what() << std::endl;
    packetListDTOresult.packets.clear();
    return packetListDTOresult;
  }
  return packetListDTOresult;
}
//
//
PacketListDTO ClientSession::processingRequestToServer(std::vector<PacketDTO> &packetDTOListVector,
                                                       const RequestType &requestType) {

  PacketListDTO packetListDTOForSend;
  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  try {
    // Add the header
    UserLoginPasswordDTO userLoginPasswordDTO;

    if (_instance.getActiveUser())
      userLoginPasswordDTO.login = _instance.getActiveUser()->getLogin();
    else
      userLoginPasswordDTO.login = "!";
    userLoginPasswordDTO.passwordhash = "UserHeder";

    PacketDTO packetDTO;
    packetDTO.requestType = requestType;
    packetDTO.structDTOClassType = StructDTOClassType::userLoginPasswordDTO;
    packetDTO.reqDirection = RequestDirection::ClientToSrv;
    packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(userLoginPasswordDTO);

    packetListDTOForSend.packets.push_back(packetDTO);

    for (const auto &packetDTO : packetDTOListVector)
      packetListDTOForSend.packets.push_back(packetDTO);

    // Manual request-serialization diagnostics kept for transport troubleshooting.
    // std::cout << "[DEBUG] Serializing reqDirection = "
    // << static_cast<int>(packetDTO.reqDirection) << std::endl;

    auto packetListBinarySend = serializePacketList((packetListDTOForSend.packets));

    // Sent the request and received the server response
    packetListDTOresult = getDatafromServer(packetListBinarySend);

  } // try
  catch (const exc::LostConnectionException &ex) {
    std::cerr << "Client processingRequestToServer: " << ex.what() << std::endl;
    packetListDTOresult.packets.clear();
  }
  return packetListDTOresult;
}

//
//
//
// utilities

void ClientSession::resetSessionData() {
  _instance = ChatSystem(); // recreate the entire chatSystem (users, chats, id)
}
//
//
//
bool ClientSession::registerClientToSystemCl(const std::string &login) {
  // In this method we send the login to the server and receive
  // the user's data along with all their chats and messages

  UserLoginDTO userLoginDTO;
  userLoginDTO.login = login;
  PacketDTO packetDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOClassType = StructDTOClassType::userLoginDTO;

  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(userLoginDTO);

  const auto isOnClientDevice = _instance.findUserByLogin(login);

  // if (isOnClientDevice != nullptr) {
  //   _instance.setActiveUser(isOnClientDevice);
  //   packetDTO.requestType = RequestType::RqFrClientSynchroUser;
  // } else {
  packetDTO.requestType = RequestType::RqFrClientRegisterUser;
  // }
  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTO);

  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  packetListDTOresult = processingRequestToServer(packetDTOListSend, packetDTO.requestType);

  try {

    for (const auto &packet : packetListDTOresult.packets) {
      // std::cout << "packet type: " << static_cast<int>(packet.requestTyfpe)
      //           << std::endl;
      if (packet.requestType != packetDTO.requestType)
        throw exc::WrongresponceTypeException();
    }

    for (const auto &packet : packetListDTOresult.packets) {

      switch (packet.structDTOClassType) {
        case StructDTOClassType::userDTO: {

          // Extract a reference to the structure from the pointer
          const auto &type = static_cast<const StructDTOClass<UserDTO> &>(*packet.structDTOPtr).getStructDTOClass();

          setActiveUserDTOFromSrv(type);
          break;
        }
        case StructDTOClassType::chatDTO: {
          // Extract a reference to the structure from the pointer
          const auto &type = static_cast<const StructDTOClass<ChatDTO> &>(*packet.structDTOPtr).getStructDTOClass();

          setOneChatDTOFromSrv(type);
          break;
        }
        case StructDTOClassType::messageChatDTO: {
          // Extract a reference to the structure from the pointer
          const auto &type = static_cast<const StructDTOClass<MessageChatDTO> &>(*packet.structDTOPtr)
                               .getStructDTOClass();

          setOneChatMessageDTO(type);
          break;
        }
        default:
          throw exc::WrongresponceTypeException();
      } // switch
    } // for

  } // try
  catch (const exc::NetworkException &ex) {
    std::cerr << "Client: device registration. " << ex.what() << std::endl;
    return false;
  }

  return true;
}

//
//
//

bool ClientSession::createUserCl(std::shared_ptr<User> &user) {

  UserDTO userDTO;

  userDTO.userName = user->getUserName();
  userDTO.login = user->getLogin();
  userDTO.passwordhash = user->getPasswordHash();
  userDTO.email = user->getEmail();
  userDTO.phone = user->getPhone();

  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientCreateUser;
  packetDTO.structDTOClassType = StructDTOClassType::userDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(userDTO);

  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTO);

  PacketListDTO packetListDTOresult;
  packetListDTOresult.packets.clear();

  packetListDTOresult = processingRequestToServer(packetDTOListSend, packetDTO.requestType);

  try {

    const auto &packet = static_cast<const StructDTOClass<ResponceDTO> &>(*packetListDTOresult.packets[0].structDTOPtr)
                           .getStructDTOClass();

    if (packet.reqResult)
      this->_instance.addUserToSystem(user);
    else
      throw exc::CreateUserException();
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Client: " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
bool ClientSession::createNewChatCl(std::shared_ptr<Chat> &chat) {

  // The logic is as follows:
  // build a packet containing the chat and message and send it to the server
  // in response we receive the chat and message IDs
  // if everything is fine we insert the chat and message into the system

  // Build the packet for sending the chat
  auto chatDTO = FillForSendOneChatDTOFromClient(chat);

  // Build the packet for sending the messages
  auto messageChatDTO = fillChatMessageDTOFromClient(chat);

  PacketDTO chatPacket;
  chatPacket.requestType = RequestType::RqFrClientCreateChat;
  chatPacket.structDTOClassType = StructDTOClassType::chatDTO;
  chatPacket.reqDirection = RequestDirection::ClientToSrv;
  chatPacket.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(chatDTO.value());

  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(chatPacket);

  PacketDTO messagePacket;
  messagePacket.requestType = RequestType::RqFrClientCreateChat;
  messagePacket.structDTOClassType = StructDTOClassType::messageChatDTO;
  messagePacket.reqDirection = RequestDirection::ClientToSrv;
  messagePacket.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(messageChatDTO);
  packetDTOListSend.push_back(messagePacket);

  PacketListDTO responcePacketListDTO;
  responcePacketListDTO.packets.clear();

  responcePacketListDTO = processingRequestToServer(packetDTOListSend, RequestType::RqFrClientCreateChat);

  try {
    if (responcePacketListDTO.packets.empty()) {
      throw exc::EmptyPacketException();
    } else {
      if (responcePacketListDTO.packets[0].requestType != RequestType::RqFrClientCreateChat)
        throw exc::WrongresponceTypeException();
      else {
        const auto &packetDTO = static_cast<const StructDTOClass<ResponceDTO> &>(
                                  *responcePacketListDTO.packets[0].structDTOPtr)
                                  .getStructDTOClass();

        if (!packetDTO.reqResult)
          throw exc::CreateChatException();
        else {
          //   std::cout << "[DEBUG] anyNumber = '" << packetDTO.anyNumber << "'" << std::endl;
          auto generalChatId = packetDTO.anyNumber;
          chat->setChatId(generalChatId);

          //   std::cout << "[DEBUG] anyString = '" << packetDTO.anyString << "'" << std::endl;

          auto generalMessageId = parseGetlineToSizeT(packetDTO.anyString);
          chat->getMessages().begin()->second->setMessageId(generalMessageId);

          if (chat->getMessages().empty())
            throw exc::CreateMessageException();

          // Add the chat to the system
          _instance.addChatToInstance(chat);
        }
      }
    }
  } // try
  catch (const exc::WrongresponceTypeException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  } catch (const exc::EmptyPacketException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  } catch (const exc::CreateChatException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  } catch (const exc::CreateChatIdException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  } catch (const exc::CreateMessageIdException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  } catch (const exc::CreateMessageException &ex) {
    std::cout << "Client. createNewChatCl. " << ex.what() << std::endl;
    return false;
  }

  return true;
}
//
//
//
MessageDTO ClientSession::fillOneMessageDTOFromCl(const std::shared_ptr<Message> &message, std::size_t chatId) {

  MessageDTO messageDTO;
  auto user_ptr = message->getSender().lock();

  messageDTO.chatId = chatId;
  messageDTO.messageId = message->getMessageId();
  messageDTO.senderLogin = user_ptr ? user_ptr->getLogin() : "";

  // Get the content
  MessageContentDTO temContent;
  temContent.messageContentType = MessageContentType::Text;
  if (message->getContent().empty())
    throw exc::UnknownException("Empty message content");
  auto contentElement = message->getContent().front();

  auto contentTextPtr = std::dynamic_pointer_cast<MessageContent<TextContent>>(contentElement);

  if (contentTextPtr) {
    auto contentText = contentTextPtr->getMessageContent();
    temContent.payload = contentText._text;
  }
  messageDTO.messageContent.push_back(temContent);

  messageDTO.timeStamp = message->getTimeStamp();

  return messageDTO;
}
//
//
//
bool ClientSession::createMessageCl(const Message &message, std::shared_ptr<Chat> &chat_ptr,
                                    const std::shared_ptr<User> &user) {

  auto message_ptr = std::make_shared<Message>(message);

  MessageDTO messageDTO = fillOneMessageDTOFromCl(message_ptr, chat_ptr->getChatId());

  PacketDTO packetDTO;
  packetDTO.requestType = RequestType::RqFrClientCreateMessage;
  packetDTO.structDTOClassType = StructDTOClassType::messageDTO;
  packetDTO.reqDirection = RequestDirection::ClientToSrv;
  packetDTO.structDTOPtr = std::make_shared<StructDTOClass<MessageDTO>>(messageDTO);

  std::vector<PacketDTO> packetDTOListForSendVector;
  packetDTOListForSendVector.push_back(packetDTO);

  PacketListDTO responcePacketListDTO;
  responcePacketListDTO.packets.clear();

  responcePacketListDTO = processingRequestToServer(packetDTOListForSendVector, packetDTO.requestType);

  try {

    if (responcePacketListDTO.packets.empty())
      throw exc::EmptyPacketException();

    if (responcePacketListDTO.packets.size() > 1)
      throw exc::WrongPacketSizeException();

    if (responcePacketListDTO.packets[0].requestType != RequestType::RqFrClientCreateMessage)
      throw exc::WrongresponceTypeException();

    // Extracted the packet
    const auto &packetDTO = static_cast<const StructDTOClass<ResponceDTO> &>(
                              *responcePacketListDTO.packets[0].structDTOPtr)
                              .getStructDTOClass();

    if (!packetDTO.reqResult)
      throw exc::CreateMessageException();
    else {
      auto newMessageId = parseGetlineToSizeT(packetDTO.anyString);

      if (chat_ptr->getMessages().empty())
        throw exc::CreateMessageException();

      message_ptr->setMessageId(newMessageId);

      chat_ptr->addMessageToChat(message_ptr, user, false);
    }
  } catch (const exc::EmptyPacketException &ex) {
    std::cerr << "Client: createMessageCl" << ex.what() << std::endl;
    return false;
  } catch (const exc::WrongPacketSizeException &ex) {
    std::cerr << "Client: createMessageCl" << ex.what() << std::endl;
    return false;
  } catch (const exc::WrongresponceTypeException &ex) {
    std::cerr << "Client: createMessageCl" << ex.what() << std::endl;
    return false;

  } catch (const exc::CreateMessageException &ex) {
    std::cerr << "Client: createMessageCl" << ex.what() << std::endl;
    return false;
  } catch (const exc::ValidationException &ex) {
    std::cerr << "Client: createMessageCl" << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
//
// Send a LastReadMessage packet
bool ClientSession::sendLastReadMessageFromClient(const std::shared_ptr<Chat> &chat_ptr, std::size_t messageId) {
  // Packet to send
  MessageDTO messageDTO;
  messageDTO.chatId = chat_ptr->getChatId();
  messageDTO.messageId = messageId;
  messageDTO.senderLogin = _instance.getActiveUser()->getLogin();
  messageDTO.messageContent = {};
  messageDTO.timeStamp = 0;

  // Standalone packet to send
  PacketDTO packetDTOForSend;
  packetDTOForSend.requestType = RequestType::RqFrClientSetLastReadMessage;
  packetDTOForSend.structDTOClassType = StructDTOClassType::messageDTO;
  packetDTOForSend.reqDirection = RequestDirection::ClientToSrv;
  packetDTOForSend.structDTOPtr = std::make_shared<StructDTOClass<MessageDTO>>(messageDTO);

  // Built the vector of packets to send
  std::vector<PacketDTO> packetDTOListSend;
  packetDTOListSend.push_back(packetDTOForSend);

  PacketListDTO responcePacketListDTO;
  responcePacketListDTO.packets.clear();

  responcePacketListDTO = processingRequestToServer(packetDTOListSend, RequestType::RqFrClientSetLastReadMessage);

  try {
    if (responcePacketListDTO.packets.empty()) {
      throw exc::EmptyPacketException();
    } else {
      if (responcePacketListDTO.packets[0].requestType != RequestType::RqFrClientSetLastReadMessage)
        throw exc::WrongresponceTypeException();
      else {
        const auto &packetDTO = static_cast<const StructDTOClass<ResponceDTO> &>(
                                  *responcePacketListDTO.packets[0].structDTOPtr)
                                  .getStructDTOClass();

        if (!packetDTO.reqResult)
          throw exc::LastReadMessageException();
        else
          return true;
      }
    }
  } // try
  catch (const exc::WrongresponceTypeException &ex) {
    std::cout << "Client. sendLastReadMessageFromClient. " << ex.what() << std::endl;
    return false;
  } catch (const exc::EmptyPacketException &ex) {
    std::cout << "Client. sendLastReadMessageFromClient. " << ex.what() << std::endl;
    return false;
  } catch (const exc::LastReadMessageException &ex) {
    std::cout << "Client. sendLastReadMessageFromClient. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// transport setters
//
//
// Receive a user
void ClientSession::setActiveUserDTOFromSrv(const UserDTO &userDTO) const {

  auto user_ptr = std::make_shared<User>(
    UserData(userDTO.login, userDTO.userName, userDTO.passwordhash, userDTO.email, userDTO.phone));
  user_ptr->createChatList(std::make_shared<UserChatList>(user_ptr));

  if (!_instance.findUserByLogin(userDTO.login))
    _instance.addUserToSystem(user_ptr);

  _instance.setActiveUser(user_ptr);
}
//
//
//
void ClientSession::setUserDTOFromSrv(const UserDTO &userDTO) const {

  auto user_ptr = std::make_shared<User>(UserData(userDTO.login, userDTO.userName, "-1", userDTO.email, userDTO.phone));
  user_ptr->createChatList();

  if (!_instance.findUserByLogin(userDTO.login))
    _instance.addUserToSystem(user_ptr);
  //   std::cout << "User added to the system. Name / Login: " << user_ptr->getUserName() << " / "
  //             << user_ptr->getLogin() << std::endl;
}
//
//
// Receive a single message
void ClientSession::setOneMessageDTO(const MessageDTO &messageDTO, const std::shared_ptr<Chat> &chat) const {

  // Get a pointer to the user who sent the message
  auto sender = this->_instance.findUserByLogin(messageDTO.senderLogin);

  // Simplified handling that supports only a single text message

  if (messageDTO.messageContent.empty())
    throw exc::UnknownException("DTO message contains no content");

  auto message = createOneMessage(messageDTO.messageContent[0].payload, sender, messageDTO.timeStamp,
                                  messageDTO.messageId);

  chat->addMessageToChat(std::make_shared<Message>(message), sender, false);

  //   std::cout << "Message added to the system. MessageId " << message.getMessageId() << " in chat chatId "
  //             << chat->getChatId() << std::endl;

  // ⬇⬇⬇ Debug check insertion
  // std::cout << "[DEBUG] Message added:\n";
  // std::cout << "- msgId: " << message.getMessageId();

  // if (sender) {
  //   std::cout << ", sender: " << sender->getLogin();
  // } else {
  //   std::cout << ", sender: [unknown]";
  // }

  // std::cout << ", timeStamp: "
  //           << formatTimeStampToString(message.getTimeStamp(), true)
  //           << ", content: ";

  // for (const auto &content_ptr : message.getContent()) {
  //   auto textContent_ptr =
  //       std::dynamic_pointer_cast<MessageContent<TextContent>>(content_ptr);
  //   if (textContent_ptr) {
  //     std::cout << textContent_ptr->getMessageContent()._text << " ";
  //   } else {
  //     std::cout << "[non-text content] ";
  //   }
  // }

  // std::cout << std::endl;
}
//
//
// Receive messages for a single chat
bool ClientSession::setOneChatMessageDTO(const MessageChatDTO &messageChatDTO) const {

  // Got the user's chat list
  auto chats = this->_instance.getActiveUser()->getUserChatList()->getChatFromList();

  std::shared_ptr<Chat> chat_ptr;
  for (const auto &chat : chats) {
    chat_ptr = chat.lock();

    if (chat_ptr && chat_ptr->getChatId() == messageChatDTO.chatId) {

      // Inside the chat located by chatId, sequentially call
      // the single-message fill routine
      for (const auto &message : messageChatDTO.messageDTO) {
        setOneMessageDTO(message, chat_ptr);
      }

      //   // ⬇⬇⬇ Debug check insertion
      //   const auto &messages = chat_ptr->getMessages();

      //   std::cout << "\n[DEBUG] Chat " << chat_ptr->getChatId()
      //             << ", total messages: " << messages.size() << std::endl;

      //   for (const auto &[timeStamp, message_ptr] : messages) {
      //     std::cout << "- msgId: " << message_ptr->getMessageId();

      //     auto sender_ptr = message_ptr->getSender().lock();
      //     if (sender_ptr) {
      //       std::cout << ", sender: " << sender_ptr->getLogin();
      //     } else {
      //       std::cout << ", sender: [unknown]";
      //     }

      //     std::cout << ", timeStamp: " << formatTimeStampToString(timeStamp,
      //     true)
      //               << ", content: ";

      //     for (const auto &content_ptr : message_ptr->getContent()) {
      //       auto textContent_ptr =
      //           std::dynamic_pointer_cast<MessageContent<TextContent>>(
      //               content_ptr);
      //       if (textContent_ptr) {
      //         std::cout << textContent_ptr->getMessageContent()._text << " ";
      //       } else {
      //         std::cout << "[non-text content] ";
      //       }
      //     }

      //     std::cout << std::endl;
      //   }
    }
  }
  return true;
}
//
//
//
bool ClientSession::checkAndAddParticipantToSystem(const std::vector<std::string> &participants) {

  try {

    bool needRequest = false;
    std::vector<PacketDTO> packetDTOListSend;

    for (const auto &participant : participants) {

      const auto &user_ptr = this->_instance.findUserByLogin(participant);

      if (user_ptr == nullptr) {

        needRequest = true;

        UserLoginDTO userLoginDTO;
        userLoginDTO.login = participant;

        PacketDTO packetDTO;
        packetDTO.requestType = RequestType::RqFrClientGetUsersData;
        packetDTO.structDTOClassType = StructDTOClassType::userLoginDTO;
        packetDTO.reqDirection = RequestDirection::ClientToSrv;
        packetDTO.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(userLoginDTO);

        packetDTOListSend.push_back(packetDTO);
      } // if user_ptr
    }

    if (needRequest) {

      PacketListDTO packetListDTOresult;
      packetListDTOresult.packets.clear();

      packetListDTOresult = processingRequestToServer(packetDTOListSend, RequestType::RqFrClientGetUsersData);

      {
        // Add the missing users
        for (const auto &responcePacket : packetListDTOresult.packets) {
          if (responcePacket.requestType != RequestType::RqFrClientGetUsersData)
            continue; // Skip unrelated packets

          if (responcePacket.structDTOClassType != StructDTOClassType::userDTO)
            throw exc::WrongresponceTypeException();

          const auto &userDTO = static_cast<const StructDTOClass<UserDTO> &>(*responcePacket.structDTOPtr)
                                  .getStructDTOClass();

          auto newUser_ptr = std::make_shared<User>(
            UserData(userDTO.login, userDTO.userName, "-1", userDTO.email, userDTO.phone));

          _instance.addUserToSystem(newUser_ptr);
          std::cout << "Chat participant added to the system. Name/Login: " << newUser_ptr->getUserName() << " / "
                    << newUser_ptr->getLogin() << std::endl;
        }
      }
    } else { // No response received from the server
      std::cerr << "Error. Failed to fetch users from the server. "
                   "Only their logins will be temporarily added to the system."
                << std::endl;

      for (const auto &packetDTO : packetDTOListSend) {

        const auto &userLoginDTO = static_cast<const StructDTOClass<UserLoginDTO> &>(*packetDTO.structDTOPtr)
                                     .getStructDTOClass();

        auto newUser_ptr = std::make_shared<User>(UserData(userLoginDTO.login, "Unknown", "-1", "Unknown", "Unknown"));

        _instance.addUserToSystem(newUser_ptr);
      }
    }
  } // try
  catch (const exc::WrongresponceTypeException &ex) {
    std::cerr << "Client: adding participants. " << ex.what() << std::endl;
    return false;
  }
  return true;
}
//
//
// Receive a chat
void ClientSession::setOneChatDTOFromSrv(const ChatDTO &chatDTO) {

  // Initialize the chat
  auto chat_ptr = std::make_shared<Chat>();
  auto chatList = this->getActiveUserCl()->getUserChatList();

  // Fill in fields
  chat_ptr->setChatId(chatDTO.chatId);

  // Add participants

  // First check whether the participants are already in the system, build a
  // request for missing participants' data, and fetch them from the server

  std::vector<std::string> participants;
  participants.clear();

  for (const auto &participant : chatDTO.participants)
    participants.push_back(participant.login);

  checkAndAddParticipantToSystem(participants);

  for (const auto &participant : chatDTO.participants) {

    for (const auto &delMessId : participant.deletedMessageIds)
      chat_ptr->setDeletedMessageMap(participant.login, delMessId);

    auto user_ptr = this->_instance.findUserByLogin(participant.login);

    if (user_ptr) {
      chat_ptr->setLastReadMessageId(user_ptr, participant.lastReadMessage);
      chat_ptr->addParticipant(user_ptr, participant.lastReadMessage, participant.deletedFromChat);
    }
  }

  this->_instance.addChatToInstance(chat_ptr);

  std::cout << "Chat added to the system. ChatId: " << chat_ptr->getChatId() << std::endl;

  // ⬇⬇⬇ Check
  // std::cout << "\n[DEBUG] Added chat on the client:\n";
  // std::cout << "chatId = " << chat_ptr->getChatId() << std::endl;
  // std::cout << "lastMessageTime = "
  //           << formatTimeStampToString(
  //                  chat_ptr->getTimeStampForLastMessage(
  //                      chat_ptr->getLastReadMessageId(
  //                          this->_instance.getActiveUser())),
  //                  true)

  //           << std::endl;

  // std::cout << "participants:" << std::endl;
  // for (const auto &participant : chat_ptr->getParticipants()) {
  //   auto user = participant._user.lock();
  //   if (user) {
  //     std::cout << "- " << user->getLogin()
  //               << " | deleted: " << participant._deletedFromChat <<
  //               std::endl;
  //   } else {
  //     std::cout << "- removed user" << std::endl;
  //   }
}
std::optional<ChatDTO> ClientSession::FillForSendOneChatDTOFromClient(const std::shared_ptr<Chat> &chat_ptr) {
  ChatDTO chatDTO;

  // Took the chatId
  chatDTO.chatId = chat_ptr->getChatId();
  chatDTO.senderLogin = _instance.getActiveUser()->getLogin();

  try {
    // Get the list of participants
    auto participants = chat_ptr->getParticipants();

    // Iterate over participants
    for (const auto &participant : participants) {

      // Got the pointer to the user
      const auto user_ptr = participant._user.lock();

      // Temporary struct to fill in
      ParticipantsDTO participantsDTO;

      if (user_ptr) {

        // Fill in the user data for registration in the system

        participantsDTO.login = user_ptr->getLogin();

        // Fill lastReadMessage
        participantsDTO.lastReadMessage = chat_ptr->getLastReadMessageId(user_ptr);

        // Fill deletedMessageIds
        participantsDTO.deletedMessageIds.clear();
        const auto &delMessMap = chat_ptr->getDeletedMessagesMap();
        const auto &it = delMessMap.find(participantsDTO.login);

        if (it != delMessMap.end()) {
          for (const auto &delMessId : it->second) {
            participantsDTO.deletedMessageIds.push_back(delMessId);
          }
        }

        participantsDTO.deletedFromChat = chat_ptr->getUserDeletedFromChat(user_ptr);

        chatDTO.participants.push_back(participantsDTO);

      } // if user_ptr
      else
        throw exc::UserNotFoundException();
    } // for participants
  }
  // try
  catch (const exc::UserNotFoundException &ex) {
    std::cerr << "Client: FillForSendOneChatDTOFromClient. " << ex.what() << std::endl;
    return std::nullopt;
  }
  return chatDTO;
}
//
//
// Get a specific user message
MessageDTO ClientSession::FillForSendOneMessageDTOFromClient(const std::shared_ptr<Message> &message,
                                                             const std::size_t &chatId) {
  MessageDTO messageDTO;
  auto user_ptr = message->getSender().lock();

  messageDTO.senderLogin = user_ptr ? user_ptr->getLogin() : "";

  messageDTO.chatId = chatId;
  messageDTO.messageId = message->getMessageId();
  messageDTO.timeStamp = message->getTimeStamp();

  // Get the content
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

// Send user messages for a specific chat
MessageChatDTO ClientSession::fillChatMessageDTOFromClient(const std::shared_ptr<Chat> &chat) {
  MessageChatDTO messageChatDTO;

  // Took the chatId
  messageChatDTO.chatId = chat->getChatId();

  for (const auto &message : chat->getMessages()) {

    messageChatDTO.messageDTO.push_back(FillForSendOneMessageDTOFromClient(message.second, messageChatDTO.chatId));

  } // for message
  return messageChatDTO;
}
