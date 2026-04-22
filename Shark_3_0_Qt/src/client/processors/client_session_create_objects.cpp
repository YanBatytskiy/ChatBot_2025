#include "client/processors/client_session_create_objects.h"

#include <QString>
#include <memory>
#include <vector>

#include "client/client_session.h"
#include "dto_struct.h"
#include "exceptions_qt/exception_network.h"
#include "exceptions_qt/exception_router.h"
#include "exceptions_qt/exception_valid.h"
#include "message/message.h"
#include "system/date_time_utils.h"
#include "system/system_function.h"
#include "user/user.h"

ClientSessionCreateObjects::ClientSessionCreateObjects(
    ClientSession &session, ClientCore &core,
    ClientSessionDtoBuilder &dto_builder)
    : session_(session), core_(core), dto_builder_(dto_builder) {}

bool ClientSessionCreateObjects::createUserProcessing(const UserDTO &user_dto) {
  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientCreateUser;
  packet_dto.structDTOClassType = StructDTOClassType::userDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr = std::make_shared<StructDTOClass<UserDTO>>(user_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(packet_dto);

  PacketListDTO packet_list_result;
  packet_list_result.packets.clear();

  packet_list_result =
      core_.processingRequestToServerCore(packet_list_send, packet_dto.requestType);

  const auto &packet = static_cast<const StructDTOClass<ResponceDTO> &>(
                           *packet_list_result.packets[0].structDTOPtr)
                           .getStructDTOClass();

  return packet.reqResult;
}

bool ClientSessionCreateObjects::createNewChatProcessing(
    std::shared_ptr<Chat> &chat, ChatDTO &chat_dto,
    MessageChatDTO &message_chat_dto) {
  PacketDTO chat_packet;
  chat_packet.requestType = RequestType::RqFrClientCreateChat;
  chat_packet.structDTOClassType = StructDTOClassType::chatDTO;
  chat_packet.reqDirection = RequestDirection::ClientToSrv;
  chat_packet.structDTOPtr =
      std::make_shared<StructDTOClass<ChatDTO>>(chat_dto);

  std::vector<PacketDTO> packet_list_send;
  packet_list_send.push_back(chat_packet);

  PacketDTO message_packet;
  message_packet.requestType = RequestType::RqFrClientCreateChat;
  message_packet.structDTOClassType = StructDTOClassType::messageChatDTO;
  message_packet.reqDirection = RequestDirection::ClientToSrv;
  message_packet.structDTOPtr =
      std::make_shared<StructDTOClass<MessageChatDTO>>(message_chat_dto);
  packet_list_send.push_back(message_packet);

  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  response_packet_list = core_.processingRequestToServerCore(
      packet_list_send, RequestType::RqFrClientCreateChat);

  try {
    if (response_packet_list.packets.empty()) {
      throw exc_qt::EmptyPacketException();
    }

    if (response_packet_list.packets[0].requestType !=
        RequestType::RqFrClientCreateChat) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &packet_dto = static_cast<const StructDTOClass<ResponceDTO> &>(
                                 *response_packet_list.packets[0].structDTOPtr)
                                 .getStructDTOClass();

    if (!packet_dto.reqResult) {
      throw exc_qt::CreateChatException();
    }

    auto general_chat_id = packet_dto.anyNumber;
    chat->setChatId(general_chat_id);

    auto general_message_id = parseGetlineToSizeT(packet_dto.anyString);

    if (chat->getMessages().empty()) {
      throw exc_qt::CreateMessageException();
    }

    chat->getMessages().begin()->second->setMessageId(general_message_id);

  } catch (const exc_qt::WrongresponceTypeException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  } catch (const exc_qt::EmptyPacketException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  } catch (const exc_qt::CreateChatException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  } catch (const exc_qt::CreateChatIdException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  } catch (const exc_qt::CreateMessageIdException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  } catch (const exc_qt::CreateMessageException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt = QString::fromStdString(chat_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(chat_dto.chatId));
    const auto message_id_qt = QString::number(0);

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createNewChatProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return false;
  }

  return true;
}

std::size_t ClientSessionCreateObjects::createMessageProcessing(
    const Message &message, std::shared_ptr<Chat> &chat_ptr,
    const std::shared_ptr<User> &user) {
  auto message_ptr = std::make_shared<Message>(message);

  MessageDTO message_dto = dto_builder_.fillOneMessageDTOFromProcessing(
      message_ptr, chat_ptr->getChatId());

  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientCreateMessage;
  packet_dto.structDTOClassType = StructDTOClassType::messageDTO;
  packet_dto.reqDirection = RequestDirection::ClientToSrv;
  packet_dto.structDTOPtr =
      std::make_shared<StructDTOClass<MessageDTO>>(message_dto);

  std::vector<PacketDTO> packet_list_for_send;
  packet_list_for_send.push_back(packet_dto);

  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  response_packet_list = core_.processingRequestToServerCore(
      packet_list_for_send, packet_dto.requestType);

  std::size_t new_message_id = 0;

  try {
    if (response_packet_list.packets.empty()) {
      throw exc_qt::EmptyPacketException();
    }

    if (response_packet_list.packets.size() > 1) {
      throw exc_qt::WrongPacketSizeException();
    }

    if (response_packet_list.packets[0].requestType !=
        RequestType::RqFrClientCreateMessage) {
      throw exc_qt::WrongresponceTypeException();
    }

    const auto &packet_dto = static_cast<const StructDTOClass<ResponceDTO> &>(
                                 *response_packet_list.packets[0].structDTOPtr)
                                 .getStructDTOClass();

    if (!packet_dto.reqResult) {
      throw exc_qt::CreateMessageException();
    }

    new_message_id = parseGetlineToSizeT(packet_dto.anyString);

    if (!user) {
      throw exc_qt::CreateMessageException();
    }

    chat_ptr->addMessageToChat(message_ptr, user, false);
    chat_ptr->getMessages().rbegin()->second->setMessageId(new_message_id);

  } catch (const exc_qt::WrongresponceTypeException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt =
        QString::fromStdString(message_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(message_dto.chatId));
    const auto message_id_qt =
        QString::number(static_cast<long long>(message_dto.messageId));

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createMessageProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return 0;
  } catch (const exc_qt::EmptyPacketException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt =
        QString::fromStdString(message_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(message_dto.chatId));
    const auto message_id_qt =
        QString::number(static_cast<long long>(message_dto.messageId));

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createMessageProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return 0;
  } catch (const exc_qt::WrongPacketSizeException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt =
        QString::fromStdString(message_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(message_dto.chatId));
    const auto message_id_qt =
        QString::number(static_cast<long long>(message_dto.messageId));

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createMessageProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return 0;
  } catch (const exc_qt::CreateMessageException &ex) {
    const auto time_stamp = QString::fromStdString(
        formatTimeStampToString(getCurrentDateTimeInt(), true));
    const auto sender_login_qt =
        QString::fromStdString(message_dto.senderLogin);
    const auto chat_id_qt =
        QString::number(static_cast<long long>(message_dto.chatId));
    const auto message_id_qt =
        QString::number(static_cast<long long>(message_dto.messageId));

    emit exc_qt::ErrorBus::i().error(
        QString::fromUtf8(ex.what()),
        QStringLiteral("[%1]   [ERROR]   [NETWORK]   [user=%2]   [chat_Id=%3]  "
                       " [msg=%4]   createMessageProcessing   ")
            .arg(time_stamp, sender_login_qt, chat_id_qt, message_id_qt));
    return 0;
  }

  return new_message_id;
}
