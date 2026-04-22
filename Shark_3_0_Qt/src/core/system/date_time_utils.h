#pragma once
#include "string"
#include <cstdint>
#include <ctime>

std::int64_t makeTimeStamp(int year, int month, int day, int hour, int minute, int second);

int64_t getCurrentDateTimeInt(); // вернет количество миллисекунд

// перевод в читаемый вид
std::string formatTimeStampToString(std::int64_t timeStamp, bool useLocalTime);