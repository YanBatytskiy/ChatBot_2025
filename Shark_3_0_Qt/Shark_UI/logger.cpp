#include "logger.h"
#include <QApplication>
#include <QThread>
#include <QVector>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

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

  // шаг назад от конца
  pos -= std::streamoff(1);

  // если в конце стоит '\n', пропускаем хвостовые переводы строки
  char ch = '\0';
  _reader.seekg(pos);
  _reader.get(ch);

  while (pos > std::streampos(0) && ch == '\n') {
    pos -= std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // идём назад до начала строки или начала файла
  while (pos > std::streampos(0) && ch != '\n') {
    pos -= std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // если нашли '\n', сместиться на символ вперёд к началу строки
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
    return result; // пустой файл

  // начать с последнего байта
  pos -= std::streamoff(1);

  char ch = '\0';
  _reader.seekg(pos);
  _reader.get(ch);

  // собрать последние N строк, двигаясь назад
  std::vector<std::string> lines; // временно в прямом виде
  while (true) {
    // пропустить хвостовые переводы строки между итерациями
    while (pos > std::streampos(0) && ch == '\n') {
      pos -= std::streamoff(1);
      _reader.seekg(pos);
      _reader.get(ch);
    }
    if (pos == std::streampos(0) && ch == '\n')
      break;

    // дойти до начала строки или файла
    while (pos > std::streampos(0) && ch != '\n') {
      pos -= std::streamoff(1);
      _reader.seekg(pos);
      _reader.get(ch);
    }

    // позиция стоит на '\n' или на 0
    std::streampos lineStart;
    if (ch == '\n') {
      lineStart = pos + std::streamoff(1);
    } else {
      lineStart = std::streampos(0);
    }

    // прочитать строку
    _reader.clear();
    _reader.seekg(lineStart);
    std::string s;
    std::getline(_reader, s);
    lines.push_back(std::move(s));

    if (static_cast<qint64>(lines.size()) >= linesToRead)
      break;
    if (lineStart == std::streampos(0))
      break; // достигли начала файла

    // перейти перед найденную строку и продолжить поиск предыдущей
    pos = lineStart - std::streamoff(1);
    _reader.seekg(pos);
    _reader.get(ch);
  }

  // вернуть в правильном порядке от старой к новой

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
    return 0; // пустой файл

  // посчитать количество '\n'
  _reader.seekg(0, std::ios::beg);
  qint64 newlines = std::count(std::istreambuf_iterator<char>(_reader),
                               std::istreambuf_iterator<char>(), '\n');

  // проверить, заканчивается ли файл на '\n'
  _reader.clear();
  _reader.seekg(end - std::streamoff(1));
  char last = '\0';
  _reader.get(last);

  // если последний символ '\n', то строк ровно newlines, иначе +1
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
