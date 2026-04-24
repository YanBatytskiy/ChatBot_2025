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
Logging levels:
TRACE - detailed debugging
DEBUG - debug information
INFO - ordinary events
WARN - warnings
ERROR - errors
CRITICAL - critical errors

Key modules for the chat:
NETWORK - connections, packets
AUTH - authentication
CHAT - messages, rooms
DATABASE - database operations
SESSION - session management

 */
