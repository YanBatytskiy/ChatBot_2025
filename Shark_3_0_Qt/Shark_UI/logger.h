#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QThread>
#include <fstream>
#include <shared_mutex>
#include <string>

class Logger : public QObject {
  Q_OBJECT

private:
  std::shared_mutex _mutex;
  std::fstream _writer;
  std::fstream _reader;
  std::string _logFileName{"log.txt"};

  QThread loggerThread;

public:
  explicit Logger();
  ~Logger();

signals:
  void signalWriteLine(const QString &logLine);
  void signalReadLine(qint64 indexLine);
  void signalStopLogger();

public slots:
  void slotStopLogger();
  bool slotWriteLine(const QString &logLine);
  std::multimap<qint64, QString> slotReadLastLine();
  std::multimap<qint64, QString> slotReadSeveralLines(qint64 linesToRead);
  bool slotClearLogFile();

private slots:
  qint64 slotgetLineCount();
};

#endif // LOGGER_H

/*
Уровни логгирования:
TRACE - детальная отладка
DEBUG - отладочная информация
INFO - обычные события
WARN - предупреждения
ERROR - ошибки
CRITICAL - критические ошибки

Ключевые модули для чата:
NETWORK - соединения, пакеты
AUTH - аутентификация
CHAT - сообщения, комнаты
DATABASE - работа с БД
SESSION - управление сессиями

 */
