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
Json::Value getHistoricalKlines(const string& symbol, const string& interval, const string& startTime, int limit) {
    string url = "https://fapi.binance.com/fapi/v1/klines?symbol=" + symbol + 
                 "&interval=" + interval + 
                 "&startTime=" + startTime + 
                 "&limit=" + to_string(limit);
    return getRequestData(url);
}

int main() {
    initializeKeys();
    vector<string> coins = getHardcodedCoins();

    cout << "Total Hardcoded Coins: " << coins.size() << endl;

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
                cout << high << " " << priceNow <<" check" << " " << (high - priceNow) / priceNow<< endl; 
                if (high > priceNow && (high - priceNow) / priceNow > 0.02) { // suddenly increase 2% 
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
