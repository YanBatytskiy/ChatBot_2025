#include "serialize.h"
#include "dto/dto_struct.h"
#include "exception/network_exception.h"
#include <cstdint>
#include <cwchar>
#include <iostream>
#include <memory>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif


std::uint64_t toBigEndian64(std::uint64_t value) {
  // делим на два 32-битных куска и применяем htonl
  std::uint32_t high = htonl(static_cast<std::uint32_t>(value >> 32));
  std::uint32_t low = htonl(static_cast<std::uint32_t>(value & 0xFFFFFFFF));
  return (static_cast<std::uint64_t>(low) << 32) | high;
}

std::uint64_t fromBigEndian64(std::uint64_t value) {
  std::uint32_t high = ntohl(static_cast<std::uint32_t>(value & 0xFFFFFFFF));
  std::uint32_t low = ntohl(static_cast<std::uint32_t>(value >> 32));
  return (static_cast<std::uint64_t>(high) << 32) | low;
}

std::vector<PacketDTO> deSerializePacketList(const std::vector<std::uint8_t> &buffer) {
  std::vector<PacketDTO> packetList;
  std::size_t offset = 0;

  // Проверка на минимальный размер (4 байта для количества пакетов)
  if (buffer.size() < 4)
    throw exc::WrongPacketSizeException();

  // Считываем количество пакетов
  std::uint32_t count = 0;
  count |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
  count |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
  count |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
  count |= static_cast<std::uint32_t>(buffer[offset++]);
  count = ntohl(count); // ← ОБЯЗАТЕЛЬНО для корре÷ктной работы

  try {
    for (std::uint32_t i = 0; i < count; ++i) {
      // Проверка на возможность прочитать 4 байта длины пакета
      if (offset + 4 > buffer.size())
        throw exc::WrongPacketSizeException();

      std::uint32_t packetSize = 0;
      packetSize |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
      packetSize |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
      packetSize |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
      packetSize |= static_cast<std::uint32_t>(buffer[offset++]);
      packetSize = ntohl(packetSize); // ← тоже приводим к host order

      // Проверка: достаточно ли байт для тела пакета
      if (offset + packetSize > buffer.size())
        throw exc::WrongPacketSizeException();

      std::vector<std::uint8_t> packet(buffer.begin() + offset, buffer.begin() + offset + packetSize);

      PacketDTO deserializedPacket = deSerializeFromBinary(packet);
      packetList.push_back(deserializedPacket);

      offset += packetSize;
    }
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Клиент. " << ex.what() << std::endl;
    packetList.clear();
  } catch (const std::exception &ex) {
    std::cerr << "Клиент. Неизвестная ошибка при десериализации: " << ex.what() << std::endl;
    packetList.clear();
  }

  return packetList;
}

