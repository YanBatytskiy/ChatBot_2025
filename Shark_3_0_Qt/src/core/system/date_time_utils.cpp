#include "date_time_utils.h"
#include <chrono>
#include <cstdint>
#include <ctime>

std::int64_t makeTimeStamp(int year, int month, int day, int hour, int minute, int second) {
  tm tm = {};
  tm.tm_year = year - 1900; // годы от 1900
  tm.tm_mon = month - 1;    // месяцы от 0
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  time_t timeT = mktime(&tm);                // локальное время
  return static_cast<int64_t>(timeT) * 1000; // миллисекунды
}

int64_t getCurrentDateTimeInt() // вернет количество миллисекунд
{

  // получили текущее время

  auto nowTime = std::chrono::system_clock::now();

  // перевели в миллисекунды для хранения
  int64_t value = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()).count();

  return value;
}

// перевод в читаемый вид
std::string formatTimeStampToString(std::int64_t timeStamp, bool useLocalTime) {
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(timeStamp));
  std::time_t timeT = std::chrono::system_clock::to_time_t(tp);

  std::tm tmBuf;

  if (useLocalTime) {
#ifdef _WIN32
    std::tm* tm = localtime(&timeT);
    if (tm)
      tmBuf = *tm;
#else
    localtime_r(&timeT, &tmBuf);
#endif
  } else {
#ifdef _WIN32
    std::tm* tm = gmtime(&timeT);
    if (tm)
      tmBuf = *tm;
#else
    gmtime_r(&timeT, &tmBuf);
#endif
  }

  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H : %M : %S", &tmBuf);

  return std::string(buffer);
}
