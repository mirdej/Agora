
#ifndef __Agora_INCLUDED__
#define __Agora_INCLUDED__
/*

  Arduino Library to simplify ESP-NOW
  © 2024 by Michael Egger AT anyma.ch
  MIT License
  */

#include "Arduino.h"
#include "WiFi.h"
#include <esp_now.h>
#include <esp_wifi.h>
#include "esp_idf_version.h"
#include <esp_now.h>
#include "FS.h"
#include "SPIFFS.h"
#include "Preferences.h"
#include "SD.h"

#define AGORA_DEFAULT_PING_INTERVAL 2000
#define AGORA_MAX_NAME_CHARACTERS 32
#define AGORA_MAX_FRIENDS 20
#define AGORA_MAX_TRIBES 20
#define AGORA_ANON_NAME "Anon"
#define ESP_NOW_MAX_CHANNEL 13

#define AGORA_WIFI_PROV_SSID_OFFSET 20
#define AGORA_WIFI_PROV_PASS_OFFSET 62

typedef enum
{
    FTPUNKOWN,
    RUNNING,
    ERROR,
    DONE,
} ftpStatus_t;

void agoraTask(void *);

typedef void (*agora_cb_t)(const uint8_t *mac, const uint8_t *incomingData, int len);
typedef void (*agora_share_cb_t)(const uint8_t *mac, ftpStatus_t status, const char *filename, size_t filesize, size_t bytesRemaining);

#if ESP_IDF_VERSION_MAJOR < 5
void generalCallback(const uint8_t *mac, const uint8_t *incomingData, int len);
#else
void generalCallback(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len);
#endif
void AGORA_LOG_STATUS(long interval = 5000);

extern FS Fileshare_Filesystem;

#define ESPNOW_FILESHARE_CHUNK_SIZE 240
#define ESPNOW_FILESHARE_TIMEOUT 1000

typedef struct
{
    char magicword[11];
    int filesize;
    char filename[48];
} agoraFileshareHeader_t;

typedef struct
{
    ftpStatus_t ftpStatus = FTPUNKOWN;
    uint8_t receiverMac[6];
    size_t bytesRemaining;
    long startTime;
    long lastMessage;
    int nextReceiver;
    File file;
    // FS Filesystem;
} agoraFileSender_t;

typedef struct
{
    ftpStatus_t ftpStatus = FTPUNKOWN;
    uint8_t senderMac[6];
    File file;
    long startTime;
    size_t bytesRemaining;
    long lastMessage;
    // FS Filesystem;
} agoraFileReceiver_t;

typedef enum
{
    UNKNOWN,
    ASPIRING_FOLLOWER,
    IN_CONTACT,
    FOLLOWER,
    GURU,
    ABSENT_GURU,
} relationship;

struct AgoraTribe
{
    char name[AGORA_MAX_NAME_CHARACTERS + 1];
    relationship meToThem;
    bool autoPair;
    long pairUntil;
    int channel;
    agora_cb_t callback;
};

struct AgoraFriend
{
    char name[AGORA_MAX_NAME_CHARACTERS + 1];
    char tribe[AGORA_MAX_NAME_CHARACTERS + 1];
    relationship meToThem;
    long lastMessageSent;
    long lastMessageReceived;
    int channel;
    uint8_t mac[6];
};

struct AgoraMessage
{
    const char *string;
    int size;
};

struct TheAgora
{
public:
    long timeout;
    long pingInterval;
    int address;
    char name[AGORA_MAX_NAME_CHARACTERS + 1];
    AgoraFriend friends[AGORA_MAX_FRIENDS];
    AgoraTribe tribes[AGORA_MAX_TRIBES];
    int friendCount;
    int tribeCount;
    int activeConnectionCount;
    int wifiChannel;
    int connectedPercent;
    bool eavesdrop;
    char version[64];
    char includedBy[128];
    bool ftpEnabled;
    agora_share_cb_t ftpCallback;

