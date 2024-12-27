#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <curl/curl.h>
#include <json/json.h>
#include <cmath>
#include <unordered_map>
#include <fstream>

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

int main() {
    initializeKeys();
    vector<string> coins = getCoinsFromAPI();
    random_shuffle(coins.begin(), coins.end());
    cout << "Total Coins: " << coins.size() << endl;

    while (true) {
        string utc_now = to_string(chrono::duration_cast<chrono::seconds>(
                                       chrono::system_clock::now().time_since_epoch())
                                       .count() - 310) + "000";

        string coin_max;
        double val_max = -1;

        for (const auto& coin : coins) {
            try {
                string url_spot = "https://api.binance.com/api/v3/klines?symbol=" + coin +
                                  "&interval=5m&startTime=" + utc_now + "&limit=1";

                string url_futures = "https://fapi.binance.com/fapi/v1/klines?symbol=" + coin +
                                     "&interval=5m&startTime=" + utc_now + "&limit=1";

                Json::Value result_spot = getRequestData(url_spot);
                Json::Value result_futures = getRequestData(url_futures);

                if (result_spot.empty() || result_futures.empty()) continue;

                double close_spot = stod(result_spot[0][4].asString());
                double close_futures = stod(result_futures[0][4].asString());
                double diff = ((close_spot - close_futures) / close_spot) * 100.0;

                if (diff > val_max) {
                    val_max = diff;
                    coin_max = coin;
                }

                if (diff > 1) {
                    cout << coin << " has a significant difference: " << diff << "%" << endl;
                }
            } catch (const exception& e) {
                cerr << "Error processing coin: " << coin << ", " << e.what() << endl;
                continue;
            }
        }

        cout << "Coin with max difference: " << coin_max << " | Difference: " << val_max << "%" << endl;
        this_thread::sleep_for(chrono::minutes(5));
    }

    return 0;
}