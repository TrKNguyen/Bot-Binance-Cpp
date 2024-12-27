#include <iostream>
#include <string>
#include <vector>
#include <curl/curl.h>
#include <json/json.h> 
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <mutex>

using namespace std;

#define endl "\n" 

// mutex init_mutex;
unordered_map<string, string> env_vars;
string api_key; 
string api_secret; 
string token; 
string chat_id; 

// Function to load critical informations from .env 
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

// Function to retrieve environment variable value
string getEnv(const string& key) {
    return env_vars[key];
}

string trim(const string &str) { // critical error : somehow it have \n or sth make it wrong format
    size_t first = str.find_first_not_of(" \t\n\r");
    size_t last = str.find_last_not_of(" \t\n\r");
    return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
}

// initialize and fetch data from .env file 
void initialization() {
    // Lock the mutex to ensure exclusive access to shared resources
    // std::lock_guard<std::mutex> lock(init_mutex);
    
    // Load environment variables and initialize values
    loadEnv(".env"); 
    api_key = trim(getEnv("binance_api_key"));
    api_secret = trim(getEnv("binance_api_secret"));
    token = trim(getEnv("BOT_TOKEN"));
    chat_id = trim(getEnv("MY_TELEGRAM_CHAT_ID"));
    
    // Print value for debugging only :D 
    // cout << "Token: " << token << " Chat ID: " << chat_id << endl; 
    // cout << token << endl; 
    // cout << chat_id << endl; 
}

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

void sendTelegramMessage(const string& message) {
    CURL *curl;
    CURLcode res;
    string url = "https://api.telegram.org/bot" + token + "/sendMessage";
    string data = "chat_id=" + chat_id + "&text=" + message;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
        res = curl_easy_perform(curl);
        
        // if (res != CURLE_OK) {
        //     cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << endl;
        // } else {
        //     cout << "Message sent successfully to Telegram!" << endl;
        // }
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
}


