#pragma once
#include "message/message.h"
#include <cstddef>
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <malloc.h>
#include <windows.h>
#else
#include <alloca.h>
#endif

#define ALPHABET_SIZE 59 ///< Размер алфавита (латиница + кириллица)
#define ENG_SIZE 26      ///< Кол-во символов в английском алфавите
#define RUS_SIZE 32      ///< Кол-во символов в русском алфавите

/**
 * @brief Глобальный алфавит для UTF-8 символов (a-z, а-я).
 */
const std::string alphabet[ALPHABET_SIZE] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o",
                                             "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "а", "б", "в", "г",
                                             "д", "е", "ё", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", "с",
                                             "т", "у", "ф", "х", "ц", "ч", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я"};

const std::string getSystemType();

void printSystemName();

/**
 * @brief Получает длину UTF-8 символа.
 * @param ch Первый байт символа
 * @return Количество байт в символе
 */
std::size_t getUtf8CharLen(unsigned char ch);

/**
 * @brief Получает индекс символа в алфавите.
 * @param ch UTF-8 символ
 * @return Индекс или -1, если символ не найден
 */
int getCharIndex(const std::string &ch);

/**
 * @brief Включает поддержку UTF-8 в консоли.
 *
 * @details Настраивает глобальную локаль и кодировку консоли.
 * Работает стабильно на Windows (MSYS2/MinGW), Linux и macOS.
 */
void enableUTF8Console();

/**
 * @brief Enum class defining message target types.
 */
enum class MessageTarget { One, Several };

/**
 * @brief Converts a string to an integer.
 * @param str The string to convert.
 * @return The converted integer value.
 */
int parseGetlineToInt(const std::string &str); // конвертация из string в int

size_t parseGetlineToSizeT(const std::string &str); // конвертация из string в int

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return The lowercase version of the input string.
 */
std::string TextToLower(const std::string &str); // приведение к нижнему регистру

Message createOneMessage(const std::string &textContent, const std::shared_ptr<User> &sender, const int64_t &timeStamp,
                         std::size_t messageId);
//      //проверяем только на англ буквы и цифры

bool engAndFiguresCheck(const std::string &inputData);

// проверяем только на цифры
bool figuresCheck(const std::string &inputData);

// проверяем только на цифры и запятую
bool figuresAndCommaCheck(const std::string &inputData);