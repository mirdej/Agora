/*TODOS

Tribe list does not get real updates (wifi channels ??)
weird channels on follower side
--> send channel with invite!!
__BASE_FILE__ üath is no absolute


*/

#include "Agora.h"
#include "Preferences.h"
#include <esp_wifi.h>
#include "SPIFFS.h"

TheAgora Agora;
Preferences AgoraPreferences;

esp_now_peer_info_t tempPeer;

void dummyCallback(const uint8_t *mac, const uint8_t *incomingData, int len) { ; }

AgoraMessage AGORA_MESSAGE_LOST = {"HALLLOOO??", 61};
AgoraMessage AGORA_MESSAGE_INVITE = {"PLZDM_ME!!", 52};
AgoraMessage AGORA_MESSAGE_PRESENT = {"Hi, I'm ", 74};
AgoraMessage AGORA_MESSAGE_WELCOME = {"I'm your guru: ", 82};
AgoraMessage AGORA_MESSAGE_PING = {"Huh?", 4};
AgoraMessage AGORA_MESSAGE_PONG = {"Ha!", 3};
AgoraMessage AGORA_MESSAGE_POLICE = {"POLICE!Your ID Please!", 25};

uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const char *r_strings[] = {
    "UNKNOWN   ",
    "ASPIRING  ",
    "CONTACTING",
    "FOLLOWER  ",
    "GURU      ",
    "ABSENT    "};

esp_fileshare_header_t esp_fileshare_header;
size_t bytes_to_send;
size_t percent_done;
esp_now_peer_info_t slave;
File file_to_share;

File file_to_write;
size_t total_bytes_to_receive;
size_t bytes_to_receive;
long file_transfer_start;
FS Fileshare_Filesystem = SPIFFS;

//-----------------------------------------------------------------------------------------------------------------------------
//                                                                                            LOGGING
//-----------------------------------------------------------------------------------------------------------------------------

void AGORA_LOG_MAC(uint8_t *mac)
{
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5]);
}

void AGORA_LOG_MAC(const uint8_t *mac)
{
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x",
                  mac[0], mac[1], mac[2],
                  mac[3], mac[4], mac[5]);
}

void AGORA_LOG_RELATIONSHIP(relationship r)
{
    Serial.print(r_strings[r]);
}
void AGORA_LOG_FRIEND(AgoraFriend f)
{
    Serial.printf("%s of tribe %s", f.name, f.tribe);
    Serial.printf(" listening on channel %d to me as their ", f.channel);
    AGORA_LOG_RELATIONSHIP(f.meToThem);
    Serial.printf(" MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  f.mac[0], f.mac[1], f.mac[2],
                  f.mac[3], f.mac[4], f.mac[5]);
}

void AGORA_LOG_FRIEND(AgoraFriend *f)
{
    AgoraFriend ff;
    memcpy(&ff, f, sizeof(AgoraFriend));
    AGORA_LOG_FRIEND(ff);
}

void AGORA_LOG_FRIEND_TABLE()
{
    Serial.println("Friends");
    Serial.println("MAC ADDRESS       | Type       | Chan | Name                           | Tribe                          | Last seen | Last messaged");
    Serial.println("---------------------------------------------------------------------------------------------------------------------------------------");
    for (int i = 0; i < Agora.friendCount; i++)
    {
        AgoraFriend f = Agora.friends[i];
        AGORA_LOG_MAC(f.mac);
        Serial.print(" | ");
        AGORA_LOG_RELATIONSHIP(f.meToThem);
        Serial.print(" | ");
        Serial.printf("%4u | %30s | %30s | ", f.channel, f.name, f.tribe);
        if (f.lastMessageReceived)
        {
            Serial.printf("%9u |", millis() - f.lastMessageReceived);
        }
        else
        {
            Serial.printf("        - |");
        }
        if (f.lastMessageSent)
        {
            Serial.printf("%9u \n", millis() - f.lastMessageSent);
        }
        else
        {
            Serial.printf("        - \n");
        }
    }
}

void AGORA_LOG_TRIBE_TABLE()
{
    Serial.println("Tribes");
    Serial.println(" Type       | Chan | Name                           | Last seen | Last messaged");
    Serial.println("-----------------------------------------------------------------------------------");
    for (int i = 0; i < Agora.tribeCount; i++)
    {
        AgoraTribe t = Agora.tribes[i];
        AGORA_LOG_RELATIONSHIP(t.meToThem);
        Serial.print(" | ");
        Serial.printf("%4u | %30s | ", t.channel, t.name);
        Serial.printf("%9u | %9u\n", millis() - t.lastMessageReceived, millis() - t.lastMessageSent);
    }
    Serial.println("-----------------------------------------------------------------------------------");
}

void AGORA_LOG_INCOMING_DATA(const uint8_t *d, int len)
{
    char buf[len + 1];
    sprintf(buf, (const char *)d, len);
    Serial.printf("INCOMING Message: %s", buf);
    Serial.println();
}

void AGORA_LOG_STATUS(long interval)
{
    if (!Agora.logStatus)
        return;
    static long last_log;
    if (millis() - last_log > interval)
    {
        last_log = millis();
        Serial.println("---------------------------------------------------------------------------------------------------------------------------------------");
        AGORA_LOG_TRIBE_TABLE();
        Serial.println();
        AGORA_LOG_FRIEND_TABLE();
        Serial.println("---------------------------------------------------------------------------------------------------------------------------------------\n\n\n");
    }
}

