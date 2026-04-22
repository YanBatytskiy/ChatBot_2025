#ifndef EXCEPTION_ROUTER_H
#define EXCEPTION_ROUTER_H
#include "exception_login.h"
#include "exception_network.h"
#include "exception_sql.h"
#include "exception_valid.h"

#include <exception>

namespace exc_qt {

struct ExceptionRouter {
  static void report(std::exception_ptr exception_ptr, const QString & ctx) noexcept {
    QString title = "Error";
    QString message = "Unknown exception.";
    try {
      if (exception_ptr) std::rethrow_exception(exception_ptr);
    }
    // LoginException
    catch (const UserNotFoundException& ex) {
      title = "Login"; message = QString::fromUtf8(ex.what());
    }
    catch (const IncorrectPasswordException& ex) {
      title = "Login"; message = QString::fromUtf8(ex.what());
    }
    catch (const LoginException& ex) {
      title = "Login"; message = QString::fromUtf8(ex.what());
    }

// Network Exception
    catch (const CreateSocketTypeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const CreateBufferException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const SocketInvalidException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const ServerFindLANException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const ConnectionToServerException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const LostConnectionException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const ConnectNotAcceptException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    // Send And Receive Exception
    catch (const SendDataException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const ReceiveDataException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  WrongPacketSizeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  EmptyPacketException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  HeaderWrongTypeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  HeaderWrongDataException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  WrongresponceTypeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    // ошибки сериализации
    catch (const  UnsupportedSirializeTypeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  UnsupportedDeSirializeTypeException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    // ошибки создавния объектов
    catch (const  CreateChatException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  CreateMessageException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  CreateUserException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  InternalDataErrorException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }
    catch (const  NetworkException& ex) {
      title = "Network"; message = QString::fromUtf8(ex.what());
    }


    //sql Exception
    catch (const  SQLExecException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLInsertUserException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLEmptyBaseException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLTableAbscentException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLCreateTableException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLReadConfigException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }
    catch (const  SQLException& ex) {
      title = "SQL"; message = QString::fromUtf8(ex.what());
    }


    //Validation Exception
    catch (const  InvalidCharacterException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }
    catch (const  IndexOutOfRangeException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }
    catch (const  InvalidQuantityCharacterException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }
    catch (const  NonCapitalCharacterException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }
    catch (const  NonDigitalCharacterException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }
    catch (const  UnknownException& ex) {
      title = "Validation"; message = QString::fromUtf8(ex.what());
    }

  }
};
};
#endif // EXCEPTION_ROUTER_H
