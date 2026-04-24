#pragma once
#include <cstdint>
#include <string>
#include <ctime>

/**
 * @return String containing the current date and time.
 * @brief Retrieves the current date and time as a string.
 */
// std::string getCurrentDateTime(); // returns a string with the current date and time

int64_t getCurrentDateTimeInt(); // returns the number of milliseconds

// Convert to human-readable form
std::string formatTimeStampToString(std::int64_t timeStamp, bool useLocalTime);
