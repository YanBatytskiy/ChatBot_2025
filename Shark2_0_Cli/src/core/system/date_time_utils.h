#pragma once
#include "string"
#include <cstdint>
#include <ctime>

/**
 * @return String containing the current date and time.
 * @brief Retrieves the current date and time as a string.
 */
// std::string getCurrentDateTime(); // вернет строку с текущей датой и временем

int64_t getCurrentDateTimeInt(); // вернет количество миллисекунд

// перевод в читаемый вид
std::string formatTimeStampToString(std::int64_t timeStamp, bool useLocalTime);