//-----------------------------------------------------------------------------------------------------------------------------
// This Task gets spawned if we are asked for ID by Agora-Police

uint8_t policeMac[6] = {0, 0, 0, 0, 0, 0};

void idTask(void *)
{
    randomSeed(analogRead(A0));
    delay(random(3000));
    char buf[240];

    int hasPoliceMac = 0;
    for (int i = 0; i < 6; i++)
    {
        hasPoliceMac += policeMac[i];
    }
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    snprintf(buf, 240, "Name:%s|Chip:%s|Flash:%u|FreeRam:%u|SDK:%s|IP:%s|Channel:%u|", Agora.name, ESP.getChipModel(), ESP.getFlashChipSize(), info.total_free_bytes, ESP.getSdkVersion(), WiFi.localIP().toString().c_str(), WiFi.channel());
    Serial.println(buf);
    if (hasPoliceMac)
        esp_now_send(policeMac, (uint8_t *)buf, 240);

    delay(100);

    snprintf(buf, 240, "Friends:%u|Tribes:%u|AgoraVersion:%s|File:%s|Date:%s|", Agora.friendCount, Agora.tribeCount, Agora.getVersion(), Agora.includedBy, __DATE__);
    Serial.println(buf);
    if (hasPoliceMac)
        esp_now_send(policeMac, (uint8_t *)buf, 240);

    delay(100);

    for (int i = 0; i < Agora.tribeCount; i++)
    {
        AgoraTribe t = Agora.tribes[i];
        snprintf(buf, 240, "Tribe:%s|%u|%s|%u|%u", t.name, t.channel, r_strings[t.meToThem], millis() - t.lastMessageReceived, millis() - t.lastMessageSent);
        Serial.println(buf);
        if (hasPoliceMac)
            esp_now_send(policeMac, (uint8_t *)buf, 240);
        delay(30);
    }

    for (int i = 0; i < Agora.friendCount; i++)
    {
        AgoraFriend f = Agora.friends[i];

        snprintf(buf, 240, "Friend:%02x:%02x:%02x:%02x:%02x:%02x|%u|%s|%s|%s|%u|%u",
                 f.mac[0], f.mac[1], f.mac[2], f.mac[3], f.mac[4], f.mac[5],
                 f.channel, f.name, f.tribe, r_strings[f.meToThem], millis() - f.lastMessageReceived, millis() - f.lastMessageSent);
        Serial.println(buf);
        if (hasPoliceMac)
            esp_now_send(policeMac, (uint8_t *)buf, 240);
        delay(30);
    }

    vTaskDelete(NULL);
}
void TheAgora::showID()
{
    AGORA_LOG_V("Ooops the cops!! Show my ID. ")
    xTaskCreate(idTask, "Police", 8192, NULL, 1, NULL);
}

char *TheAgora::getVersion()
{
    snprintf(version, 64, "%s %s", __DATE__, __TIME__);
    return version;
}
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//                                                                                            IMPLEMENTATION

//-----------------------------------------------------------------------------------------------------------------------------
void TheAgora::establish(const char *name)
{
    TheAgora::establish(name, dummyCallback);
}

