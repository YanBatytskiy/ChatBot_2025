#include "date_time_utils.h"
#include <chrono>
#include <cstdint>
#include <ctime>

/**
 * @brief Retrieves the current date and time as a string.
 * @return String in the format "YYYY-MM-DD, HH:MM:SS".
 * @details Uses local time to format the current date and time.
 */
// std::string getCurrentDateTime() { // вернет строку с текущей датой и временем

//   time_t mytime = time(NULL);
//   struct tm *now = localtime(&mytime);

//   char date_stamp[20];
//   char time_stamp[20];

//   std::strftime(date_stamp, sizeof(date_stamp), "%Y-%m-%d", now);
//   std::strftime(time_stamp, sizeof(time_stamp), "%H:%M:%S", now);

//   return std::string(date_stamp) + ", " + time_stamp;
// }

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
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tmBuf);

  return std::string(buffer);
}
