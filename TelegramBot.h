#ifndef TELEGRAM_BOT_H
#define TELEGRAM_BOT_H

#include <string>
#include <unordered_map>

// Declare external global variables
extern std::unordered_map<std::string, std::string> env_vars;
extern std::string api_key;
extern std::string api_secret;
extern std::string token;
extern std::string chat_id;

// Function prototypes
void sendTelegramMessage(const std::string& message);
void handleTelegramBotPrint(const std::string& message);

#endif // TELEGRAM_BOT_H
