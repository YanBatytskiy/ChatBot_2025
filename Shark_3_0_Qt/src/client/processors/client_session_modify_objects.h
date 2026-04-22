#pragma once

#include <memory>

#include "dto_struct.h"

class ClientSession;
class ClientCore;

class ClientSessionModifyObjects {
 public:
  ClientSessionModifyObjects(ClientSession &session, ClientCore &core);

  bool changeUserDataProcessing(const UserDTO &user_dto);

 private:
  ClientSession &session_;
  ClientCore &core_;
};
