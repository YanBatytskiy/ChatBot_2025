#include "user_registration_processor.h"

#include "exceptions_cpp/network_exception.h"
#include "exceptions_cpp/sql_exception.h"
#include "server_session.h"

#include <iostream>
#include <libpq-fe.h>
#include <memory>
#include <typeinfo>

UserRegistrationProcessor::UserRegistrationProcessor(
    UserSqlReader &user_sql_reader, MessageSqlWriter &message_sql_writer)
    : user_sql_reader_(user_sql_reader),
      message_sql_writer_(message_sql_writer) {}

bool UserRegistrationProcessor::Process(ServerSession &session,
                                        SessionTransport &transport,
                                        PacketListDTO &packet_list,
                                        const RequestType &request_type,
                                        int connection) {
  PacketListDTO response_packet_list;
  response_packet_list.packets.clear();

  try {
    switch (request_type) {
      case RequestType::RqFrClientCheckLogin: {
        if (packet_list.packets.empty()) {
          throw exc::EmptyPacketException();
        }

        PacketDTO response_packet;
        response_packet.requestType = request_type;
        response_packet.structDTOClassType = StructDTOClassType::responceDTO;
        response_packet.reqDirection = RequestDirection::ClientToSrv;

        const auto &login_packet =
            static_cast<const StructDTOClass<UserLoginDTO> &>(
                *packet_list.packets[0].structDTOPtr)
                .getStructDTOClass();

        ResponceDTO responce_dto;
        responce_dto.anyNumber = 0;

        if (user_sql_reader_.CheckUserLoginSQL(
                login_packet.login, session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = login_packet.login;
        } else {
          responce_dto.reqResult = false;
          responce_dto.anyString = "@";
        }

        response_packet.structDTOPtr =
            std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
        response_packet_list.packets.push_back(response_packet);
        break;
      }
      case RequestType::RqFrClientCheckLogPassword: {
        if (packet_list.packets.empty()) {
          throw exc::EmptyPacketException();
        }

        PacketDTO response_packet;
        response_packet.requestType = request_type;
        response_packet.structDTOClassType = StructDTOClassType::responceDTO;
        response_packet.reqDirection = RequestDirection::ClientToSrv;

        const auto &credentials =
            static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                *packet_list.packets[0].structDTOPtr)
                .getStructDTOClass();

        ResponceDTO responce_dto;
        responce_dto.anyNumber = 0;

        if (user_sql_reader_.CheckUserPasswordSQL(
                credentials, session.getPGConnection())) {
          responce_dto.reqResult = true;
          responce_dto.anyString = credentials.login;
        } else {
          responce_dto.reqResult = false;
          responce_dto.anyString = "@";
        }

        response_packet.structDTOPtr =
            std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
        response_packet_list.packets.push_back(response_packet);
        break;
      }
      case RequestType::RqFrClientRegisterUser: {
        if (packet_list.packets.empty()) {
          throw exc::EmptyPacketException();
        }

        const auto &login_packet =
            static_cast<const StructDTOClass<UserLoginDTO> &>(
                *packet_list.packets[0].structDTOPtr)
                .getStructDTOClass();

        const auto device_payload =
            session.registerOnDeviceDataSrvSQL(login_packet.login);
        if (!device_payload.has_value()) {
          return false;
        }

        response_packet_list = device_payload.value();
        break;
      }
      case RequestType::RqFrClientFindUserByPart: {
        if (packet_list.packets.empty()) {
          throw exc::EmptyPacketException();
        }

        const auto &login_packet =
            static_cast<const StructDTOClass<UserLoginPasswordDTO> &>(
                *packet_list.packets[0].structDTOPtr)
                .getStructDTOClass();

        const auto &user_vector = user_sql_reader_.GetUsersByTextPartSQL(
            login_packet, session.getPGConnection());

        if (user_vector.has_value() && !user_vector->empty()) {
          PacketDTO packet;
          packet.requestType = request_type;
          packet.structDTOClassType = StructDTOClassType::userDTO;
          packet.reqDirection = RequestDirection::ClientToSrv;

          for (const auto &user_dto : user_vector.value()) {
            packet.structDTOPtr =
                std::make_shared<StructDTOClass<UserDTO>>(user_dto);
            response_packet_list.packets.push_back(packet);
          }
        } else {
          ResponceDTO responce_dto{};
          responce_dto.reqResult = false;
          responce_dto.anyNumber = 0;
          responce_dto.anyString = "";

          PacketDTO packet;
          packet.requestType = request_type;
          packet.structDTOClassType = StructDTOClassType::responceDTO;
          packet.reqDirection = RequestDirection::ClientToSrv;
          packet.structDTOPtr =
              std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
          response_packet_list.packets.push_back(packet);
        }
        break;
      }
      case RequestType::RqFrClientSetLastReadMessage: {
        if (packet_list.packets.empty()) {
          throw exc::EmptyPacketException();
        }

        PacketDTO response_packet;
        response_packet.requestType = request_type;
        response_packet.structDTOClassType = StructDTOClassType::responceDTO;
        response_packet.reqDirection = RequestDirection::ClientToSrv;

        const auto &message_packet =
            static_cast<const StructDTOClass<MessageDTO> &>(
                *packet_list.packets[0].structDTOPtr)
                .getStructDTOClass();

        auto value = message_sql_writer_.SetLastReadMessageSQL(
            message_packet, session.getPGConnection());

        ResponceDTO responce_dto;
        responce_dto.anyNumber = 0;
        responce_dto.anyString = "";
        responce_dto.reqResult = value;

        if (!value) {
          std::cerr << "Сервер: RqFrClientSetLastReadMessage."
                    << " Ошибка базы."
                    << " Значение не установлено."
                    << std::endl;
        }

        response_packet.structDTOPtr =
            std::make_shared<StructDTOClass<ResponceDTO>>(responce_dto);
        response_packet_list.packets.push_back(response_packet);
        break;
      }
      default:
        throw exc::HeaderWrongTypeException();
    }

    if (!response_packet_list.packets.empty()) {
      session.sendPacketListDTO(transport, response_packet_list, connection);
    }
  } catch (const exc::SendDataException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const exc::NetworkException &ex) {
    std::cerr << "Сервер: " << ex.what() << std::endl;
    return false;
  } catch (const std::bad_cast &ex) {
    std::cerr << "Сервер: Неправильный тип пакета. "
              << ex.what() << std::endl;
    return false;
  }

  return true;
}