PacketDTO deSerializeFromBinary(const std::vector<std::uint8_t> &buffer) {
  PacketDTO packet;
  std::size_t offset = 0;

  // Проверка на минимальный размер: тип запроса (4), тип пакета (4), тип
  // направления (4)
  if (buffer.size() < 13)
    throw exc::WrongPacketSizeException();

  // Тип запроса (4 байта) RequestType
  std::uint32_t requestType = 0;
  requestType |= (static_cast<std::uint32_t>(buffer[offset++]) << 24);
  requestType |= (static_cast<std::uint32_t>(buffer[offset++]) << 16);
  requestType |= (static_cast<std::uint32_t>(buffer[offset++]) << 8);
  requestType |= static_cast<std::uint32_t>(buffer[offset++]);
  packet.requestType = static_cast<RequestType>(ntohl(requestType));

  // Тип пакета (4 байта) StructDTOClassType
  std::uint32_t classType = 0;
  classType |= (static_cast<std::uint32_t>(buffer[offset++]) << 24);
  classType |= (static_cast<std::uint32_t>(buffer[offset++]) << 16);
  classType |= (static_cast<std::uint32_t>(buffer[offset++]) << 8);
  classType |= static_cast<std::uint32_t>(buffer[offset++]);
  packet.structDTOClassType = static_cast<StructDTOClassType>(ntohl(classType));

  // Тип пакета (4 байта) RequestDirection

  std::uint32_t direction = 0;
  direction |= (buffer[offset++] << 24);
  direction |= (buffer[offset++] << 16);
  direction |= (buffer[offset++] << 8);
  direction |= buffer[offset++];
  // std::cerr << "[DEBUG] Направление (сырое) = " << direction
  //           << ", ntohl = " << ntohl(direction) << std::endl;
  packet.reqDirection = static_cast<RequestDirection>(ntohl(direction));

  //   std::uint32_t direction = 0;
  //   direction |= (static_cast<std::uint32_t>(buffer[offset++]) << 24);
  //   direction |= (static_cast<std::uint32_t>(buffer[offset++]) << 16);
  //   direction |= (static_cast<std::uint32_t>(buffer[offset++]) << 8);
  //   direction |= static_cast<std::uint32_t>(buffer[offset++]);
  //   packet.reqDirection = static_cast<RequestDirection>(ntohl(direction));
  // std::cerr << "[DEBUG] Направление (сырое) = " << direction << ", ntohl = "
  // << ntohl(direction) << std::endl;
  try {
    switch (packet.structDTOClassType) {
    case StructDTOClassType::responceDTO: {
      if (offset + 4 > buffer.size())
        throw exc::WrongPacketSizeException();

      ResponceDTO responceDTO_DSer;
      responceDTO_DSer.reqResult = buffer[offset++] != 0;

      if (offset + 8 > buffer.size())
        throw exc::WrongPacketSizeException();
      // anyNumber
      std::uint64_t anyNumber_DSer = 0;
      for (int i = 0; i < 8; ++i) {
        anyNumber_DSer = (anyNumber_DSer << 8) | buffer[offset++];
      }
      responceDTO_DSer.anyNumber = fromBigEndian64(anyNumber_DSer);

      auto readString = [&](std::string &str) {
        if (offset + 4 > buffer.size())
          throw exc::WrongPacketSizeException();

        std::uint32_t len = 0;
        len |= (buffer[offset++] << 24);
        len |= (buffer[offset++] << 16);
        len |= (buffer[offset++] << 8);
        len |= buffer[offset++];
        len = ntohl(len);
        if (offset + len > buffer.size())
          throw exc::WrongPacketSizeException();
        str.assign(buffer.begin() + offset, buffer.begin() + offset + len);
        offset += len;
      };

      readString(responceDTO_DSer.anyString);

      packet.structDTOPtr = std::make_shared<StructDTOClass<ResponceDTO>>(responceDTO_DSer);
      break;
    }

    case StructDTOClassType::userLoginDTO: {
      if (offset + 4 > buffer.size())
        throw exc::WrongPacketSizeException();

      std::uint32_t len = 0;
      len |= (buffer[offset++] << 24);
      len |= (buffer[offset++] << 16);
      len |= (buffer[offset++] << 8);
      len |= buffer[offset++];
      len = ntohl(len);

      if (offset + len > buffer.size())
        throw exc::WrongPacketSizeException();

      std::string login(buffer.begin() + offset, buffer.begin() + offset + len);
      offset += len;

      packet.structDTOPtr = std::make_shared<StructDTOClass<UserLoginDTO>>(UserLoginDTO{login});

      break;
    }
    case StructDTOClassType::userLoginPasswordDTO: {
      auto readString = [&](std::string &str) {
        if (offset + 4 > buffer.size())
          throw exc::WrongPacketSizeException();

        std::uint32_t len = 0;
        len |= (buffer[offset++] << 24);
        len |= (buffer[offset++] << 16);
        len |= (buffer[offset++] << 8);
        len |= buffer[offset++];
        len = ntohl(len);
        if (offset + len > buffer.size())
          throw exc::WrongPacketSizeException();
        str.assign(buffer.begin() + offset, buffer.begin() + offset + len);
        offset += len;
      };

      UserLoginPasswordDTO dto;
      readString(dto.login);
      readString(dto.passwordhash);
      packet.structDTOPtr = std::make_shared<StructDTOClass<UserLoginPasswordDTO>>(dto);
      break;
    }

    case StructDTOClassType::userDTO: {
      auto readString = [&](std::string &str) {
        std::uint32_t len = 0;
        len |= (buffer[offset++] << 24);
        len |= (buffer[offset++] << 16);
        len |= (buffer[offset++] << 8);
        len |= buffer[offset++];
        len = ntohl(len);
        str.assign(buffer.begin() + offset, buffer.begin() + offset + len);
        offset += len;
      };

      UserDTO dto;
      readString(dto.login);
      readString(dto.passwordhash);
      readString(dto.userName);
      readString(dto.email);
      readString(dto.phone);

      packet.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(dto);
      break;
    }
    case StructDTOClassType::chatDTO: {
      ChatDTO chatDTO;

      // chat Id
      std::uint64_t id64 = 0;
      for (int i = 0; i < 8; ++i) {
        id64 |= static_cast<std::uint64_t>(buffer[offset++]) << (56 - i * 8);
      }
      chatDTO.chatId = static_cast<std::size_t>(fromBigEndian64(id64));

      // senderLogin
      auto readString = [&](std::string &str) {
        std::uint32_t len = 0;
        len |= (buffer[offset++] << 24);
        len |= (buffer[offset++] << 16);
        len |= (buffer[offset++] << 8);
        len |= buffer[offset++];
        len = ntohl(len);
        str.assign(buffer.begin() + offset, buffer.begin() + offset + len);
        offset += len;
      };

      readString(chatDTO.senderLogin);

      // participants
      std::uint32_t partCount = 0;
      partCount |= (buffer[offset++] << 24);
      partCount |= (buffer[offset++] << 16);
      partCount |= (buffer[offset++] << 8);
      partCount |= buffer[offset++];
      partCount = ntohl(partCount);

      for (std::uint32_t i = 0; i < partCount; ++i) {
        ParticipantsDTO participant;
        readString(participant.login);

        std::uint32_t lastRead = 0;
        lastRead |= (buffer[offset++] << 24);
        lastRead |= (buffer[offset++] << 16);
        lastRead |= (buffer[offset++] << 8);
        lastRead |= buffer[offset++];
        participant.lastReadMessage = ntohl(lastRead);

        // Десериализация количества удалённых сообщений (4 байта)
        if (offset + 4 > buffer.size())
          throw exc::WrongPacketSizeException();

        std::uint32_t count = 0;
        for (int i = 0; i < 4; ++i) {
          count = (count << 8) | buffer[offset++];
        }
        count = ntohl(count);

        // Десериализация deletedMessageIds (каждый как 8 байт, big endian)
        for (std::uint32_t i = 0; i < count; ++i) {
          if (offset + 8 > buffer.size())
            throw exc::WrongPacketSizeException();

          std::uint64_t idNet = 0;
          for (int j = 0; j < 8; ++j) {
            idNet = (idNet << 8) | buffer[offset++];
          }
          participant.deletedMessageIds.push_back(static_cast<std::size_t>(fromBigEndian64(idNet)));
        }

        participant.deletedFromChat = buffer[offset++] != 0;
        chatDTO.participants.push_back(participant);
      }

      packet.structDTOPtr = std::make_shared<StructDTOClass<ChatDTO>>(chatDTO);
      break;
    }
    case StructDTOClassType::messageChatDTO: {
      MessageChatDTO messageChatDTO;

      // Считываем chatId (8 байт, big-endian)
      std::uint64_t chatIdNet = 0;
      for (int i = 0; i < 8; ++i) {
        chatIdNet = (chatIdNet << 8) | buffer[offset++];
      }
      messageChatDTO.chatId = fromBigEndian64(chatIdNet);

      // Считываем количество сообщений (4 байта)
      std::uint32_t msgCount = 0;
      msgCount |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
      msgCount |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
      msgCount |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
      msgCount |= static_cast<std::uint32_t>(buffer[offset++]);
      msgCount = ntohl(msgCount);

      for (std::size_t i = 0; i < msgCount; ++i) {
        MessageDTO msg;

        // senderLogin (строка)
        std::uint32_t lenLogin = 0;
        lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
        lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
        lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
        lenLogin |= static_cast<std::uint32_t>(buffer[offset++]);
        lenLogin = ntohl(lenLogin);

        msg.senderLogin = std::string(buffer.begin() + offset, buffer.begin() + offset + lenLogin);
        offset += lenLogin;

        // messageId (8 байт)
        std::uint64_t msgIdNet = 0;
        for (int j = 0; j < 8; ++j) {
          msgIdNet = (msgIdNet << 8) | buffer[offset++];
        }
        msg.messageId = fromBigEndian64(msgIdNet);

        // timeStamp (8 байт)
        std::uint64_t tsNet = 0;
        for (int j = 0; j < 8; ++j) {
          tsNet = (tsNet << 8) | buffer[offset++];
        }
        msg.timeStamp = fromBigEndian64(tsNet);

        // messageContent.size() (4 байта)
        std::uint32_t contentCount = 0;
        contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
        contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
        contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
        contentCount |= static_cast<std::uint32_t>(buffer[offset++]);
        contentCount = ntohl(contentCount);

        for (std::size_t j = 0; j < contentCount; ++j) {
          MessageContentDTO content;

          // Тип содержимого (1 байт)
          content.messageContentType = static_cast<MessageContentType>(buffer[offset++]);

          // Длина payload (4 байта)
          std::uint32_t lenPayload = 0;
          lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
          lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
          lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
          lenPayload |= static_cast<std::uint32_t>(buffer[offset++]);
          lenPayload = ntohl(lenPayload);

          // Сами байты строки
          content.payload = std::string(buffer.begin() + offset, buffer.begin() + offset + lenPayload);
          offset += lenPayload;

          msg.messageContent.push_back(content);
        }

        messageChatDTO.messageDTO.push_back(msg);
      }

      packet.structDTOPtr = std::make_shared<StructDTOClass<MessageChatDTO>>(messageChatDTO);
      break;
    }
    case StructDTOClassType::messageDTO: {
      MessageDTO messageDTO;

      std::uint64_t chatIdNet = 0;
      for (int i = 0; i < 8; ++i) {
        chatIdNet = (chatIdNet << 8) | buffer[offset++];
      }
      messageDTO.chatId = fromBigEndian64(chatIdNet);

      // senderLogin
      std::uint32_t lenLogin = 0;
      lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
      lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
      lenLogin |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
      lenLogin |= static_cast<std::uint32_t>(buffer[offset++]);
      lenLogin = ntohl(lenLogin);

      messageDTO.senderLogin = std::string(buffer.begin() + offset, buffer.begin() + offset + lenLogin);
      offset += lenLogin;

      // messageId (8 байт)
      std::uint64_t msgIdNet = 0;
      for (int i = 0; i < 8; ++i) {
        msgIdNet = (msgIdNet << 8) | buffer[offset++];
      }
      messageDTO.messageId = fromBigEndian64(msgIdNet);

      // timeStamp (8 байт)
      std::uint64_t tsNet = 0;
      for (int i = 0; i < 8; ++i) {
        tsNet = (tsNet << 8) | buffer[offset++];
      }
      messageDTO.timeStamp = fromBigEndian64(tsNet);

      // messageContent.size() (4 байта)
      std::uint32_t contentCount = 0;
      contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
      contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
      contentCount |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
      contentCount |= static_cast<std::uint32_t>(buffer[offset++]);
      contentCount = ntohl(contentCount);

      for (std::size_t j = 0; j < contentCount; ++j) {
        MessageContentDTO content;

        // Тип содержимого (1 байт)
        content.messageContentType = static_cast<MessageContentType>(buffer[offset++]);

        // Длина строки payload (4 байта)
        std::uint32_t lenPayload = 0;
        lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 24;
        lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 16;
        lenPayload |= static_cast<std::uint32_t>(buffer[offset++]) << 8;
        lenPayload |= static_cast<std::uint32_t>(buffer[offset++]);
        lenPayload = ntohl(lenPayload);

        // payload
        content.payload = std::string(buffer.begin() + offset, buffer.begin() + offset + lenPayload);
        offset += lenPayload;

        messageDTO.messageContent.push_back(content);
      }

      packet.structDTOPtr = std::make_shared<StructDTOClass<MessageDTO>>(messageDTO);
      break;
    }
    default:
      throw exc::UnsupportedDeSirializeTypeException();
    }
  } catch (const exc::UnsupportedDeSirializeTypeException &ex) {
    std::cerr << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Неизвестная ошибка. DeSerialize: " << ex.what() << std::endl;
  }

  return packet;
}
//
//
//
std::vector<std::uint8_t> serializePacketList(std::vector<PacketDTO> &packetList) {
  std::vector<std::uint8_t> buffer;

  std::uint32_t count = htonl(static_cast<std::uint32_t>(packetList.size()));
  buffer.push_back((count >> 24) & 0xFF);
  buffer.push_back((count >> 16) & 0xFF);
  buffer.push_back((count >> 8) & 0xFF);
  buffer.push_back(count & 0xFF);

  // проверка
  // std::cerr << "[DEBUG] Всего пакетов для сериализации: " <<
  // packetList.size()
  //           << std::endl;

  for (std::size_t i = 0; i < packetList.size(); ++i) {
    const auto &packet = packetList[i];
    // проверка

    // std::cerr << "[DEBUG] Пакет #" << i
    //           << ", RequestType = " << static_cast<int>(packet.requestType)
    //           << ", ClassType = " <<
    //           static_cast<int>(packet.structDTOClassType)
    //           << ", Direction = " << static_cast<int>(packet.reqDirection)
    //           << std::endl;

    // std::cerr << "[DEBUG] Перед serializeToBinary — адрес PacketDTO: "
    //           << &packet << std::endl;

    std::vector<std::uint8_t> serialized = serializeToBinary(packet);
    // std::cerr << "[DEBUG] serializeToBinary() вернул " << serialized.size()
    //           << " байт" << std::endl;

    // Длина пакета
    std::uint32_t packetSize = htonl(static_cast<std::uint32_t>(serialized.size()));
    buffer.push_back((packetSize >> 24) & 0xFF);
    buffer.push_back((packetSize >> 16) & 0xFF);
    buffer.push_back((packetSize >> 8) & 0xFF);
    buffer.push_back(packetSize & 0xFF);

    // Сам пакет
    buffer.insert(buffer.end(), serialized.begin(), serialized.end());
  }

  return buffer;
} //
//
//