    void begin(bool addressInName = false);
    void begin(const char *newname, bool addressInName = false, const char *caller = __BASE_FILE__);
    void begin(String name) { begin(name.c_str()); };
    void end();
    void tell(const char *text);
    void tell(const char *name, const char *text);
    void tell(uint8_t *buf, int len);
    void tell(char *buf, int len) { tell((uint8_t *)buf, len); }
    void tell(const char *name, uint8_t *buf, int len);
    void tell(const char *name, char *buf, int len) { tell(name, (uint8_t *)buf, len); }
    /*     void establish(const char *name, bool autoPair = false);
     */
    void establish(const char *name, agora_cb_t cb = NULL, bool autoPair = true);
    /*    void join(const char *name, bool autoPair = false);
       void join(String name, bool autoPair = false) { join(name.c_str(), autoPair); }; */
    void join(const char *name, agora_cb_t cb = NULL, bool autoPair = true);
    void conspire(int forSeconds = 20, const char *cult = NULL);
    char *getVersion();
    bool addFriend(AgoraFriend *newFriend);

    void rememberFriends();
    void forgetFriends();
    void showID();
    void enableFileSharing(fs::FS &fs = SPIFFS, agora_share_cb_t cb = NULL)
    {
        ftpCallback = cb;
        ftpEnabled = true;
        Fileshare_Filesystem = fs;
    }
    void share(const char *path);
    void share(const char *name, const char *path);
    void giveUpAfterSeconds(int seconds) { timeout = seconds * 1000; };
    void setPingInterval(long ms);
    int connected() { return activeConnectionCount; }

    void openTheGate(const char *ssid, const char *pass);

private:
};

extern TheAgora Agora;

esp_err_t sendMessage(AgoraFriend *to, AgoraMessage message, char *name = (char *)"");
esp_err_t sendMessage(const uint8_t *macAddr, AgoraMessage message, char *name = (char *)"");
esp_err_t sendMessage(uint8_t *macAddr, AgoraMessage message, char *name = (char *)"");
int handleAgoraMessageAsGuru(const uint8_t *macAddr, const uint8_t *incomingData, int len);
int handleAgoraMessageAsMember(const uint8_t *macAddr, const uint8_t *incomingData, int len);
bool handle_agora_ftp(const uint8_t *macAddr, const uint8_t *incomingData, int len);
bool isMessage(const uint8_t *input, int len, AgoraMessage message);
bool macMatch(uint8_t *a, uint8_t *b);
bool macMatch(AgoraFriend a, AgoraFriend b);
void resetFileShareInfo();

// bool EspNowAddPeer(uint8_t *peer_addr);
bool EspNowAddPeer(const uint8_t *peer_addr);
// bool EspNowAddPeer(const uint8_t peer_addr);

#define AGORA_LOG_LEVEL_NONE 0
#define AGORA_LOG_LEVEL_ERROR 1
#define AGORA_LOG_LEVEL_INFO 2
#define AGORA_LOG_LEVEL_VERBOSE 3

#ifndef AGORA_LOG_LEVEL
#define AGORA_LOG_LEVEL AGORA_LOG_LEVEL_VERBOSE
#endif

#if (AGORA_LOG_LEVEL == AGORA_LOG_LEVEL_VERBOSE)
// Preprocessor Macro: https://en.cppreference.com/w/c/preprocessor/replace
#define AGORA_LOG_V(msg, ...)                \
    {                                        \
        Serial.printf((msg), ##__VA_ARGS__); \
        Serial.println();                    \
    }
#else
#define AGORA_LOG_V (msg, ...)
#endif

#define AGORA_LOG_E(msg, ...)                \
    {                                        \
        Serial.print("  - AGORA ERROR: ");   \
        Serial.printf((msg), ##__VA_ARGS__); \
        Serial.println();                    \
    }

void AGORA_LOG_MAC(uint8_t *mac);
void AGORA_LOG_MAC(const uint8_t *mac);
void AGORA_LOG_RELATIONSHIP(relationship r);
void AGORA_LOG_FRIEND(AgoraFriend f);

void agora_ftp_send_header();
void agora_ftp_send_chunk();
#endif
