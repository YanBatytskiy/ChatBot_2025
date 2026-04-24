#include "logger.h"
#include <QApplication>
#include <QThread>
#include <QVector>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

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

Logger::Logger() {

  QString path = QCoreApplication::applicationDirPath() + "/log.txt";

  connect(this, &Logger::signalWriteLine, this, &Logger::slotWriteLine);

  _logFileName = path.toStdString();

  _writer.open(_logFileName, std::ios::out | std::ios::app);
  _reader.open(_logFileName, std::ios::in);

  this->moveToThread(&loggerThread);
  loggerThread.start();
}

Logger::~Logger() {
  loggerThread.quit();
  loggerThread.wait();

  _writer.flush();
  _writer.close();
  _reader.close();
}

bool Logger::slotWriteLine(const QString &logLine) {

  if (!_writer.is_open() || !_reader.is_open())
    return false;

  std::unique_lock<std::shared_mutex> lk(_mutex);

  if (!_writer.good()) {
    _writer.clear();
    _writer.seekp(0, std::ios::end);
    if (!_writer.good()) {
      _writer.close();
      _writer.open(_logFileName, std::ios::out | std::ios::app);
      if (!_writer.good())
        return false;
    }
  }

  _reader.clear();
  _reader.seekg(0, std::ios::end);
  const std::streampos endPos = _reader.tellg();

  if (endPos > std::streampos(0)) {
    _reader.seekg(endPos - std::streamoff(1));
    char lastChar = '\0';
    _reader.get(lastChar);

    if (lastChar != '\n')
      _writer.put('\n');
  }

  const QByteArray utf8 = logLine.toUtf8();
  _writer.write(utf8.constData(), utf8.size());
  _writer.put('\n');
  _writer.flush();

  return _writer.good();
}

std::multimap<qint64, QString> Logger::slotReadLastLine() {

  std::multimap<qint64, QString> result;
  result.clear();

  if (!_reader.is_open())
    return result;

  const qint64 quantity = slotgetLineCount();

  std::shared_lock<std::shared_mutex> lk(_mutex);

  _reader.clear();
  _reader.seekg(0, std::ios::end);
  std::streampos pos = _reader.tellg();

  if (pos == std::streampos(0))
    return result;

  // step back from the end
  pos -= std::streamoff(1);

  // if a '\n' is at the end, skip trailing newlines
  char ch = '\0';
  _reader.seekg(pos);
  _reader.get(ch);

  while (pos > std::streampos(0) && ch == '\n') {
    pos -= std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // walk back to the start of the line or the start of the file
  while (pos > std::streampos(0) && ch != '\n') {
    pos -= std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // if '\n' was found, shift forward by one to the start of the line
  if (ch == '\n')
    pos += std::streamoff(1);

  std::string str;
  _reader.clear();
  _reader.seekg(pos);
  std::getline(_reader, str);
  if (str.empty())
    return result;

  result.insert(std::pair{quantity, QString::fromStdString(str)});

  return result;
}

std::multimap<qint64, QString> Logger::slotReadSeveralLines(qint64 linesToRead) {

  std::multimap<qint64, QString> result;
  result.clear();

  if (!_reader.is_open() || linesToRead < 0)
    return result;

  const auto quantityLines = slotgetLineCount();
  if (quantityLines <= 0)
    return result;

  if (linesToRead == 0)
    linesToRead = quantityLines;

  std::shared_lock<std::shared_mutex> lk(_mutex);

  _reader.clear();
  _reader.seekg(0, std::ios::end);
  std::streampos pos = _reader.tellg();
  if (pos == std::streampos(0))
    return result; // empty file

  // start from the last byte
  pos -= std::streamoff(1);

  char ch = '\0';
  _reader.seekg(pos);
  _reader.get(ch);

  // gather the last N lines, moving backwards
  std::vector<std::string> lines; // temporarily in direct order
  while (true) {
    // skip trailing newlines between iterations
    while (pos > std::streampos(0) && ch == '\n') {
      pos -= std::streamoff(1);
      _reader.seekg(pos);
      _reader.get(ch);
    }
    if (pos == std::streampos(0) && ch == '\n')
      break;

    // reach the start of the line or the file
    while (pos > std::streampos(0) && ch != '\n') {
      pos -= std::streamoff(1);
      _reader.seekg(pos);
      _reader.get(ch);
    }

    // position is at '\n' or at 0
    std::streampos lineStart;
    if (ch == '\n') {
      lineStart = pos + std::streamoff(1);
    } else {
      lineStart = std::streampos(0);
    }

    // read the line
    _reader.clear();
    _reader.seekg(lineStart);
    std::string s;
    std::getline(_reader, s);
    lines.push_back(std::move(s));

    if (static_cast<qint64>(lines.size()) >= linesToRead)
      break;
    if (lineStart == std::streampos(0))
      break; // reached the start of the file

    // move to just before the found line and continue searching for the previous one
    pos = lineStart - std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // return in correct order from oldest to newest

  qint64 row_number = quantityLines > static_cast<qint64>(lines.size())
                        ? (quantityLines - static_cast<qint64>(lines.size()))
                        : 0;

  for (auto it = lines.rbegin(); it != lines.rend(); ++it) {
    result.insert(std::pair{row_number, QString::fromStdString(*it)});
    ++row_number;
  }
  return result;
}

qint64 Logger::slotgetLineCount() {
  if (!_reader.is_open())
    return -1;

  std::shared_lock<std::shared_mutex> lk(_mutex);

  _reader.clear();
  _reader.seekg(0, std::ios::end);
  std::streampos end = _reader.tellg();
  if (end == std::streampos(0))
    return 0; // empty file

  // count the number of '\n'
  _reader.seekg(0, std::ios::beg);
  qint64 newlines = std::count(std::istreambuf_iterator<char>(_reader),
                               std::istreambuf_iterator<char>(), '\n');

  // check whether the file ends with '\n'
  _reader.clear();
  _reader.seekg(end - std::streamoff(1));
  char last = '\0';
  _reader.get(last);

  // if the last character is '\n', line count equals newlines, otherwise +1
  return (last == '\n') ? newlines : (newlines + 1);
}

bool Logger::slotClearLogFile() {
  if (_logFileName.empty())
    return false;

  std::unique_lock<std::shared_mutex> lk(_mutex);

  _writer.flush();
  _writer.close();
  _reader.close();

  std::ofstream trunc(_logFileName, std::ios::out | std::ios::trunc);
  const bool ok = trunc.is_open();
  trunc.close();

  _writer.open(_logFileName, std::ios::out | std::ios::app);
  _reader.open(_logFileName, std::ios::in);

  return ok && _writer.is_open() && _reader.is_open();
}

void Logger::slotStopLogger() {
  loggerThread.quit();
  loggerThread.wait();
}
