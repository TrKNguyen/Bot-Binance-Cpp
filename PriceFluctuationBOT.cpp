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
    api_key = getEnv("binance_api_key");
    api_secret = getEnv("binance_api_secret");

    if (api_key.empty() || api_secret.empty()) {
        cerr << "API keys are missing in the .env file." << endl;
        exit(1);
    }
}

Json::Value getHistoricalKlines(const string& symbol, const string& interval, const string& startTime, int limit) {
    string url = "https://fapi.binance.com/fapi/v1/klines?symbol=" + symbol + 
                 "&interval=" + interval + 
                 "&startTime=" + startTime + 
                 "&limit=" + to_string(limit);
    return getRequestData(url);
}

vector<string> getHardcodedCoins() {
    return {
        "BTCUSDT", "ETHUSDT", "BCHUSDT", "XRPUSDT", "EOSUSDT", "LTCUSDT", "TRXUSDT", "ETCUSDT", "LINKUSDT", "XLMUSDT", 
        "ADAUSDT", "XMRUSDT", "DASHUSDT", "ZECUSDT", "XTZUSDT", "BNBUSDT", "ATOMUSDT", "ONTUSDT", "IOTAUSDT", "BATUSDT", 
        "VETUSDT", "NEOUSDT", "QTUMUSDT", "IOSTUSDT", "THETAUSDT", "ALGOUSDT", "ZILUSDT", "KNCUSDT", "ZRXUSDT", "COMPUSDT", 
        "OMGUSDT", "DOGEUSDT", "SXPUSDT", "KAVAUSDT", "BANDUSDT", "RLCUSDT", "MKRUSDT", "SNXUSDT", "DOTUSDT", "DEFIUSDT", 
        "YFIUSDT", "BALUSDT", "CRVUSDT", "TRBUSDT", "RUNEUSDT", "SUSHIUSDT", "EGLDUSDT", "SOLUSDT", "ICXUSDT", "STORJUSDT", 
        "UNIUSDT", "AVAXUSDT", "FTMUSDT", "ENJUSDT", "FLMUSDT", "KSMUSDT", "NEARUSDT", "AAVEUSDT", "FILUSDT", "RSRUSDT", 
        "LRCUSDT", "BELUSDT", "AXSUSDT", "ALPHAUSDT", "ZENUSDT", "SKLUSDT", "GRTUSDT", "1INCHUSDT", "CHZUSDT", "SANDUSDT", 
        "ANKRUSDT", "LITUSDT", "REEFUSDT", "RVNUSDT", "SFPUSDT", "COTIUSDT", "CHRUSDT", "MANAUSDT", "ALICEUSDT", "HBARUSDT", 
        "ONEUSDT", "LINAUSDT", "STMXUSDT", "DENTUSDT", "CELRUSDT", "HOTUSDT", "MTLUSDT", "OGNUSDT", "NKNUSDT", "1000SHIBUSDT", 
        "BAKEUSDT", "GTCUSDT", "BTCDOMUSDT", "IOTXUSDT", "C98USDT", "MASKUSDT", "ATAUSDT", "DYDXUSDT", "1000XECUSDT", "GALAUSDT", 
        "CELOUSDT", "ARUSDT", "ARPAUSDT", "CTSIUSDT", "LPTUSDT", "ENSUSDT", "PEOPLEUSDT", "ROSEUSDT", "DUSKUSDT", "FLOWUSDT", 
        "IMXUSDT", "API3USDT", "GMTUSDT", "APEUSDT", "WOOUSDT", "JASMYUSDT", "OPUSDT", "INJUSDT", "STGUSDT", "SPELLUSDT", 
        "1000LUNCUSDT", "LUNA2USDT", "LDOUSDT", "ICPUSDT", "APTUSDT", "QNTUSDT", "FETUSDT", "FXSUSDT", "HOOKUSDT", "MAGICUSDT", 
        "TUSDT", "HIGHUSDT", "MINAUSDT", "ASTRUSDT", "PHBUSDT", "GMXUSDT", "CFXUSDT", "STXUSDT", "BNXUSDT", "ACHUSDT", "SSVUSDT"
    };
}

int main() {
    initializeKeys();
    vector<string> coins = getHardcodedCoins();

    cout << "Total Coins: " << coins.size() << endl;

    while (true) {
        double mx = 0;
        string coinMax;
        string utcNow = to_string(chrono::duration_cast<chrono::milliseconds>(
                                       chrono::system_clock::now().time_since_epoch() - chrono::seconds(310))
                                       .count());

        for (const auto& coin : coins) {
            try {
                Json::Value result = getHistoricalKlines(coin, "5m", utcNow, 1);

                if (result.empty()) continue;

                double high = stod(result[0][2].asString());
                double low = stod(result[0][3].asString());
                double open = stod(result[0][1].asString());
                double close = stod(result[0][4].asString());

                double val = ((high - low) / open) * 100;

                if (mx < val && close > open) {
                    mx = val;
                    coinMax = coin;
                    cout << "Max Coin: " << coinMax << " | Value: " << mx << "%" << endl;
                }

                if (val > 1.5 && coinMax != coin) {
                    cout << "Significant Coin: " << coin << " | Value: " << val << "%" << endl;
                }

                this_thread::sleep_for(chrono::seconds(1));
            } catch (const exception& e) {
                cerr << "Error processing coin: " << coin << " | " << e.what() << endl;
                this_thread::sleep_for(chrono::seconds(30));
                continue;
            }
        }

        this_thread::sleep_for(chrono::seconds(1));
    }

    return 0;
}
