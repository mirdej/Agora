/*TODOS

Tribe list does not get real updates
POLICE MODE


*/

#include "Agora.h"
#include "Preferences.h"
#include <esp_wifi.h>

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

uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
    switch (r)
    {
    case GURU:
        Serial.print("GURU      ");
        break;
    case FOLLOWER:
        Serial.print("FOLLOWER  ");
        break;
    case ASPIRING_FOLLOWER:
        Serial.print("ASPIRING  ");
        break;
    case IN_CONTACT:
        Serial.print("CONTACTING");
        break;
    default:
        Serial.print("UNKNOWN   ");
    }
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
    Serial.println("\n\nFriends");
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
        Serial.printf("%9u | %9u\n", millis() - f.lastMessageReceived, millis() - f.lastMessageSent);
    }
}

void AGORA_LOG_TRIBE_TABLE()
{
    Serial.println("\nTribes");
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
        AGORA_LOG_FRIEND_TABLE();
        // AGORA_LOG_TRIBE_TABLE();
    }
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
    memcpy(&Agora.tribes[Agora.tribeCount], &newTribe, sizeof(AgoraTribe));
    Agora.tribeCount++;
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *text)
{
}

void TheAgora::tell(uint8_t *buf, int len)
{
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(const char *name, uint8_t *buf, int len)
{
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

void TheAgora::begin(const char *newname)
{
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

bool macMatch(uint8_t *a, uint8_t *b)
{
    for (int i = 0; i < 6; i++)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

bool macMatch(AgoraFriend *a, AgoraFriend *b)
{
    return macMatch(a->mac, b->mac);
}

AgoraFriend *friendForMac(uint8_t *mac)
{
    for (int i; i < Agora.friendCount; i++)
    {
        if (macMatch(Agora.friends[i].mac, mac))
        {
            return Agora.friends + i * sizeof(AgoraFriend);
        }
    }
    return NULL;
}

AgoraFriend *friendForMac(const uint8_t *mac)
{
    return friendForMac((uint8_t *)mac);
}

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
            if (sendMessage(macAddr, AGORA_MESSAGE_INVITE, tribename) != ESP_OK)
            {
                AGORA_LOG_V("Failed to send to peer");
                return 0;
            }

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

        strncpy(theFriend->name, membername, AGORA_MAX_NAME_CHARACTERS);
        theFriend->lastMessageReceived = millis();
        theFriend->meToThem = GURU;
        AGORA_LOG_V("\n\nHi %s", membername);
        AGORA_LOG_V("Welcome to the team: ")
        AGORA_LOG_FRIEND(theFriend);
        AGORA_LOG_V("\n\n");
        Agora.rememberFriends();

        return 1;
    }
    else if (isMessage(incomingData, len, AGORA_MESSAGE_PONG))
    {
        // it's ok, we've already logged the timestamp. nothing more to do here
        // AGORA_LOG_V("Got pong");
        return 1;
    }

    if (message_is_from_a_friend)
    {
        AGORA_LOG_V("Message from a member. Call Callback");
        /*         callback(macAddr, incomingData, len); */
        return 1;
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
        AGORA_LOG_MAC(macAddr);
        AGORA_LOG_V(" from tribe %s ", tribename);

        for (int i = 0; i < Agora.tribeCount; i++)
        {
            if (!strcmp(Agora.tribes[i].name, tribename))
            {
                Agora.tribes[i].meToThem = IN_CONTACT;
            }
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

    /* if (sender == guru.macAddress)
    {
        myself.time_of_last_received_message = millis();

        callback(macAddr, incomingData, len);
        //      AGORA_LOG_V("Message for me from GURU %s == %s", sender.toString().c_str(),guru.macAddress.toString().c_str());
        return true;
    }
    AGORA_LOG_V("Not a message from my GURU. Dismissed"); */
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
    // AGORA_LOG_INCOMING_DATA(incomingData, len);
    uint16_t washandled = 0;
    ;
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
                        AGORA_LOG_V("Found my GURU for tribe %s: %%s", Agora.tribes[i].name, Agora.friends[f].name);
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
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}
