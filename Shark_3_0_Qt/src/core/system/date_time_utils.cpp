#include "date_time_utils.h"
#include <chrono>
#include <cstdint>
#include <ctime>

std::int64_t makeTimeStamp(int year, int month, int day, int hour, int minute, int second) {
  tm tm = {};
  tm.tm_year = year - 1900; // years since 1900
  tm.tm_mon = month - 1;    // months from 0
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = second;

  time_t timeT = mktime(&tm);                // local time
  return static_cast<int64_t>(timeT) * 1000; // milliseconds
}

int64_t getCurrentDateTimeInt() // returns the number of milliseconds
{

  // got the current time

  auto nowTime = std::chrono::system_clock::now();

  // converted to milliseconds for storage
  int64_t value = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()).count();

  return value;
}

// conversion to a readable form
std::string formatTimeStampToString(std::int64_t timeStamp, bool useLocalTime) {
  auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(timeStamp));
  std::time_t timeT = std::chrono::system_clock::to_time_t(tp);

  std::tm tmBuf;

  if (useLocalTime) {
    localtime_r(&timeT, &tmBuf);
  } else {
    gmtime_r(&timeT, &tmBuf);
  }

  char buffer[64];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H : %M : %S", &tmBuf);

  return std::string(buffer);
}
