#include "client/processors/client_session_modify_objects.h"

#include <vector>

#include "client/client_session.h"

ClientSessionModifyObjects::ClientSessionModifyObjects(ClientSession &session,
                                                       ClientCore &core)
    : session_(session), core_(core) {}

bool ClientSessionModifyObjects::changeUserDataProcessing(
    const UserDTO &user_dto) {
  PacketDTO packet_dto;
  packet_dto.requestType = RequestType::RqFrClientChangeUserData;
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
