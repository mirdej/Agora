/*

Arduino Library to simplify ESP-NOW
© 2024 by Michael Egger AT anyma.ch
MIT License


*/

#ifndef __AGORA_TRIBE_INCLUDED__
#define __AGORA_TRIBE_INCLUDED__
#include "Arduino.h"
#include "Agora.h"
#include "MacAddress.h"
#include <iostream>
#include <vector>
#include "WiFi.h"
#include <esp_now.h>
#include <esp_wifi.h>

esp_err_t error;
//-----------------------------------------------------------------------------------------------------------------------------

bool isMessage(const uint8_t *input, int len, AgoraMessage message)
{
    if (len != message.size)
    {
        return false;
    }

    /* Serial.println(message.string); */

    char buf[strlen(message.string) + 1];
    memcpy(buf, input, strlen(message.string));
    buf[strlen(message.string)] = 0;
    return (strcmp(buf, message.string) == 0);
}
//-----------------------------------------------------------------------------------------------------------------------------

esp_err_t sendMessage(MAC_Address to, AgoraMessage message, char *name)
{
    char buf[message.size];
    memset(buf, '.', message.size);
    buf[message.size - 1] = 0;

    sprintf(buf, "%s%s\0", message.string, name);
    // log_v("Sending Message: %s", buf);
    return esp_now_send(to.mac, (uint8_t *)buf, message.size);
}

esp_err_t sendMessage(MAC_Address to, AgoraMessage message)
{
    return sendMessage(to, message, (char *)"");
}

typedef enum
{
    ALIEN,
    LOST,
    FOLLOWER,
    GURU,
    MEDIATOR
} MemberStatus;

String MemberStatusString(MemberStatus s)
{
    switch (s)
    {
    case ALIEN:
        return "Alien";
    case LOST:
        return "Lost";
    case FOLLOWER:
        return "Follower";
    case GURU:
        return "Guru";
    case MEDIATOR:
        return "MEDIATOR";
    }
    return "Unknown";
}
//-----------------------------------------------------------------------------------------------------------------------------

struct TribeMember
{
    char name[AGORA_MAX_TRIBE_NAME_CHARACTERS + 1];
    MAC_Address macAddress;
    int address;
    MemberStatus status;
    long time_of_last_sent_message;
    long time_of_last_received_message;

    TribeMember()
    {
        strcpy(name, AGORA_ANON_NAME);
    }

    String MemberDesccription(void)
    {
        String s;
        s = "  Name: " + String(name);
        s = "  Status: " + MemberStatusString(status);
        s += " MAC: " + macAddress.toString();
        s += " ADDRESS: " + String(address);
        return s;
    }

    bool operator==(TribeMember other) const
    {
        return other.macAddress == macAddress;
    }
};

extern TribeMember me;
//-----------------------------------------------------------------------------------------------------------------------------

struct Tribe
{
public:
    Tribe(const char *tribename, esp_now_recv_cb_t cb);
    char name[AGORA_MAX_TRIBE_NAME_CHARACTERS + 1]; // space for terminating NULL if LEN = MAXLEN

    void begin();
    void end();
    void update();
    void tell(uint8_t *buf, int len);
    bool handleMessage(const uint8_t *macAddr, const uint8_t *incomingData, int len);
    bool handleMessageAsGuru(const uint8_t *macAddr, const uint8_t *incomingData, int len);
    bool handleMessageAsMember(const uint8_t *macAddr, const uint8_t *incomingData, int len);

    void addMember(char *name, MAC_Address mac);
    bool hasMember(MAC_Address mac);

    esp_now_recv_cb_t callback;