void TheAgora::establish(const char *name, agora_cb_t cb)
{

    if (Agora.tribeCount >= AGORA_MAX_TRIBES)
    {
        AGORA_LOG_E("NO More tribes allowed");
        return;
    }

    AgoraTribe newTribe;
    memset(&newTribe, 0, sizeof(AgoraTribe));
    int len = strlen(name);
    AGORA_LOG_V("Register tribe: %s, len %d", name, len);
    if (len > AGORA_MAX_NAME_CHARACTERS)
    {
        len = AGORA_MAX_NAME_CHARACTERS;
        AGORA_LOG_V("The name is longer than %d characters, it has been truncated !!", AGORA_MAX_NAME_CHARACTERS);
    }
    memcpy(&newTribe.name, name, len);
    newTribe.meToThem = GURU;
    newTribe.channel = WiFi.channel();
    newTribe.callback = cb;
    memcpy(&Agora.tribes[Agora.tribeCount], &newTribe, sizeof(AgoraTribe));
    Agora.tribeCount++;
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::join(const char *name)
{
    join(name, dummyCallback);
}

void TheAgora::join(const char *name, agora_cb_t cb)
{

    if (Agora.tribeCount >= AGORA_MAX_TRIBES)
    {
        AGORA_LOG_E("NO More tribes allowed");
        return;
    }

    AgoraTribe newTribe;
    memset(&newTribe, 0, sizeof(AgoraTribe));
    int len = strlen(name);
    AGORA_LOG_V("Join tribe: %s, len %d", name, len);
    if (len > AGORA_MAX_NAME_CHARACTERS)
    {
        len = AGORA_MAX_NAME_CHARACTERS;
        AGORA_LOG_V("The name is longer than %d characters, it has been truncated !!", AGORA_MAX_NAME_CHARACTERS);
    }
    memcpy(&newTribe.name, name, len);
    newTribe.meToThem = ASPIRING_FOLLOWER;
    newTribe.callback = cb;
    memcpy(&Agora.tribes[Agora.tribeCount], &newTribe, sizeof(AgoraTribe));
    Agora.tribeCount++;
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *text)
{
    int len = strlen(text);
    if (len > 249)
    {
        AGORA_LOG_E("Message truncated (was %d bytes, now 249)", len);
        len = 249;
    }
    uint8_t buf[len];
    memset(buf, 0, sizeof(buf));
    strncpy((char *)buf, text, len);
    tell(buf, len);
}

void TheAgora::tell(const char *name, const char *text)
{
    int len = strlen(text);
    if (len > 249)
    {
        AGORA_LOG_E("Message truncated (was %d bytes, now 249)", len);
        len = 249;
    }
    uint8_t buf[len];
    memset(buf, 0, sizeof(buf));
    strncpy((char *)buf, text, len);
    tell(name, buf, len);
}

void TheAgora::tell(uint8_t *buf, int len)
{
    if (ftp_enabled && (bytes_to_receive || bytes_to_send))
    {
        AGORA_LOG_E("Sharing a File, Please wait until done !!!");
        return;
    }
    if (esp_now_send(NULL, buf, len) != ESP_OK)
    {
        AGORA_LOG_E("Error sending message to all members");
    }
    for (int i = 0; i < friendCount; i++)
    {
        friends[i].lastMessageSent = millis();
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *name, uint8_t *buf, int len)
{
    if (ftp_enabled && (bytes_to_receive || bytes_to_send))
    {
        AGORA_LOG_E("Sharing a File, Please wait until done !!!");
        return;
    }
    for (int i = 0; i < friendCount; i++)
    {
        if (!strcmp(friends[i].name, name) || !strcmp(friends[i].tribe, name))
        {
            if (esp_now_send(friends[i].mac, buf, len) != ESP_OK)
            {
                AGORA_LOG_E("Error sending message to all members");
            }
            friends[i].lastMessageSent = millis();
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------
int TheAgora::connected()
{
    return connectedPercent > 99;
}

//-----------------------------------------------------------------------------------------------------------------------------
void TheAgora::setPingInterval(long ms)
{
    pingInterval = (ms > 99) ? ms : 100;
}

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

void TheAgora::begin(const char *newname, const char *caller)
{

    strncpy(includedBy, caller, 127);
    strncpy(name, newname, AGORA_MAX_NAME_CHARACTERS);
    name[AGORA_MAX_NAME_CHARACTERS] = 0;
    pingInterval = AGORA_DEFAULT_PING_INTERVAL;

    xTaskCreate(
        agoraTask,    // Function that implements the task.
        "Agora Task", // Text name for the task.
        8192,         // Stack size in words, not bytes.
        NULL,         // Parameter passed into the task.
        1,            // Priority at which the task is created.
        NULL);
}

//----------------------------------------------------------------------------------------

bool EspNowAddPeer(const uint8_t *peer_addr)

/* {
    return EspNowAddPeer((uint8_t *)peer_addr);
};

bool EspNowAddPeer(uint8_t *peer_addr) */
{ // add pairing
    memset(&tempPeer, 0, sizeof(tempPeer));
    const esp_now_peer_info_t *peer = &tempPeer;
    memcpy(tempPeer.peer_addr, peer_addr, 6);

    tempPeer.channel = WiFi.channel(); // pick a channel
    tempPeer.encrypt = 0;              // no encryption
    // check if the peer exists
    AGORA_LOG_MAC(peer_addr);
    AGORA_LOG_V(": Add peer, channel %d ", tempPeer.channel);
    bool exists = esp_now_is_peer_exist(tempPeer.peer_addr);
    if (exists)
    {
        // Slave already paired.
        Serial.println("Already Paired");
        return true;
    }
    else
    {
        esp_err_t addStatus = esp_now_add_peer(peer);
        if (addStatus == ESP_OK)
        {
            // Pair success
            Serial.println("Add peer success");
            return true;
        }
        else
        {
            Serial.println("Add peer failed");
            return false;
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------

bool macMatch(uint8_t *a, uint8_t *b)
{
    /*     AGORA_LOG_MAC(a);
        Serial.print(" == ");
        AGORA_LOG_MAC(b);
        Serial.print(" ? ");
     */
    for (int i = 0; i < 6; i++)
    {
        if (a[i] != b[i])
        {
            //         Serial.println("NO");
            return false;
        }
    }
    // Serial.println("YES");
    return true;
}

bool macMatch(AgoraFriend *a, AgoraFriend *b)
{
    return macMatch(a->mac, b->mac);
}

//-----------------------------------------------------------------------------------------------------------------------------

AgoraTribe *tribeNamed(char *name)
{
    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (!strcmp(Agora.tribes[i].name, name))
        {
            return &Agora.tribes[i];
        }
    }
    return NULL;
}

AgoraTribe *tribeNamed(const char *name)
{
    return tribeNamed((char *)name);
}

//-----------------------------------------------------------------------------------------------------------------------------

AgoraFriend *friendForMac(uint8_t *mac)
{
    for (int i = 0; i < Agora.friendCount; i++)
    {
        if (macMatch(Agora.friends[i].mac, mac))
        {
            return &Agora.friends[i]; // + i * sizeof(AgoraFriend);
        }
    }
    return NULL;
}

AgoraFriend *friendForMac(const uint8_t *mac)
{
    return friendForMac((uint8_t *)mac);
}
//-----------------------------------------------------------------------------------------------------------------------------

bool TheAgora::addFriend(AgoraFriend *newFriend)
{
    if (friendCount >= AGORA_MAX_FRIENDS)
    {
        AGORA_LOG_E("Too many friends, cannot have a new friend.");
        return false;
    }

    // see if friend exists
    for (int i; i < friendCount; i++)
    {
        if (macMatch(&friends[i], newFriend))
        {
            AGORA_LOG_MAC(newFriend->mac);
            AGORA_LOG_MAC(friends[i].mac);
            AGORA_LOG_V(" Friend already exists: %d ", friends[i].name);
            return false;
        }
    }

    AGORA_LOG_V("Add friend:");
    memcpy(&friends[friendCount], newFriend, sizeof(AgoraFriend));
    AGORA_LOG_FRIEND(friends[friendCount]);
    friendCount++;
    EspNowAddPeer(newFriend->mac);
    return true;
}

void TheAgora::forgetFriends()
{
    Agora.friendCount = 0;
    AgoraPreferences.begin("Agora");
    AgoraPreferences.putInt("friendCount", Agora.friendCount);
    AgoraPreferences.end();
    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (Agora.tribes[i].meToThem != GURU)
        {
            Agora.tribes[i].meToThem = ASPIRING_FOLLOWER;
        }
    }
}
//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::rememberFriends()
{
    AgoraPreferences.begin("Agora");
    AgoraPreferences.putInt("friendCount", Agora.friendCount);
    AGORA_LOG_V("Storing %d friends.", Agora.friendCount);
    int storageBytes = 0;
    for (int i = 0; i < Agora.friendCount; i++)
    {
        char prefname[12];
        sprintf(prefname, "friend%02d", i);
        AgoraPreferences.putBytes(prefname, &Agora.friends[i], sizeof(AgoraFriend));
        storageBytes += sizeof(AgoraFriend);
    }
    AgoraPreferences.putInt("wifiChannel", WiFi.channel());
    AgoraPreferences.end();
    AGORA_LOG_V("Stored %d bytes (plus some).");
}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
// check if message is from a known member, if so, call this tribe's callback function
bool amIaGuruForTribe(const char *name)
{
    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (Agora.tribes[i].meToThem == GURU)
        {
            if (!strcmp(Agora.tribes[i].name, name))
            {
                return true;
            }
        }
    }
    return false;
}

int handleAgoraMessageAsGuru(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    esp_err_t error;

    if (isMessage(incomingData, len, AGORA_MESSAGE_INVITE))
    {
        return 0; // Gurus don't get invited
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_WELCOME))
    {
        return 0; // Gurus don't get welcomed
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PING))
    {
        return 0; // Gurus don't get pinged
    }

    bool message_is_from_a_friend;

    for (int i = 0; i < Agora.friendCount; i++)
    {
        if (macMatch((uint8_t *)macAddr, Agora.friends[i].mac))
        {
            message_is_from_a_friend = true;
            Agora.friends[i].lastMessageReceived = millis();
        }
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_LOST))
    {
        char tribename[AGORA_MAX_NAME_CHARACTERS + 1];
        memcpy(tribename, incomingData + strlen(AGORA_MESSAGE_LOST.string), AGORA_MAX_NAME_CHARACTERS);
        tribename[AGORA_MAX_NAME_CHARACTERS] = 0;
        AGORA_LOG_MAC(macAddr);
        AGORA_LOG_V(" wants to join tribe *%s*, len %d", tribename, strlen(tribename));

        if (amIaGuruForTribe(tribename))
        {
            AGORA_LOG_V("MATCH");
            if (!EspNowAddPeer(macAddr))
            {
                return 0;
            }
            AGORA_LOG_MAC(macAddr);
            AGORA_LOG_V(" Send an invite. ")

            // INVITE MESSAGE takes 11 chars, tribename max 30
            // Length of total message: 52, fill before-last-byte of message [50] with channel number
            char buf[AGORA_MESSAGE_INVITE.size];
            memset(buf, '.', sizeof(buf));
            buf[AGORA_MESSAGE_INVITE.size - 1] = 0;
            buf[AGORA_MESSAGE_INVITE.size - 2] = WiFi.channel();
            sprintf(buf, "%s%s\0", AGORA_MESSAGE_INVITE.string, tribename);
            if (Agora.logMessages)
            {
                ("Sending Message: %s", buf);
            }
            AgoraFriend *f = friendForMac(macAddr);
            if (f)
                f->lastMessageSent = millis();
            esp_now_send(macAddr, (uint8_t *)buf, AGORA_MESSAGE_INVITE.size);

            /*   if (sendMessage(macAddr, AGORA_MESSAGE_INVITE, tribename) != ESP_OK)
              {
                  AGORA_LOG_V("Failed to send to peer");
                  return 0;
              } */

            AgoraFriend *knownFriend = friendForMac(macAddr);
            if (knownFriend)
            {
                knownFriend->meToThem = IN_CONTACT;
                knownFriend->channel = WiFi.channel();
                knownFriend->lastMessageReceived = millis();

                AGORA_LOG_V("Nice to see you again, %s. Were you lost??", knownFriend->name);
            }
            else
            {
                AgoraFriend newFriend;
                memcpy(newFriend.mac, macAddr, 6);
                newFriend.channel = WiFi.channel();
                newFriend.lastMessageReceived = millis();
                newFriend.meToThem = IN_CONTACT;
                strcpy(newFriend.name, "unknown");
                strncpy(newFriend.tribe, tribename, AGORA_MAX_NAME_CHARACTERS);

                if (!Agora.addFriend(&newFriend))
                {
                    AGORA_LOG_V("Failed to add friend whose name I dont know yet");
                    return 0;
                }
            }
            return 1;
        }
    }

    else if (isMessage(incomingData, len, AGORA_MESSAGE_PRESENT))
    {
        AGORA_LOG_V("Presented himself");

        char membername[AGORA_MAX_NAME_CHARACTERS + 1];
        memcpy(membername, incomingData + strlen(AGORA_MESSAGE_PRESENT.string), AGORA_MAX_NAME_CHARACTERS);
        membername[AGORA_MAX_NAME_CHARACTERS] = 0;
        if (sendMessage((const uint8_t *)macAddr, AGORA_MESSAGE_WELCOME, Agora.name) != ESP_OK)
        {
            AGORA_LOG_V("Failed to send to peer");
            return 0;
        }
        AgoraFriend *theFriend = friendForMac(macAddr);
        if (theFriend == NULL)
        {
            AGORA_LOG_MAC(macAddr);
            AGORA_LOG_E(" Cannot find this guy anymaore.")
            return 0;
        }

        AGORA_LOG_MAC(theFriend->mac);
        Serial.printf("\n\n\nPointer Check for new Friend %s", theFriend->name);
        Serial.printf("%p theFriend, %p Agora\n\n", theFriend, Agora.friends[1]);

        strncpy(theFriend->name, membername, AGORA_MAX_NAME_CHARACTERS);
        theFriend->lastMessageReceived = millis();
        theFriend->meToThem = GURU;
        AGORA_LOG_V("\n\nHi %s", membername);
        AGORA_LOG_V("Welcome to the team: ")
        AGORA_LOG_FRIEND(theFriend);
        AGORA_LOG_V("\n\n");

        AgoraTribe *theTribe = tribeNamed(theFriend->tribe);
        theTribe->meToThem = GURU;
        theTribe->lastMessageReceived = millis();

        Agora.rememberFriends();
        return 1;
    }
    else if (isMessage(incomingData, len, AGORA_MESSAGE_PONG))
    {
        // it's ok, we've already logged the timestamp. nothing more to do here
        // AGORA_LOG_V("Got pong");
        return 1;
    }

    AgoraFriend *f = friendForMac(macAddr);
    if (f)
    {
        AgoraTribe *theTribe = tribeNamed(f->tribe);
        if (theTribe)
        {
            theTribe->callback(macAddr, incomingData, len);
            return 1;
        }
        else
        {
            AGORA_LOG_E("Cannot find tribe %s", f->tribe);
            return 0;
        }
    }
    else
    {
        AGORA_LOG_V("Not a message from a member. Dismissed");
        return 0;
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------
int handleAgoraMessageAsMember(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{

    /*         AGORA_LOG_V("Tribe %s, Status %d", name, myself.status);
            log_message(macAddr, incomingData, len); */

    if (isMessage(incomingData, len, AGORA_MESSAGE_LOST))
    {
        // I'm not a guru for this tribe. don't bother with lost messages
        return 0;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PONG))
    {
        // I'm not a guru for this tribe. don't bother with pong messages
        return 0;
    }
    if (isMessage(incomingData, len, AGORA_MESSAGE_PRESENT))
    { // I'm not a guru for this tribe. don't bother with present messages
        return 0;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_INVITE))
    {
        AGORA_LOG_MAC(macAddr);
        AGORA_LOG_V("  wants PIX !! ;-o ");
        char tribename[AGORA_MAX_NAME_CHARACTERS + 1];
        memcpy(tribename, incomingData + strlen(AGORA_MESSAGE_INVITE.string), AGORA_MAX_NAME_CHARACTERS);
        tribename[AGORA_MAX_NAME_CHARACTERS] = 0;
        int theChannel = incomingData[len - 2];
        theChannel = theChannel > 12 ? 1 : theChannel;
        theChannel = constrain(theChannel, 1, 12);
        AGORA_LOG_MAC(macAddr);
        AGORA_LOG_V(" from tribe %s , channel %u", tribename, theChannel);

        AgoraTribe *theTribe = tribeNamed(tribename);
        if (theTribe)
            theTribe->channel = theChannel;

        for (int i = 0; i < Agora.tribeCount; i++)
        {
            if (!strcmp(Agora.tribes[i].name, tribename))
            {
                Agora.tribes[i].meToThem = IN_CONTACT;
            }
        }

        if (WiFi.setChannel(theChannel))
        {
            AGORA_LOG_E("Faild to change channel to %u", theChannel);
        }

        if (!EspNowAddPeer(macAddr))
        {
            return 0;
        }

        AgoraFriend newFriend;
        memcpy(newFriend.mac, macAddr, 6);
        newFriend.channel = WiFi.channel();
        newFriend.lastMessageReceived = millis();
        newFriend.meToThem = IN_CONTACT;
        strcpy(newFriend.name, "unknown");
        strncpy(newFriend.tribe, tribename, AGORA_MAX_NAME_CHARACTERS);

        if (!Agora.addFriend(&newFriend))
        {
            AGORA_LOG_V("Failed to add GURU whose name I dont know yet");
            return 0;
        }

        if (sendMessage(macAddr, AGORA_MESSAGE_PRESENT, Agora.name) != ESP_OK)
        {
            AGORA_LOG_E("Failed to send to peer");
            return 0;
        }
        return 1;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_WELCOME))
    {
        AGORA_LOG_MAC(macAddr);
        char membername[AGORA_MAX_NAME_CHARACTERS + 1];
        memcpy(membername, incomingData + strlen(AGORA_MESSAGE_WELCOME.string), AGORA_MAX_NAME_CHARACTERS);
        membername[AGORA_MAX_NAME_CHARACTERS] = 0;

        AGORA_LOG_V("%s  welcomes me", membername);

        AgoraFriend *theFriend = friendForMac(macAddr);
        if (!theFriend)
        {
            AGORA_LOG_MAC(macAddr);
            AGORA_LOG_E(" Cannot find this Guru anymaore.")
            return 0;
        }

        strncpy(theFriend->name, membername, AGORA_MAX_NAME_CHARACTERS);
        theFriend->lastMessageReceived = millis();
        theFriend->meToThem = FOLLOWER;
        AGORA_LOG_V("\n\nHi %s", membername);
        AGORA_LOG_V("My adored GURU: ")
        AGORA_LOG_FRIEND(theFriend);
        AGORA_LOG_V("\n\n");

        AgoraTribe *theTribe = tribeNamed(theFriend->tribe);
        theTribe->meToThem = FOLLOWER;
        theTribe->lastMessageReceived = millis();

        Agora.rememberFriends();

        return 1;
    }

    AgoraFriend *f = friendForMac(macAddr);
    if (f)
        f->lastMessageReceived = millis();

    if (isMessage(incomingData, len, AGORA_MESSAGE_PING))
    {
        if (sendMessage(macAddr, AGORA_MESSAGE_PONG) != ESP_OK)
        {
            AGORA_LOG_E("Failed to send to peer");
            return 0;
        }
        return 1;
    }

    if (f)
    {
        AgoraTribe *theTribe = tribeNamed(f->tribe);
        if (theTribe)
        {
            theTribe->callback(macAddr, incomingData, len);
            return 1;
        }
        else
        {
            AGORA_LOG_E("Cannot find tribe %s", f->tribe);
            return 0;
        }
    }

    AGORA_LOG_V("Message not handled. Len %u", len);

    return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------
// determine if message is a known agora message

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
// callback function that will be executed when data is received

void generalCallback(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    if (Agora.ftp_enabled)
    {
        if (handle_agora_ftp(macAddr, incomingData, len))
        {
            // we dont want Pingpong while sending a file
            for (int i = 0; i < Agora.friendCount; i++)
            {
                Agora.friends[i].lastMessageReceived = millis();
                Agora.friends[i].lastMessageSent = millis();
            }
            return;
        }
    }

    // AGORA_LOG_INCOMING_DATA(incomingData, len);
    uint16_t washandled = 0;
    if (isMessage(incomingData, len, AGORA_MESSAGE_POLICE))
    {
        memcpy(policeMac, macAddr, 6);
        EspNowAddPeer(policeMac);
        Agora.showID();
        return;
    }

    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (Agora.tribes[i].meToThem == GURU)
        {

            washandled += handleAgoraMessageAsGuru(macAddr, incomingData, len);
        }
        else
        {
            washandled += handleAgoraMessageAsMember(macAddr, incomingData, len);
        }
    }
    // AGORA_LOG_V("Message was handled %d times", washandled);
}

#if ESP_IDF_VERSION_MAJOR < 5
void OnDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    generalCallback(macAddr, incomingData, len);
}
#else
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len)
{
    generalCallback(esp_now_info->src_addr, incomingData, len);
}
#endif