void handleTelegramBotPrint(const string& message) {
    sendTelegramMessage(message);
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
        "TUSDT", "HIGHUSDT", "MINAUSDT", "ASTRUSDT", "PHBUSDT", "GMXUSDT", "CFXUSDT", "STXUSDT", "BNXUSDT", "ACHUSDT", "SSVUSDT", 
        "CKBUSDT", "PERPUSDT", "TRUUSDT", "LQTYUSDT", "USDCUSDT", "IDUSDT", "ARBUSDT", "JOEUSDT", "TLMUSDT", "AMBUSDT", "LEVERUSDT", 
        "RDNTUSDT", "HFTUSDT", "XVSUSDT", "ETHBTC", "BLURUSDT", "EDUUSDT", "SUIUSDT", "1000PEPEUSDT", "1000FLOKIUSDT", "UMAUSDT", 
        "COMBOUSDT", "NMRUSDT", "MAVUSDT", "XVGUSDT", "WLDUSDT", "PENDLEUSDT", "ARKMUSDT", "AGLDUSDT", "YGGUSDT", "DODOXUSDT", 
        "BNTUSDT", "OXTUSDT", "SEIUSDT", "CYBERUSDT", "HIFIUSDT", "ARKUSDT", "BICOUSDT", "BIGTIMEUSDT", "WAXPUSDT", "BSVUSDT", 
        "RIFUSDT", "POLYXUSDT", "GASUSDT", "POWRUSDT", "TIAUSDT", "CAKEUSDT", "MEMEUSDT", "TWTUSDT", "TOKENUSDT", "ORDIUSDT", 
        "STEEMUSDT", "BADGERUSDT", "ILVUSDT", "NTRNUSDT", "KASUSDT", "BEAMXUSDT", "1000BONKUSDT", "PYTHUSDT", "SUPERUSDT", "USTCUSDT", 
        "ONGUSDT", "ETHWUSDT", "JTOUSDT", "1000SATSUSDT", "AUCTIONUSDT", "1000RATSUSDT", "ACEUSDT", "MOVRUSDT", "NFPUSDT", "BTCUSDC", 
        "ETHUSDC", "BNBUSDC", "SOLUSDC", "XRPUSDC", "AIUSDT", "XAIUSDT", "DOGEUSDC", "WIFUSDT", "MANTAUSDT", "ONDOUSDT", "LSKUSDT", 
        "ALTUSDT", "JUPUSDT", "ZETAUSDT", "RONINUSDT", "DYMUSDT", "SUIUSDC", "OMUSDT", "LINKUSDC", "PIXELUSDT", "STRKUSDT", "ORDIUSDC", 
        "GLMUSDT", "PORTALUSDT", "TONUSDT", "AXLUSDT", "MYROUSDT", "1000PEPEUSDC", "METISUSDT", "AEVOUSDT", "WLDUSDC", "VANRYUSDT", 
        "BOMEUSDT", "ETHFIUSDT", "AVAXUSDC", "1000SHIBUSDC", "ENAUSDT", "WUSDT", "WIFUSDC", "BCHUSDC", "TNSRUSDT", "SAGAUSDT", 
        "LTCUSDC", "NEARUSDC", "TAOUSDT", "OMNIUSDT", "ARBUSDC", "NEOUSDC", "FILUSDC", "TIAUSDC", "BOMEUSDC", "REZUSDT", "ENAUSDC", 
        "ETHFIUSDC", "1000BONKUSDC", "BBUSDT", "NOTUSDT", "TURBOUSDT", "IOUSDT", "ZKUSDT", "MEWUSDT", "LISTAUSDT", "ZROUSDT", 
        "BTCUSDT_241227", "ETHUSDT_241227", "CRVUSDC", "RENDERUSDT", "BANANAUSDT", "RAREUSDT", "GUSDT", "SYNUSDT", "SYSUSDT", 
        "VOXELUSDT", "BRETTUSDT", "ALPACAUSDT", "POPCATUSDT", "SUNUSDT", "VIDTUSDT", "NULSUSDT", "DOGSUSDT", "MBOXUSDT", "CHESSUSDT", 
        "FLUXUSDT", "BSWUSDT", "QUICKUSDT", "NEIROETHUSDT", "RPLUSDT", "AERGOUSDT", "POLUSDT", "UXLINKUSDT", "1MBABYDOGEUSDT", 
        "NEIROUSDT", "KDAUSDT", "FIDAUSDT", "FIOUSDT", "CATIUSDT", "GHSTUSDT", "LOKAUSDT", "HMSTRUSDT", "BTCUSDT_250328", 
        "ETHUSDT_250328", "REIUSDT", "COSUSDT", "EIGENUSDT", "DIAUSDT", "1000CATUSDT", "SCRUSDT", "GOATUSDT", "MOODENGUSDT", 
        "SAFEUSDT", "SANTOSUSDT", "TROYUSDT", "PONKEUSDT", "COWUSDT", "CETUSUSDT", "1000000MOGUSDT", "GRASSUSDT", "DRIFTUSDT", 
        "SWELLUSDT", "ACTUSDT", "PNUTUSDT", "HIPPOUSDT", "1000XUSDT", "DEGENUSDT", "BANUSDT", "AKTUSDT", "SLERFUSDT", "SCRTUSDT", 
        "1000CHEEMSUSDT", "1000WHYUSDT", "THEUSDT", "MORPHOUSDT", "CHILLGUYUSDT", "KAIAUSDT", "AEROUSDT", "ACXUSDT", "ORCAUSDT", 
        "MOVEUSDT", "RAYSOLUSDT", "KOMAUSDT", "VIRTUALUSDT", "SPXUSDT", "MEUSDT", "AVAUSDT", "DEGOUSDT", "VELODROMEUSDT", 
        "MOCAUSDT", "VANAUSDT", "PENGUUSDT", "LUMIAUSDT", "USUALUSDT", "AIXBTUSDT", "FARTCOINUSDT", "KMNOUSDT", "CGPTUSDT", 
        "HIVEUSDT", "DEXEUSDT"
    };
}
int main() {
    ios_base::sync_with_stdio(0);
    cin.tie(0); 
    cout.tie(0);
    initialization(); 

    if (api_key.empty() || api_secret.empty()) {
        handleTelegramBotPrint("API keys are missing from environment variables.");
        return 1;
    }

    // Define the URLs for Binance API
    string base_url_Account = "https://www.binance.com/futures/data/topLongShortAccountRatio";
    string base_url_Position = "https://www.binance.com/futures/data/topLongShortPositionRatio";

    // Sample list of trading symbols (should be fetched from Binance API but it's hardcoded here for simplicity)
    vector<string> coins = getHardcodedCoins();

    vector<pair<float, string>> list_longrate;
    vector<pair<float, string>> list_longshortrate;

    for (const auto& coin : coins) {
        if (coin.substr(coin.length() - 4) != "USDT") {
            continue;
        }

        string url_Acount = base_url_Account + "?symbol=" + coin + "&period=15m&limit=1";
        string url_Position = base_url_Position + "?symbol=" + coin + "&period=15m&limit=1";

        Json::Value data1 = getRequestData(url_Acount);
        Json::Value data2 = getRequestData(url_Position);

        if (!data1.empty() && !data2.empty()) {
            float longratebyAccount = stof(data1[0]["longAccount"].asString()) * 100.0;
            float longratebyPosition = stof(data2[0]["longAccount"].asString()) * 100.0;

            if (longratebyPosition * 2 - longratebyAccount > 75) {
                cout << (coin + " " + to_string(longratebyAccount) + " " + to_string(longratebyPosition) + " High mystery index") << endl;
            }

            list_longrate.push_back({longratebyAccount, coin});
            list_longshortrate.push_back({longratebyPosition - longratebyAccount, coin});

            if (longratebyAccount < 40) {
                cout << (coin + " " + to_string(longratebyAccount) + " Low Longrate") << endl; 
            }

            if (coin == "BTCUSDT") {
                cout << (coin + " " + to_string(longratebyAccount) + " " + to_string(longratebyPosition)) << endl; 
            }
        } else {
            cout << ("Something went wrong for " + coin) << endl;
        }
    }

    // Sort lists
    sort(list_longrate.begin(), list_longrate.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    sort(list_longshortrate.begin(), list_longshortrate.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });

    handleTelegramBotPrint("Long Rate By Account:");
    for (int i = 0; i < 5; i++) {
        handleTelegramBotPrint(list_longrate[i].second + " " + to_string(list_longrate[i].first));
    }

    handleTelegramBotPrint("Long Rate By Position - Long Rate By Account:");
    for (int i = 0; i < 5; i++) {
        handleTelegramBotPrint(list_longshortrate[i].second + " " + to_string(list_longshortrate[i].first));
    }

    return 0;
}