    TribeMember guru;
    TribeMember myself;

private:
    std::vector<TribeMember> members;
    esp_now_peer_info_t peerInfo;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------
void Tribe::tell(uint8_t *buf, int len)
{
    if (myself.status == GURU)
    {
        for (std::size_t i = 0; i < members.size(); ++i)
        {
            if (members[i].status == FOLLOWER)
            {
                if (esp_now_send(members[i].macAddress.mac, buf, len) != ESP_OK)
                {
                    log_e("Error sending message to member %s", members[i].name);
                }
            }
        }
    }
    else if (myself.status == FOLLOWER)
    {
        if (esp_now_send(guru.macAddress.mac, buf, len) != ESP_OK)
        {
            log_e("Error sending message to guru");
        }
    }
    else
    {
        log_e("I'm lost, or what? Can't tell right now.");
    }
}
//-------------------------------------------------------------------------------------------------------------------------------------------------
bool Tribe::hasMember(MAC_Address mac)
{

    for (std::size_t i = 0; i < members.size(); ++i)
    {
        if (mac == members[i].macAddress)
        {
            log_v("%s is already a Member: %s", members[i].macAddress.toString().c_str(), members[i].name);
            return true;
        }
    }
    return false;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------
Tribe::Tribe(const char *tribename, esp_now_recv_cb_t cb)
{
    int len = strlen(tribename);
    log_v("Register tribe: %s, len %d", tribename, len);
    if (len > AGORA_MAX_TRIBE_NAME_CHARACTERS)
    {
        len = AGORA_MAX_TRIBE_NAME_CHARACTERS;
        log_v("The name is longer than %d characters, it has been truncated !!", AGORA_MAX_TRIBE_NAME_CHARACTERS);
    }
    memcpy(name, tribename, len);
    name[len] = 0; // terminating null
    callback = cb;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void Tribe::addMember(char *name, MAC_Address mac)
{
    TribeMember newMember;
    strncpy(newMember.name, name, AGORA_MAX_TRIBE_NAME_CHARACTERS);
    newMember.name[AGORA_MAX_TRIBE_NAME_CHARACTERS] = 0;
    newMember.macAddress = mac;

    members.push_back(newMember);
    log_v("Added member %s. Membercount: %d", members[members.size() - 1].name, members.size());
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
void Tribe::update()
{
    switch (myself.status)
    {
    case GURU:
        // Serial.println("Looking for followers");
        for (std::size_t i = 0; i < members.size(); ++i)
        {
            if (millis() - members[i].time_of_last_sent_message > AGORA_PING_INTERVAL)
            {
                sendMessage(members[i].macAddress, AGORA_MESSAGE_PING);
                members[i].time_of_last_sent_message = millis();
            }

            if (millis() - members[i].time_of_last_received_message > 4 * AGORA_PING_INTERVAL)
            {
                log_e("Lost a member");
                members[i].status = LOST;
            }
        }
        break;

    case LOST:
        sendMessage(MAC_Address(BROADCAST_ADDRESS), AGORA_MESSAGE_LOST, name);
        break;
    case FOLLOWER:
        if (millis() - myself.time_of_last_received_message > 4 * AGORA_PING_INTERVAL)
        {
            log_e("Lost my GURU !!");
            myself.status = LOST;
        }
        break;
    default:
        break;
    }
}

void log_message(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    Serial.printf("MESSAGE Length: %d -  Body: ", len);
    for (int i = 0; i < len; i++)
    {
        Serial.write(incomingData[i]);
    }
    Serial.println();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
bool Tribe::handleMessage(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    if (myself.status == GURU)
    {
        return handleMessageAsGuru(macAddr, incomingData, len);
    }
    else
    {
        return handleMessageAsMember(macAddr, incomingData, len);
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
bool Tribe::handleMessageAsGuru(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{

    MAC_Address sender(macAddr);

    if (isMessage(incomingData, len, AGORA_MESSAGE_INVITE))
    {
        // Gurus don't get invited
        return false;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_WELCOME))
    {
        // Gurus don't get welcomed
        return false;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PING))
    {
        // Gurus don't get pinged
        return false;
    }

    bool message_is_from_a_member;

    for (std::size_t i = 0; i < members.size(); ++i)
    {
        if (sender == members[i].macAddress)
        {
            message_is_from_a_member = true;
            members[i].time_of_last_received_message = millis();
            members[i].status = FOLLOWER;
        }
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_LOST))
    {
        char tribename[AGORA_MAX_TRIBE_NAME_CHARACTERS + 1];
        memcpy(tribename, incomingData + strlen(AGORA_MESSAGE_LOST.string), AGORA_MAX_TRIBE_NAME_CHARACTERS);
        tribename[AGORA_MAX_TRIBE_NAME_CHARACTERS] = 0;
        log_v("%s wants to join tribe *%s*, len %d", sender.toString().c_str(), tribename, strlen(tribename));

        if (strcmp(name, tribename) == 0)
        {
            Serial.println("Match");
            log_v("Send INVITE to %s", sender.toArrayString().c_str());

            esp_err_t error;

            if (!esp_now_is_peer_exist(sender.mac))
            {
                Serial.println("register peer");
                memset(&peerInfo, 0, sizeof(peerInfo));
                peerInfo.channel = WiFi.channel();
                peerInfo.encrypt = false;
                memcpy(peerInfo.peer_addr, sender.mac, 6);
                error = esp_now_add_peer(&peerInfo);
                if (error != ESP_OK)
                {
                    log_v("Failed to add peer: %s", esp_err_to_name(error));
                    return false;
                }
            }
            // error = esp_now_send(sender.mac, (uint8_t *)AGORA_MESSAGE_INVITE.string, AGORA_MESSAGE_INVITE.size);
            if (sendMessage(sender, AGORA_MESSAGE_INVITE) != ESP_OK)
            {
                log_v("Failed to send to peer: %s", esp_err_to_name(error));
                return false;
            }
            return true;
        }
    }

    else if (isMessage(incomingData, len, AGORA_MESSAGE_PRESENT))
    {
        char membername[AGORA_MAX_TRIBE_NAME_CHARACTERS + 1];
        memcpy(membername, incomingData + strlen(AGORA_MESSAGE_PRESENT.string), AGORA_MAX_TRIBE_NAME_CHARACTERS);
        membername[AGORA_MAX_TRIBE_NAME_CHARACTERS] = 0;

        if (sendMessage(sender, AGORA_MESSAGE_WELCOME, myself.name) != ESP_OK)
        {
            log_v("Failed to send to peer: %s", esp_err_to_name(error));
            return false;
        }

        if (!hasMember(sender))
        {
            addMember(membername, sender);
        }
        log_v("Welcome: %s", membername);
        return true;
    }
    else if (isMessage(incomingData, len, AGORA_MESSAGE_PONG))
    {
        // it's ok, we've already logged the timestamp. nothing more to do here
        return true;
    }

    if (message_is_from_a_member)
    {
        //  log_v("Message from a member. Call Callback");
        callback(macAddr, incomingData, len);
        return true;
    }
    else
    {
        log_v("Not a message from a member. Dismissed");
        return false;
    }
    // check if message is from a known member, if so, call this tribe's callback function
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
bool Tribe::handleMessageAsMember(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{

    /*         log_v("Tribe %s, Status %d", name, myself.status);
            log_message(macAddr, incomingData, len); */

    MAC_Address sender(macAddr);
    if (isMessage(incomingData, len, AGORA_MESSAGE_LOST))
    {
        // I'm not a guru for this tribe. don't bother with lost messages
        return false;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PONG))
    {
        // I'm not a guru for this tribe. don't bother with pong messages
        return false;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PRESENT))
    { // I'm not a guru for this tribe. don't bother with present messages
        return false;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_INVITE))
    {
        log_v("%s wants PIX !! ;-o ", sender.toString().c_str());

        if (!esp_now_is_peer_exist(sender.mac))
        {
            Serial.println("register peer");
            memset(&peerInfo, 0, sizeof(peerInfo));
            peerInfo.channel = WiFi.channel();
            peerInfo.encrypt = false;
            memcpy(peerInfo.peer_addr, sender.mac, 6);
            error = esp_now_add_peer(&peerInfo);
            if (error != ESP_OK)
            {
                log_e("Failed to add peer: %s", esp_err_to_name(error));
                return false;
            }
        }

        if (sendMessage(sender, AGORA_MESSAGE_PRESENT, myself.name) != ESP_OK)
        {
            log_e("Failed to send to peer: %s", esp_err_to_name(error));
            return false;
        }
        return true;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_WELCOME))
    {
        log_v("%s welcomes me", sender.toString().c_str());

        myself.status = FOLLOWER;
        guru.macAddress = sender;
        myself.time_of_last_received_message = millis();
        return true;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_PING))
    {
        myself.time_of_last_received_message = millis();
        //  log_v("Got ping'ed. %s", name);
        if (sendMessage(sender, AGORA_MESSAGE_PONG) != ESP_OK)
        {
            log_e("Failed to send to peer: %s", esp_err_to_name(error));
            return false;
        }
        return true;
    }

    if (sender == guru.macAddress)
    {
        myself.time_of_last_received_message = millis();

        callback(macAddr, incomingData, len);
        //      log_v("Message for me from GURU %s == %s", sender.toString().c_str(),guru.macAddress.toString().c_str());
        return true;
    }
    log_v("Not a message from my GURU. Dismissed");
    return false;
}

#endif