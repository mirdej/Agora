/*

  Arduino Library to simplify ESP-NOW
  © 2024 by Michael Egger AT anyma.ch
  MIT License
  */

#include "esp_idf_version.h"
#include <esp_now.h>
#include "FS.h"
#include "AgoraFTP.h"

#ifndef __Agora_INCLUDED__
#define __Agora_INCLUDED__

#define AGORA_TASK_IDLE_TIME_MS 6000
#define AGORA_TASK_AGGRESSIVE_SEARCH_TIME_MS 50
#define AGORA_TASK_AGGRESSIVE_SEARCH_PERIOD 7200

#define AGORA_MAX_TRIBE_NAME_CHARACTERS 32
#define AGORA_ANON_NAME "Anon"

void agoraTask(void *);
typedef void (*agora_cb_t)(const uint8_t *mac, const uint8_t *incomingData, int len);

void dummyCallback(const uint8_t *mac, const uint8_t *incomingData, int len) { ; }

#if ESP_IDF_VERSION_MAJOR < 5
void generalCallback(const uint8_t *mac, const uint8_t *incomingData, int len);
#else
void generalCallback(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len);
#endif

#ifdef esp_now_recv_info_t
#pragma message("C Preprocessor got here too!")

#endif

struct AgoraMessage
{
    const char *string;
    int size;
};

AgoraMessage AGORA_MESSAGE_LOST = {"HALLLOOO??", 61};
AgoraMessage AGORA_MESSAGE_INVITE = {"PLZDM_ME!!", 12};
AgoraMessage AGORA_MESSAGE_PRESENT = {"Hi, I'm ", 74};
AgoraMessage AGORA_MESSAGE_WELCOME = {"I'm your guru: ", 82};
AgoraMessage AGORA_MESSAGE_PING = {"Huh?", 4};
AgoraMessage AGORA_MESSAGE_PONG = {"Ha!", 3};

const uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; //{0x24, 0xEC ,0x4A, 0x82, 0xC0 ,0xE0};//

#include "Arduino.h"
#include "MacAddress.h"
#include "Tribe.h"

#include "WiFi.h"
#include <esp_now.h>
#include <esp_wifi.h>

esp_now_peer_info_t peerInfo;

struct TheAgora
{
public:
    TribeMember me;
    std::vector<Tribe> tribes;
    long timeout;
    long interval;
    bool ftp_enabled;

    void begin();
    void begin(const char *name);
    void begin(String name) { begin(name.c_str()); };
    void tell(const char *text);
    void tell(uint8_t *buf, int len);
    void tell(const char *name, uint8_t *buf, int len);
    void establish(const char *name);
    void establish(const char *name, agora_cb_t cb);
    void join(const char *name);
    void join(String name) { join(name.c_str()); };
    void join(const char *name, agora_cb_t cb);
    void share(File file)
    {
        if (!ftp_enabled)
            return;
        file_to_share = file;
        send_header();
    };
    void giveUpAfterSeconds(int seconds) { timeout = seconds * 1000; };
    void setPingInterval(long ms);
    int connected();
};

TheAgora Agora;

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//                                                                                            IMPLEMENTATION

void TheAgora::setPingInterval(long ms)
{
    interval = ms < 100 ? 100 : ms;
    for (std::size_t i = 0; i < tribes.size(); ++i)
    {
        tribes[i].pingInterval = interval;
    }
}

void TheAgora::establish(const char *name)
{
    TheAgora::establish(name, dummyCallback);
}

