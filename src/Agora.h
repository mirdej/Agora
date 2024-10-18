/*

  Arduino Library to simplify ESP-NOW
  © 2024 by Michael Egger AT anyma.ch
  MIT License
  */

#ifndef __Agora_INCLUDED__
#define __Agora_INCLUDED__

#define AGORA_TASK_IDLE_TIME_MS 6000
#define AGORA_MAX_TRIBE_NAME_CHARACTERS 32
#define AGORA_ANON_NAME "Anon"
#define AGORA_PING_INTERVAL AGORA_TASK_IDLE_TIME_MS

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

namespace Agora
{
    TribeMember me;
    std::vector<Tribe> tribes;
    esp_now_peer_info_t peerInfo;

    void begin();
    void begin(const char *name);
    void tell(const char *text);
    void tell(uint8_t *buf, int len);
    void tell(const char *name, uint8_t *buf, int len);
    void establish(const char *name, esp_now_recv_cb_t cb);
    void join(const char *name, esp_now_recv_cb_t cb);
    void generalCallback(const uint8_t *mac, const uint8_t *incomingData, int len);

    //-----------------------------------------------------------------------------------------------------------------------------
    //-----------------------------------------------------------------------------------------------------------------------------
    //                                                                                            IMPLEMENTATION

    void establish(const char *name, esp_now_recv_cb_t cb)
    {
        Tribe newtribe(name, cb);
        newtribe.myself.status = GURU;
        newtribe.myself.macAddress.setLocal();
        newtribe.guru.macAddress.setLocal(); // TODO THIS DOES NOT FUNCTION !!!!
        tribes.push_back(newtribe);
    }
    //-----------------------------------------------------------------------------------------------------------------------------

    void join(const char *name, esp_now_recv_cb_t cb)
    {
        Tribe newtribe(name, cb);
        // newtribe.myself.mac = me.mac;
        newtribe.myself.status = LOST;

        strncpy(newtribe.myself.name, me.name, AGORA_MAX_TRIBE_NAME_CHARACTERS);
        newtribe.myself.macAddress.setLocal();

        tribes.push_back(newtribe);
    }
    //-----------------------------------------------------------------------------------------------------------------------------

    void tell(const char *text)
    {
        if (tribes.size())
        {
            int len = strlen(text);
            if (len > 249)
            {
                len = 249;
            }
            uint8_t buf[len];
            strncpy((char *)buf, text, len);
            {
                tribes[0].tell(buf, len);
            }
        }
    }

    void tell(uint8_t *buf, int len)
    {
        if (tribes.size())
        {
            tribes[0].tell(buf, len);
        }
    }

    //-----------------------------------------------------------------------------------------------------------------------------

    void tell(const char *name, uint8_t *buf, int len)
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

    void fullPicture()
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
    }

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
    void generalCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
    {
        // log_message(macAddr, incomingData, len);
        for (std::size_t i = 0; i < tribes.size(); ++i)
        {

            if (tribes[i].handleMessage(macAddr, incomingData, len))
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
        MAC_Address sender(macAddr);

        log_v("Mess len %d: ", len);
        for (int i = 0; i < len; i++)
        {
            Serial.write(incomingData[i]);
        }
        Serial.print(" from: ");
        Serial.println(sender.toString());
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
        log_v("IP address: %s", WiFi.localIP().toString());
        log_v("MAC address: %s", WiFi.macAddress().c_str());
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

            for (std::size_t i = 0; i < tribes.size(); ++i)
            {
                tribes[i].update();
            }

            // fullPicture();
            int delay_time = AGORA_TASK_IDLE_TIME_MS;
            delay_time += (esp_random() >> 20);
            //  log_v("delay_time %d", delay_time);
            vTaskDelay(pdMS_TO_TICKS(delay_time));
        }
    }

    /*-----------------------------------------------------------------------------------------------------------------------------
        Start Agora Task:

        Set up WIFI in Station mode if not already enabled, initialize ESP-NOW and setup global Callback
        Then - if we're following a Guru, check periodically if he is still there.
        Otherwise, if we're a guru, check if we hear back from our followers from time to time or if we have lost them
    */

    void begin()
    {
        begin(AGORA_ANON_NAME);
    }

    void begin(const char *name)
    {

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
}
#endif