std::vector<std::uint8_t> serializeToBinary(const PacketDTO &packetDTO) {

  // проверка
  // std::cerr << "[DEBUG] PacketDTO: type = "
  //           << static_cast<int>(packetDTO.structDTOClassType)
  //           << ", direction = " << static_cast<int>(packetDTO.reqDirection)
  //           << ", requestType = " << static_cast<int>(packetDTO.requestType)
  //           << std::endl;

  std::vector<std::uint8_t> buffer;

  // Сериализуем тип запроса (4 байта)
  std::uint32_t requestType = htonl(static_cast<std::uint32_t>(packetDTO.requestType));
  buffer.push_back((requestType >> 24) & 0xFF);
  buffer.push_back((requestType >> 16) & 0xFF);
  buffer.push_back((requestType >> 8) & 0xFF);
  buffer.push_back(requestType & 0xFF);

  // Сериализуем тип пакета (4 байта)
  std::uint32_t classType = htonl(static_cast<std::uint32_t>(packetDTO.structDTOClassType));
  buffer.push_back((classType >> 24) & 0xFF);
  buffer.push_back((classType >> 16) & 0xFF);
  buffer.push_back((classType >> 8) & 0xFF);
  buffer.push_back(classType & 0xFF);

  // Сериализуем направление (4 байта)
  std::uint32_t direction = htonl(static_cast<std::uint32_t>(packetDTO.reqDirection));
  buffer.push_back((direction >> 24) & 0xFF);
  buffer.push_back((direction >> 16) & 0xFF);
  buffer.push_back((direction >> 8) & 0xFF);
  buffer.push_back(direction & 0xFF);

  try {
    switch (packetDTO.structDTOClassType) {
      // ResponceDTO
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case StructDTOClassType::responceDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::responceDTO)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<ResponceDTO>>(packetDTO.structDTOPtr);

      const auto result = ptr->getStructDTOClass();

      // проверка
      // std::cerr << "[DEBUG] buffer address before push: " << &buffer
      //           << ", size = " << buffer.size() << std::endl;
      //  добавили значение Результат
      buffer.push_back(result.reqResult ? 1 : 0);

      // добавили число и строку
      //  сериализуем anyNumber
      std::uint64_t anyNumberNet = toBigEndian64(result.anyNumber);
      buffer.push_back((anyNumberNet >> 56) & 0xFF);
      buffer.push_back((anyNumberNet >> 48) & 0xFF);
      buffer.push_back((anyNumberNet >> 40) & 0xFF);
      buffer.push_back((anyNumberNet >> 32) & 0xFF);
      buffer.push_back((anyNumberNet >> 24) & 0xFF);
      buffer.push_back((anyNumberNet >> 16) & 0xFF);
      buffer.push_back((anyNumberNet >> 8) & 0xFF);
      buffer.push_back(anyNumberNet & 0xFF);

      // сериализуем anyString (владелец чата)
      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      writeString(result.anyString);
      break;
    }
    // UserLoginDTO
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case StructDTOClassType::userLoginDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::userLoginDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<UserLoginDTO>>(packetDTO.structDTOPtr);

      const auto login = ptr->getStructDTOClass().login;

      // Длина строки логина (4 байта)
      std::uint32_t len = htonl(static_cast<std::uint32_t>(login.size()));
      buffer.push_back((len >> 24) & 0xFF);
      buffer.push_back((len >> 16) & 0xFF);
      buffer.push_back((len >> 8) & 0xFF);
      buffer.push_back(len & 0xFF);

      // Сами байты строки
      buffer.insert(buffer.end(), login.begin(), login.end());
      break;
    }
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    case StructDTOClassType::userLoginPasswordDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::userLoginPasswordDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<UserLoginPasswordDTO>>(packetDTO.structDTOPtr);

      const auto &dto = ptr->getStructDTOClass();

      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      writeString(dto.login);
      writeString(dto.passwordhash);
      break;
    }
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    case StructDTOClassType::userDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::userDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<UserDTO>>(packetDTO.structDTOPtr);
      const auto &dto = ptr->getStructDTOClass();

      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      writeString(dto.login);
      writeString(dto.passwordhash);
      writeString(dto.userName);
      writeString(dto.email);
      writeString(dto.phone);
      break;
    }
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case StructDTOClassType::chatDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::chatDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<ChatDTO>>(packetDTO.structDTOPtr);
      const ChatDTO &chatDTO = ptr->getStructDTOClass();

      // сериализуем chatId (8 байта)
      std::uint64_t id64 = toBigEndian64(static_cast<std::uint64_t>(chatDTO.chatId));
      for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<std::uint8_t>((id64 >> (56 - i * 8)) & 0xFF));
      }

      // сериализуем senderLogin (владелец чата)
      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      writeString(chatDTO.senderLogin);

      // сериализуем список участников
      std::uint32_t partCount = htonl(static_cast<std::uint32_t>(chatDTO.participants.size()));
      buffer.push_back((partCount >> 24) & 0xFF);
      buffer.push_back((partCount >> 16) & 0xFF);
      buffer.push_back((partCount >> 8) & 0xFF);
      buffer.push_back(partCount & 0xFF);

      for (const auto &participant : chatDTO.participants) {
        writeString(participant.login);

        // сериализуем lastReadMessage
        std::uint32_t lastRead = htonl(static_cast<std::uint32_t>(participant.lastReadMessage));
        buffer.push_back((lastRead >> 24) & 0xFF);
        buffer.push_back((lastRead >> 16) & 0xFF);
        buffer.push_back((lastRead >> 8) & 0xFF);
        buffer.push_back(lastRead & 0xFF);

        // Сериализуем количество удалённых сообщений
        std::uint32_t count = htonl(static_cast<std::uint32_t>(participant.deletedMessageIds.size()));
        buffer.push_back((count >> 24) & 0xFF);
        buffer.push_back((count >> 16) & 0xFF);
        buffer.push_back((count >> 8) & 0xFF);
        buffer.push_back(count & 0xFF);

        // Сами ID сообщений — каждое как 8 байт (64-бит, big-endian)
        for (const auto &messageId : participant.deletedMessageIds) {
          std::uint64_t idNet = toBigEndian64(static_cast<std::uint64_t>(messageId));
          buffer.push_back((idNet >> 56) & 0xFF);
          buffer.push_back((idNet >> 48) & 0xFF);
          buffer.push_back((idNet >> 40) & 0xFF);
          buffer.push_back((idNet >> 32) & 0xFF);
          buffer.push_back((idNet >> 24) & 0xFF);
          buffer.push_back((idNet >> 16) & 0xFF);
          buffer.push_back((idNet >> 8) & 0xFF);
          buffer.push_back(idNet & 0xFF);
        }

        buffer.push_back(participant.deletedFromChat ? 1 : 0);
      }

      break;
    }
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case StructDTOClassType::messageChatDTO: {
      if (packetDTO.structDTOClassType != StructDTOClassType::messageChatDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<MessageChatDTO>>(packetDTO.structDTOPtr);
      const MessageChatDTO &messageChatDTO = ptr->getStructDTOClass();

      // Лямбда для сериализации строки
      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      // Сериализуем chatId — 8 байт (big endian)
      std::uint64_t chatIdNet = toBigEndian64(static_cast<std::uint64_t>(messageChatDTO.chatId));
      buffer.push_back((chatIdNet >> 56) & 0xFF);
      buffer.push_back((chatIdNet >> 48) & 0xFF);
      buffer.push_back((chatIdNet >> 40) & 0xFF);
      buffer.push_back((chatIdNet >> 32) & 0xFF);
      buffer.push_back((chatIdNet >> 24) & 0xFF);
      buffer.push_back((chatIdNet >> 16) & 0xFF);
      buffer.push_back((chatIdNet >> 8) & 0xFF);
      buffer.push_back(chatIdNet & 0xFF);

      // Сериализуем количество сообщений
      std::uint32_t msgCount = htonl(static_cast<std::uint32_t>(messageChatDTO.messageDTO.size()));
      buffer.push_back((msgCount >> 24) & 0xFF);
      buffer.push_back((msgCount >> 16) & 0xFF);
      buffer.push_back((msgCount >> 8) & 0xFF);
      buffer.push_back(msgCount & 0xFF);

      // Сериализуем каждое сообщение
      for (const MessageDTO &msg : messageChatDTO.messageDTO) {
        writeString(msg.senderLogin);

        std::uint64_t msgIdNet = toBigEndian64(static_cast<std::uint64_t>(msg.messageId));
        buffer.push_back((msgIdNet >> 56) & 0xFF);
        buffer.push_back((msgIdNet >> 48) & 0xFF);
        buffer.push_back((msgIdNet >> 40) & 0xFF);
        buffer.push_back((msgIdNet >> 32) & 0xFF);
        buffer.push_back((msgIdNet >> 24) & 0xFF);
        buffer.push_back((msgIdNet >> 16) & 0xFF);
        buffer.push_back((msgIdNet >> 8) & 0xFF);
        buffer.push_back(msgIdNet & 0xFF);

        std::uint64_t tsNet = toBigEndian64(static_cast<std::uint64_t>(msg.timeStamp));
        buffer.push_back((tsNet >> 56) & 0xFF);
        buffer.push_back((tsNet >> 48) & 0xFF);
        buffer.push_back((tsNet >> 40) & 0xFF);
        buffer.push_back((tsNet >> 32) & 0xFF);
        buffer.push_back((tsNet >> 24) & 0xFF);
        buffer.push_back((tsNet >> 16) & 0xFF);
        buffer.push_back((tsNet >> 8) & 0xFF);
        buffer.push_back(tsNet & 0xFF);

        std::uint32_t contentCount = htonl(static_cast<std::uint32_t>(msg.messageContent.size()));
        buffer.push_back((contentCount >> 24) & 0xFF);
        buffer.push_back((contentCount >> 16) & 0xFF);
        buffer.push_back((contentCount >> 8) & 0xFF);
        buffer.push_back(contentCount & 0xFF);

        for (const MessageContentDTO &content : msg.messageContent) {
          buffer.push_back(static_cast<std::uint8_t>(content.messageContentType));
          writeString(content.payload);
        }
      }

      break;
    }
      //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    case StructDTOClassType::messageDTO: {

      if (packetDTO.structDTOClassType != StructDTOClassType::messageDTO)
        throw exc::InternalDataErrorException();

      if (!packetDTO.structDTOPtr)
        throw exc::InternalDataErrorException();

      auto ptr = std::static_pointer_cast<StructDTOClass<MessageDTO>>(packetDTO.structDTOPtr);
      const MessageDTO &messageDTO = ptr->getStructDTOClass();

      auto writeString = [&](const std::string &str) {
        std::uint32_t len = htonl(static_cast<std::uint32_t>(str.size()));
        buffer.push_back((len >> 24) & 0xFF);
        buffer.push_back((len >> 16) & 0xFF);
        buffer.push_back((len >> 8) & 0xFF);
        buffer.push_back(len & 0xFF);
        buffer.insert(buffer.end(), str.begin(), str.end());
      };

      std::uint64_t chatIdNet = toBigEndian64(static_cast<std::uint64_t>(messageDTO.chatId));
      buffer.push_back((chatIdNet >> 56) & 0xFF);
      buffer.push_back((chatIdNet >> 48) & 0xFF);
      buffer.push_back((chatIdNet >> 40) & 0xFF);
      buffer.push_back((chatIdNet >> 32) & 0xFF);
      buffer.push_back((chatIdNet >> 24) & 0xFF);
      buffer.push_back((chatIdNet >> 16) & 0xFF);
      buffer.push_back((chatIdNet >> 8) & 0xFF);
      buffer.push_back(chatIdNet & 0xFF);

      writeString(messageDTO.senderLogin);

      std::uint64_t msgIdNet = toBigEndian64(static_cast<std::uint64_t>(messageDTO.messageId));
      buffer.push_back((msgIdNet >> 56) & 0xFF);
      buffer.push_back((msgIdNet >> 48) & 0xFF);
      buffer.push_back((msgIdNet >> 40) & 0xFF);
      buffer.push_back((msgIdNet >> 32) & 0xFF);
      buffer.push_back((msgIdNet >> 24) & 0xFF);
      buffer.push_back((msgIdNet >> 16) & 0xFF);
      buffer.push_back((msgIdNet >> 8) & 0xFF);
      buffer.push_back(msgIdNet & 0xFF);

      std::uint64_t tsNet = toBigEndian64(static_cast<std::uint64_t>(messageDTO.timeStamp));
      buffer.push_back((tsNet >> 56) & 0xFF);
      buffer.push_back((tsNet >> 48) & 0xFF);
      buffer.push_back((tsNet >> 40) & 0xFF);
      buffer.push_back((tsNet >> 32) & 0xFF);
      buffer.push_back((tsNet >> 24) & 0xFF);
      buffer.push_back((tsNet >> 16) & 0xFF);
      buffer.push_back((tsNet >> 8) & 0xFF);
      buffer.push_back(tsNet & 0xFF);

      std::uint32_t contentCount = htonl(static_cast<std::uint32_t>(messageDTO.messageContent.size()));
      buffer.push_back((contentCount >> 24) & 0xFF);
      buffer.push_back((contentCount >> 16) & 0xFF);
      buffer.push_back((contentCount >> 8) & 0xFF);
      buffer.push_back(contentCount & 0xFF);

      for (const auto &content : messageDTO.messageContent) {
        buffer.push_back(static_cast<std::uint8_t>(content.messageContentType));
        writeString(content.payload);
      }

      break;
    }
    default:
      std::cerr << "[FATAL] Unsupported StructDTOClassType: " << static_cast<int>(packetDTO.structDTOClassType)
                << std::endl;
      throw exc::InternalDataErrorException();
      break;
    }
  } catch (const exc::InternalDataErrorException &ex) {
    std::cerr << ex.what() << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Неизвестная ошибка. Serialize." << ex.what() << std::endl;
  }

  return buffer;
}