void TheAgora::establish(const char *name, agora_cb_t cb)
{
    Tribe newtribe(name, cb);
    newtribe.myself.status = GURU;
    newtribe.myself.macAddress.setLocal();
    newtribe.guru.macAddress.setLocal(); // TODO THIS DOES NOT FUNCTION !!!!
    newtribe.pingInterval = interval;
    tribes.push_back(newtribe);
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::join(const char *name)
{
    join(name, dummyCallback);
}

void TheAgora::join(const char *name, agora_cb_t cb)
{
    Tribe newtribe(name, cb);
    // newtribe.myself.mac = me.mac;
    newtribe.myself.status = LOST;

    strncpy(newtribe.myself.name, me.name, AGORA_MAX_TRIBE_NAME_CHARACTERS);
    newtribe.myself.macAddress.setLocal();
    newtribe.pingInterval = interval;
    tribes.push_back(newtribe);
}
//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *text)
{

    int len = strlen(text);
    if (len > 249)
    {
        log_v("Message truncated (was %d bytes, now 249)", len);
        len = 249;
    }
    uint8_t buf[len];
    strncpy((char *)buf, text, len);
    tell(buf, len);
}

void TheAgora::tell(uint8_t *buf, int len)
{
    for (std::size_t i = 0; i < tribes.size(); ++i)
    {
        tribes[i].tell(buf, len);
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *name, uint8_t *buf, int len)
{
    for (std::size_t i = 0; i < tribes.size(); ++i)
    {
        if (strcmp(tribes[i].name, name) == 0)
        {
            tribes[i].tell(buf, len);
            return;
        }
    }
    log_e("Cannot tell: No tribe found named %s", name);
}

//-----------------------------------------------------------------------------------------------------------------------------
int TheAgora::connected()
{
    int connections = 0;

    for (std::size_t i = 0; i < tribes.size(); ++i)
    {
        if (tribes[i].myself.status != LOST)
        {
            connections++;
        }
    }
    return connections;
}
//-----------------------------------------------------------------------------------------------------------------------------

/* void TheAgora::fullPicture()
{
    Serial.println("------------------------- THE AGORA ------------------------- ");
    for (std::size_t i = 0; i < tribes.size(); ++i)
    {
        Serial.println();
        log_v("Tribe **** %s ****", tribes[i].name);
        log_v("Guru: %s", tribes[i].guru.MemberDesccription().c_str());
        log_v("Me: %s", tribes[i].myself.MemberDesccription().c_str());
        Serial.println();

        //   log_v("Membes %d:", tribes[i].mMESSAGE Lengthembers.size());
    }
    Serial.println("------------------------------------------------------ ");
    Serial.println();
} */

/* void log_message(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("MESSAGE Length: %d -  Body: ", len);
    for (int i = 0; i < len; i++)
    {
        Serial.write(incomingData[i]);
    }
    Serial.println();
} */

//-----------------------------------------------------------------------------------------------------------------------------
// callback function that will be executed when data is received
#if ESP_IDF_VERSION_MAJOR < 5
void generalCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    MAC_Address sender(macAddr);
    if (Agora.ftp_enabled)
    {
        if (handle_agora_ftp(macAddr, incomingData, len))
        {
            for (std::size_t i = 0; i < Agora.tribes.size(); ++i)
            {
                Agora.tribes[i].ftpUpdate();
            }
            return;
        }
    }
    // log_message(macAddr, incomingData, len);
    for (std::size_t i = 0; i < Agora.tribes.size(); ++i)
    {

        if (Agora.tribes[i].handleMessage(macAddr, incomingData, len))
        {
            //   log_v("Tribe %d %s: TRUE", i, tribes[i].name);
            return;
        }
        /*   else
          {
              log_v("Tribe %d %s: FALSE", i, tribes[i].name);
          } */
    }
    /*  Serial.println("\n\n");
     return; */
    Serial.println("MESSAGE NOT HANDLED:");

    log_v("Mess len %d: ", len);
    for (int i = 0; i < len; i++)
    {
        Serial.write(incomingData[i]);
    }
    Serial.print(" from: ");
    Serial.println(sender.toString());
}

#else

void generalCallback(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len)
{
    MAC_Address sender(esp_now_info->src_addr);
    // log_message(macAddr, incomingData, len);
    if (Agora.ftp_enabled)
    {
        if (handle_agora_ftp(esp_now_info->src_addr, incomingData, len))
        {
            for (std::size_t i = 0; i < Agora.tribes.size(); ++i)
            {
                Agora.tribes[i].ftpUpdate();
            }
            return;
        }
    }

    for (std::size_t i = 0; i < Agora.tribes.size(); ++i)
    {

        if (Agora.tribes[i].handleMessage(esp_now_info->src_addr, incomingData, len))
        {
            //   log_v("Tribe %d %s: TRUE", i, tribes[i].name);
            return;
        }
        /*   else
          {
              log_v("Tribe %d %s: FALSE", i, tribes[i].name);
          } */
    }
    /*  Serial.println("\n\n");
     return; */
    Serial.println("MESSAGE NOT HANDLED:");

    log_v("Mess len %d: ", len);
    for (int i = 0; i < len; i++)
    {
        Serial.write(incomingData[i]);
    }
    Serial.print(" from: ");
    Serial.println(sender.toString());
}

#endif

/*-----------------------------------------------------------------------------------------------------------------------------
    Start Agora Task:

    Set up WIFI in Station mode if not already enabled, initialize ESP-NOW and setup global Callback
    Then - if we're following a Guru, check periodically if he is still there.
    Otherwise, if we're a guru, check if we hear back from our followers from time to time or if we have lost them
*/

void TheAgora::begin()
{
    begin(AGORA_ANON_NAME);
}

void TheAgora::begin(const char *name)
{
    interval = AGORA_TASK_IDLE_TIME_MS;
    strncpy(me.name, name, AGORA_MAX_TRIBE_NAME_CHARACTERS);
    me.name[AGORA_MAX_TRIBE_NAME_CHARACTERS] = 0;

    xTaskCreate(
        agoraTask,    // Function that implements the task.
        "Agora Task", // Text name for the task.
        8192,         // Stack size in words, not bytes.
        NULL,         // Parameter passed into the task.
        1,            // Priority at which the task is created.
        NULL);
}

//-----------------------------------------------------------------------------------------------------------------------------
// Agora Task
void agoraTask(void *)
{

    // Start WiFI if not already done
    if (WiFi.getMode() == WIFI_MODE_NULL)
        WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        log_v("Error initializing ESP-NOW");
        return;
    }
    else
    {
        log_v("Initialized ESP-NOW");
    }

    Serial.print("Wi-Fi Channel: ");
    Serial.println(WiFi.channel());
    log_v("IP address: %s", WiFi.localIP().toString().c_str());
    log_v("MAC address: %s", WiFi.macAddress().c_str());
    log_v("WiFi channel: %d", WiFi.channel());

    me.macAddress.setLocal();

    log_v("Init Callbacks");
    esp_now_register_recv_cb(generalCallback);
    //  esp_now_register_send_cb(Agora::OnDataSent);

    // register peer for broadcasting
    peerInfo.channel = WiFi.channel();
    peerInfo.encrypt = false;
    memcpy(peerInfo.peer_addr, BROADCAST_ADDRESS, 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        log_v("Failed to add peer");
        return;
    }

    //----------------------------------------------------------------------------------------------------------- Loop

    while (1)
    {
        int active_tribes = 0;
        int searching_tribes = 0;
        int delay_time = Agora.interval > 0 ? Agora.interval : AGORA_TASK_IDLE_TIME_MS;
        /*         delay_time += (esp_random() >> 20);
         */
        for (std::size_t i = 0; i < Agora.tribes.size(); ++i)
        {
            active_tribes += Agora.tribes[i].update(Agora.timeout);
            if (Agora.tribes[i].myself.status == LOST)
            {
                if (millis() < AGORA_TASK_AGGRESSIVE_SEARCH_PERIOD)
                {
                    delay_time = AGORA_TASK_AGGRESSIVE_SEARCH_TIME_MS;
                }
            }
        }

        if (!active_tribes)
        {
            if (Agora.timeout && millis() > Agora.timeout)
            {
                log_v("\n\nThere is nothing going on in the Agora. Bored to death.\nGiving up, bye.\n\n");
                vTaskDelete(NULL);
            }
        }
        // fullPicture();
        //   log_v("delay_time %d", delay_time);
        vTaskDelay(pdMS_TO_TICKS(delay_time));
    }
}

#endif
