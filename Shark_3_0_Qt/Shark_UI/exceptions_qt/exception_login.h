#ifndef SHARK_UI_EXCEPTION_LOGIN_H
#define SHARK_UI_EXCEPTION_LOGIN_H
#include "errorbus.h"

namespace exc_qt {

class LoginException : public MyException{
public:
  explicit LoginException(const std::string &message) : MyException("Login exception: "+ message) {};
};

class UserNotFoundException : public LoginException {
public:
  UserNotFoundException() : LoginException("!!!Такой пользователь не найден. "){};
};

class IncorrectPasswordException : public LoginException {
public:
  IncorrectPasswordException() : LoginException("!!!Неверный пароль."){};
};
}
#endif // EXCEPTION_LOGIN_H
