#pragma once
#include <cstddef>
#include <memory>
#include <optional>
#include <string>

class Chat;
class ClientSession;
class Message;

std::optional<Message> inputNewMessage(ClientSession &clientSession, std::shared_ptr<Chat> chat);

bool checkNewDataInputForLimits(const std::string &inputData, std::size_t contentLengthMin,
                                std::size_t contentLengthMax, bool isPassword);

std::string inputDataValidation(const std::string &prompt, std::size_t dataLengthMin, std::size_t dataLengthMax,
                                bool isPassword, bool newUserData);
