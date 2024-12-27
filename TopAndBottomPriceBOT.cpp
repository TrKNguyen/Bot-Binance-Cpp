#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <json/json.h>
#include <cmath>
#include <unordered_map>
#include <fstream>
#include <ctime>

using namespace std;

unordered_map<string, string> env_vars;
string api_key;
string api_secret;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

Json::Value getRequestData(const string& url) {
    CURL* curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        string readBuffer;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        }

        curl_easy_cleanup(curl);

        Json::Reader reader;
        Json::Value data;
        reader.parse(readBuffer, data);
        return data;
    }
    curl_global_cleanup();
    return Json::Value(); 
}

void loadEnv(const string& filename) {
    ifstream file(filename);
    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != string::npos) {
            string key = line.substr(0, delimiter_pos);
            string value = line.substr(delimiter_pos + 1);
            env_vars[key] = value;
        }
    }
    file.close();
}

string getEnv(const string& key) {
    return env_vars[key];
}

void initializeKeys() {
    loadEnv(".env");
    api_key = getEnv("BINANCE_API_KEY");
    api_secret = getEnv("BINANCE_API_SECRET");

    if (api_key.empty() || api_secret.empty()) {
        cerr << "API keys are missing in the .env file." << endl;
        exit(1);
    }
}

vector<string> getCoinsFromAPI() {
    string url = "https://api.binance.com/api/v3/exchangeInfo";
    Json::Value data = getRequestData(url);
    vector<string> coins;

    for (const auto& symbol : data["symbols"]) {
        if (symbol["status"].asString() == "TRADING" &&
            symbol["symbol"].asString().substr(symbol["symbol"].asString().length() - 4) == "USDT") {
            coins.push_back(symbol["symbol"].asString());
        }
    }
    return coins;
}

Json::Value getHistoricalKlines(const string& symbol, const string& interval, const string& startTime, int limit) {
    string url = "https://fapi.binance.com/fapi/v1/klines?symbol=" + symbol + "&interval=" + interval + "&startTime=" + startTime + "&limit=" + to_string(limit);
    return getRequestData(url);
}

bool checkPriceDrop(const Json::Value& result, int mid) {
    double priceNow = stod(result[result.size() - 1][4].asString());

    for (const auto& day : result) {
        double high = stod(day[2].asString());
        time_t startTime = stoll(day[6].asString()) / 1000;
        time_t endTime = stoll(result[result.size() - 1][6].asString()) / 1000;
        int durationDays = difftime(endTime, startTime) / (60 * 60 * 24);

        if (durationDays > mid) continue;
        if (high > priceNow && (high - priceNow) / priceNow > 0.05) {
            return false;
        }
    }
    return true;
}

bool checkPriceIncrease(const Json::Value& result, int mid) {
    double priceNow = stod(result[result.size() - 1][4].asString());

    for (const auto& day : result) {
        double low = stod(day[3].asString());
        time_t startTime = stoll(day[6].asString()) / 1000;
        time_t endTime = stoll(result[result.size() - 1][6].asString()) / 1000;
        int durationDays = difftime(endTime, startTime) / (60 * 60 * 24);

        if (durationDays > mid) continue;
        if (low < priceNow && (priceNow - low) / low > 0.02) {
            return false;
        }
    }
    return true;
}

int main() {
    initializeKeys();
    vector<string> coins = getCoinsFromAPI();
    vector<string> excludedCoins = {"BTCSTUSDT", "TLMUSDT", "SCUSDT", "FTTUSDT", "RENUSDT", "HNTUSDT", "BTSUSDT", "RAYUSDT", "SRMUSDT", "CVCUSDT"};

    for (const auto& excluded : excludedCoins) {
        coins.erase(remove(coins.begin(), coins.end(), excluded), coins.end());
    }

    while (true) {
        for (const auto& coin : coins) {
            if (coin.substr(coin.size() - 4) != "USDT") continue;

            string utcNow = to_string(chrono::duration_cast<chrono::milliseconds>(
                                           chrono::system_clock::now().time_since_epoch() - chrono::hours(24 * 365))
                                           .count());

            Json::Value result = getHistoricalKlines(coin, "1d", utcNow, 360);

            if (result.empty()) continue;

            int idPre = 0;
            double priceNow = stod(result[result.size() - 1][4].asString());

            for (int i = 0; i < result.size(); i++) {
                double high = stod(result[i][2].asString());
                if (high > priceNow && (high - priceNow) / priceNow > 0.02) {
                    idPre = i;
                }
            }
            if (result.size() - idPre > 30) {
                cout << coin << " " << result.size() - idPre << " top" << endl;
            }

            idPre = 0;
            for (int i = 0; i < result.size(); i++) {
                double low = stod(result[i][3].asString());
                if (low < priceNow && (priceNow - low) / low > 0.02) {
                    idPre = i;
                }
            }
            if (result.size() - idPre > 30) {
                cout << coin << " " << result.size() - idPre << " bottom" << endl;
            }
        }

        this_thread::sleep_for(chrono::minutes(5));
    }

    return 0;
}