esp_err_t sendMessage(const uint8_t *macAddr, AgoraMessage message, char *name)
{
    uint8_t buf[6];
    memcpy(buf, macAddr, 6);
    return sendMessage(buf, message, name);
}

esp_err_t sendMessage(uint8_t *macAddr, AgoraMessage message, char *name)
{
    char buf[message.size];
    memset(buf, '.', message.size);
    buf[message.size - 1] = 0;
    sprintf(buf, "%s%s\0", message.string, name);
    if (Agora.logMessages)
    {
        ("Sending Message: %s", buf);
    }
    AgoraFriend *f = friendForMac(macAddr);
    if (f)
        f->lastMessageSent = millis();
    return esp_now_send(macAddr, (uint8_t *)buf, message.size);
}

esp_err_t sendMessage(AgoraFriend *to, AgoraMessage message, char *name)
{
    //  to->lastMessageSent = millis();
    return sendMessage(to->mac, message, name);
}
//-----------------------------------------------------------------------------------------------------------------------------
// sends ping to followers regularly -  returns  number of Followers that don't reply in a timely manner
int checkMyFollowers()
{
    int missingFollowers = 0;
    for (int i = 0; i < Agora.friendCount; i++)
    {
        if (Agora.friends[i].meToThem == GURU)
        {
            if (millis() - Agora.friends[i].lastMessageSent > Agora.pingInterval)
            {
                sendMessage(&Agora.friends[i], AGORA_MESSAGE_PING);
            }

            if (millis() - Agora.friends[i].lastMessageReceived > 2 * Agora.pingInterval)
            {
                missingFollowers++;
            }
        }
    }
    return missingFollowers;
}
//-----------------------------------------------------------------------------------------------------------------------------
// returns  number of GURUS that we haven't heard of in a while

