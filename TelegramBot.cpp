#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <cstdlib>
#include <unistd.h> 
#include "TelegramBot.h"
using namespace std;

// Function to send a message via Telegram
void sendTelegramMessage(const string& message) {
    CURL *curl;
    CURLcode res;

    // Prepare the API URL and the message parameters
    string url = "https://api.telegram.org/bot" + token + "/sendMessage";
    string data = "chat_id=" + chat_id + "&text=" + message;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set the URL for the request
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        
        // Set the POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        
        // Perform the request
        res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        } else {
            cout << "Message sent successfully to Telegram!" << endl;
        }
        
        // Clean up curl
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}

void handleTelegramBotPrint(const string& message) {
    sendTelegramMessage(message);
}

// void getUpdates() {
//     CURL *curl;
//     CURLcode res;

//     string url = "https://api.telegram.org/bot" + token + "/getUpdates";

//     curl_global_init(CURL_GLOBAL_DEFAULT);
//     curl = curl_easy_init();

//     if (curl) {
//         curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
//         res = curl_easy_perform(curl);

//         if (res != CURLE_OK) {
//             cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
//         } else {
//             cout << "Updates fetched successfully!" << endl;
//         }
//         curl_easy_cleanup(curl);
//     }

//     curl_global_cleanup();
// }

// // Simulate the message handler (just for example, handling "/start")
// void simulateMessage(const string& command) {
//     if (command == "/start") {
//         string welcome_message = "Welcome! I'm your bot. How can I help you?";
//         sendTelegramMessage(welcome_message);
//     }
// }
