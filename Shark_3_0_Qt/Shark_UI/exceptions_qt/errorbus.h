#ifndef ERRORBUS_H
#define ERRORBUS_H
#include <QObject>

namespace exc_qt {

class ErrorBus : public QObject {
  Q_OBJECT
public:
  static ErrorBus& i(){static ErrorBus errorBus; return errorBus;}
signals:
  void error(QString message, QString context);
};



class MyException : public std::exception {
private:
  std::string _message; ///< The error message.

public:
  explicit MyException(const std::string &message) : _message(message) {};

  const char *what() const noexcept override { return _message.c_str(); };
};
};
#endif // ERRORBUS_H