int checkMyGurus()
{
    int missingGurus = 0;
    for (int i = 0; i < Agora.friendCount; i++)
    {
        if (Agora.friends[i].meToThem == FOLLOWER)
        {
            if (millis() - Agora.friends[i].lastMessageReceived > 2 * Agora.pingInterval)
            {
                missingGurus++;
            }
        }
    }
    return missingGurus;
}

//-----------------------------------------------------------------------------------------------------------------------------
// autoPair
void lookForNewGurus()
{
    static int connect_fail_count;

    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (Agora.tribes[i].meToThem == ASPIRING_FOLLOWER)
        {
            // see if were not acually following this triube
            for (int f = 0; f < Agora.friendCount; f++)
            {
                if (Agora.friends[f].meToThem == FOLLOWER)
                {
                    if (!strcmp(Agora.friends[f].tribe, Agora.tribes[i].name))
                    {
                        Agora.tribes[i].meToThem = FOLLOWER;
                        AGORA_LOG_V("Found my GURU for tribe %s: %s", Agora.tribes[i].name, Agora.friends[f].name);
                        return;
                    }
                }
            }

            if (connect_fail_count > 5)
            {
                Agora.tribes[i].channel = (Agora.tribes[i].channel < ESP_NOW_MAX_CHANNEL) ? Agora.tribes[i].channel + 1 : 1;
                AGORA_LOG_V("\n\nConnection failed %d times. Trying next channel: %d\n", connect_fail_count, Agora.tribes[i].channel);
                connect_fail_count = 0;
                ESP_ERROR_CHECK(esp_wifi_set_channel(Agora.tribes[i].channel, WIFI_SECOND_CHAN_NONE));
                if (esp_now_init() != ESP_OK)
                {
                    Serial.println("Error initializing ESP-NOW");
                }
                esp_now_del_peer(BROADCAST_ADDRESS);
                EspNowAddPeer(BROADCAST_ADDRESS);
            }
            AGORA_LOG_V("Sending %s", AGORA_MESSAGE_LOST.string);
            AgoraFriend temp;
            memcpy(temp.mac, BROADCAST_ADDRESS, 6);
            sendMessage(&temp, AGORA_MESSAGE_LOST, Agora.tribes[i].name);
            connect_fail_count++;
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
// Agora Task
void agoraTask(void *)
{
    AGORA_LOG_V("\n\n---------------------------\nStarting Agora Task\n\n");

    AgoraPreferences.begin("Agora");
    Agora.friendCount = AgoraPreferences.getInt("friendCount", 0);
    Agora.wifiChannel = AgoraPreferences.getInt("wifiChannel", 0);

    if (Agora.friendCount > AGORA_MAX_FRIENDS)
        Agora.friendCount = 0;

    Agora.wifiChannel = constrain(Agora.wifiChannel, 1, ESP_NOW_MAX_CHANNEL);

    AGORA_LOG_V("Recalling %d friends.");
    int storageBytes = 0;
    int imAMaster, imASlave;
    for (int i = 0; i < Agora.friendCount; i++)
    {
        char prefname[12];
        sprintf(prefname, "friend%02d", i);
        AgoraPreferences.getBytes(prefname, &Agora.friends[i], sizeof(AgoraFriend));
        storageBytes += sizeof(AgoraFriend);
        AGORA_LOG_FRIEND(Agora.friends[i]);
        if (Agora.friends[i].meToThem == GURU)
        {
            imAMaster++;
        }
        else
        {
            imASlave++;
        }
        Agora.friends[i].lastMessageReceived = 0;
        Agora.friends[i].lastMessageSent = 0;
    }
    AGORA_LOG_V("Recalled %d bytes.");
    AgoraPreferences.end();

    if (imAMaster > imASlave)
    {
        AGORA_LOG_V("I'm more of a GURU type of person");
    }

    // Start WiFI if not already done
    if (WiFi.getMode() == WIFI_MODE_NULL)
    {
        WiFi.mode(WIFI_STA);
        ESP_ERROR_CHECK(esp_wifi_set_channel(Agora.wifiChannel > 0 ? Agora.wifiChannel : 1, WIFI_SECOND_CHAN_NONE));
    }
    else
    {
        Agora.wifiChannel = WiFi.channel();
    }

    delay(1000);
    if (esp_now_init() != ESP_OK)
    {
        AGORA_LOG_E("Error initializing ESP-NOW");
        return;
    }
    else
    {
        AGORA_LOG_V("Initialized ESP-NOW");
    }
    // set callback routines
    /*       esp_now_register_send_cb(OnDataSent);
     */
    esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

    AGORA_LOG_V("IP address: %s", WiFi.localIP().toString().c_str());
    AGORA_LOG_V("MAC address: %s", WiFi.macAddress().c_str());
    AGORA_LOG_V("WiFi channel: %d", WiFi.channel());

    for (int i = 0; i < Agora.friendCount; i++)
    {
        EspNowAddPeer(Agora.friends[i].mac);
    }

    while (1)
    {
        int missing_connections = 0;
        missing_connections += checkMyFollowers();
        missing_connections += checkMyGurus();
        Agora.connectedPercent = 100 * Agora.friendCount - 100 * missing_connections;
        lookForNewGurus();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//----------------------------------------------------------------------------------------

void send_header()
{
    if (!file_to_share)
    {
        Serial.println("Cannot open file");
        return;
    }

    strcpy(esp_fileshare_header.magicword, "lookatthis");
    strcpy(esp_fileshare_header.filename, file_to_share.name());
    esp_fileshare_header.filesize = file_to_share.size();
    esp_err_t result = esp_now_send(NULL, (uint8_t *)&esp_fileshare_header, sizeof(esp_fileshare_header));

    if (result == ESP_OK)
    {
        Serial.println("Header sent with success");
        bytes_to_send = file_to_share.size();
        percent_done = 0;
        Serial.printf("Send %d bytes\n", bytes_to_send);
    }
    else
    {
        Serial.println("Error sending the data");
        bytes_to_send = 0;
    }
}

//----------------------------------------------------------------------------------------

void send_chunk()
{
    if (!file_to_share)
    {
        Serial.println("Cannot open file");
        return;
    }

    char buffer[ESPNOW_FILESHARE_CHUNK_SIZE];
    if (bytes_to_send)
    {
        int bytes_to_send_now = bytes_to_send > ESPNOW_FILESHARE_CHUNK_SIZE ? ESPNOW_FILESHARE_CHUNK_SIZE : bytes_to_send;
        file_to_share.readBytes(buffer, bytes_to_send_now);
        int percent = 100 - bytes_to_send / file_to_share.size();
        esp_err_t result = esp_now_send(NULL, (uint8_t *)buffer, bytes_to_send_now);

        if (result == ESP_OK)
        {
            if (percent != percent_done)
            {
                percent_done = percent;
                Serial.print('.');
            }
            bytes_to_send -= bytes_to_send_now;
        }
        else
        {
            Serial.println("\n\nError sending the data");
            bytes_to_send = 0;
            file_to_share.close();
        }
    }
    else
    {
        Serial.println("\n\nDONE sending the data");
        file_to_share.close();
    }
}

bool handle_agora_ftp(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    if (len == 11)
    {
        if (strcmp((const char *)incomingData, "gimmemore!") == 0)
        {
            Serial.printf("Let's send the file then. %d bytes to go\n", bytes_to_send);
            send_chunk();
            return true;
        }
    }

    if (bytes_to_receive)
    {
        if (len == ESPNOW_FILESHARE_CHUNK_SIZE || len == bytes_to_receive)
        {
            if (file_to_write.write(incomingData, len) == len)
            {
                bytes_to_receive -= len;
                if (bytes_to_receive > 0)
                {
                    const uint8_t mess[] = "gimmemore!";
                    esp_err_t result = esp_now_send(macAddr, mess, sizeof(mess));
                }
                else
                {
                    if (bytes_to_receive < 0)
                    {
                        Serial.println("TOO MANY BYTES RECEIVED ????");
                    }
                    file_to_write.close();
                    delay(200);
                    char filepath[100];
                    sprintf(filepath, "/%s", esp_fileshare_header.filename);
                    /*                     File file = SD.open(filepath, "r");
                                        size_t writtensize = file.size();
                                        file.close();
                                        if (writtensize != esp_fileshare_header.filesize)
                                        {
                                            Serial.printf("\nERROR: Written File size does not match expecttions %d written vs %d\n", writtensize, esp_fileshare_header.filesize);
                                        }
                                        else
                                        {
                     */
                    long timetaken = millis() - file_transfer_start;
                    Serial.printf("SUCCESS: File written %d bytes in %d.%d seconds (%d kbit/s)", esp_fileshare_header.filesize, timetaken / 1000, timetaken % 1000, esp_fileshare_header.filesize * 8000 / timetaken / 1024);
                    memset(&esp_fileshare_header, 0, sizeof(esp_fileshare_header));
                    Serial.println();
                    //}
                }
            }
            else
            {
                Serial.println("Error writing file");
            }
            return true;
        }
        else
        {
            Serial.printf("Weird number of bytes received: %d.", len);
            Serial.printf("Should be %d or %d\n", bytes_to_receive, ESPNOW_FILESHARE_CHUNK_SIZE);
        }
    }
    else if (len == sizeof(esp_fileshare_header_t))
    {
        memcpy(&esp_fileshare_header, incomingData, len);
        if (strcmp(esp_fileshare_header.magicword, "lookatthis") == 0)
        {
            Serial.printf("Receive File %s of size %d\n", esp_fileshare_header.filename, esp_fileshare_header.filesize);
            file_transfer_start = millis();
            bytes_to_receive = esp_fileshare_header.filesize;

            char filepath[100];
            sprintf(filepath, "/%s", esp_fileshare_header.filename);
            file_to_write = Fileshare_Filesystem.open(filepath, "w");
            if (!file_to_write)
            {
                Serial.printf("Error opening file for writing %s\n", filepath);
                return true;
            }
            const uint8_t mess[] = "gimmemore!";
            esp_err_t result = esp_now_send(macAddr, mess, sizeof(mess));
            return true;
        }
    }

    return false; // this was not a FTP message
}

void TheAgora::share(File file)
{
    if (!ftp_enabled)
        return;
    file_to_share = file;
    send_header();
};