/*TODOS

Purge friends list, remove long time inactive members ???

Tribe list does not get real updates (wifi channels ??)
__BASE_FILE__ üath is no absolute

Auto pairing crashes ESP if wifi is already enabled. Once the triend info is stored in preferences everything works as expected.
ALSO E (45996) ESPNOW: Peer channel is not equal to the home channel, send fail!
*/

#include "Agora.h"
#include "esp_mac.h"

TheAgora Agora;
Preferences AgoraPreferences;

esp_now_peer_info_t tempPeer;

AgoraMessage AGORA_MESSAGE_LOST = {"HALLLOOO??", 61};
AgoraMessage AGORA_MESSAGE_INVITE = {"PLZDM_ME!!", 53};
AgoraMessage AGORA_MESSAGE_PRESENT = {"Hi, I'm ", 75};
AgoraMessage AGORA_MESSAGE_WELCOME = {"I'm your guru: ", 83};
AgoraMessage AGORA_MESSAGE_PING = {"Huh?", 4};
AgoraMessage AGORA_MESSAGE_PONG = {"Ha!", 3};
AgoraMessage AGORA_MESSAGE_POLICE = {"POLICE!Your ID Please!", 25};
AgoraMessage AGORA_MESSAGE_FTP_DONE = {"GOT the file. Thanks.", 29};
AgoraMessage AGORA_MESSAGE_FTP_ABORT = {"Fucked up. Over.", 19};
AgoraMessage AGORA_MESSAGE_WIFI_PROV = {"Here's da kod:", 122};
AgoraMessage AGORA_MESSAGE_SINGLE_INT = {"A", 1 + sizeof(int)};
AgoraMessage AGORA_MESSAGE_DOUBLE_INT = {"B", 1 + 2 * sizeof(int)};
AgoraMessage AGORA_MESSAGE_TRIPLE_INT = {"C", 1 + 3 * sizeof(int)};

uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const char *r_strings[] = {
    "UNKNOWN   ",
    "ASPIRING  ",
    "CONTACTING",
    "FOLLOWER  ",
    "GURU      ",
    "ABSENT    "};

agoraFileshareHeader_t fileshareHeader;
agoraFileSender_t fileSender;
agoraFileReceiver_t fileReceiver;
FS Fileshare_Filesystem = SPIFFS;
TaskHandle_t AgoraTaskHandle;
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
        Serial.printf("%4u | %-30s | %-30s | ", f.channel, f.name, f.tribe);
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
    Serial.println(" Type       | Chan | Name                           | Autopair | pair until ");
    Serial.println("-----------------------------------------------------------------------------------");
    for (int i = 0; i < Agora.tribeCount; i++)
    {
        AgoraTribe t = Agora.tribes[i];
        AGORA_LOG_RELATIONSHIP(t.meToThem);
        Serial.print(" | ");
        Serial.printf("%4u | %-30s | ", t.channel, t.name);
        Serial.printf("%s | %9u\n", t.autoPair ? "Yes" : "No", t.pairUntil);
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
    static long last_log;
    static int lastConnections;
    bool doLog = false;

    if (Agora.activeConnectionCount != lastConnections)
    {
        doLog = true;
        Serial.printf("Active connections changed from %d to %d\n", lastConnections, Agora.activeConnectionCount);
        lastConnections = Agora.activeConnectionCount;
    }
    if (interval > 0 && millis() - last_log > interval)
    {
        doLog = true;
    }
    if (!doLog)
        return;

    last_log = millis();
    Serial.println("---------------------------------------------------------------------------------------------------------------------------------------");
    Serial.printf("THE AGORA | Status for %s | ", Agora.name);
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK)
    {
        Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x",
                      baseMac[0], baseMac[1], baseMac[2],
                      baseMac[3], baseMac[4], baseMac[5]);
    }
    else
    {
        Serial.print("Failed to read MAC address");
    }
    Serial.printf(" | Number of active connections %u\n", Agora.connected());
    Serial.println("---------------------------------------------------------------------------------------------------------------------------------------");
    Serial.println();
    AGORA_LOG_TRIBE_TABLE();
    Serial.println();
    AGORA_LOG_FRIEND_TABLE();
    Serial.println("---------------------------------------------------------------------------------------------------------------------------------------\n\n\n");
}

//-----------------------------------------------------------------------------------------------------------------------------

#if ESP_IDF_VERSION_MAJOR < 5
void AgoraOnDataRecv(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{
    generalCallback(macAddr, incomingData, len);
}
#else
void AgoraOnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len)
{
    generalCallback(esp_now_info->src_addr, incomingData, len);
}
#endif

//-----------------------------------------------------------------------------------------------------------------------------
// This Task gets spawned if we are asked for ID by Agora-Police

uint8_t policeMac[6] = {0, 0, 0, 0, 0, 0};

void idTask(void *)
{
    randomSeed(millis());
    delay(random(3000));
    char buf[240];

    int hasPoliceMac = 0;
    for (int i = 0; i < 6; i++)
    {
        hasPoliceMac += policeMac[i];
    }
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    snprintf(buf, 240, "1|AgoraVersion:%s|Name:%s|Chip:%s|Flash:%u|FreeRam:%u|SDK:%s|IP:%s|Channel:%u|", Agora.getVersion(),Agora.name, ESP.getChipModel(), ESP.getFlashChipSize(), info.total_free_bytes, ESP.getSdkVersion(), WiFi.localIP().toString().c_str(), WiFi.channel());
    Serial.println(buf);
    if (hasPoliceMac)
        esp_now_send(policeMac, (uint8_t *)buf, 240);

    delay(100);

    snprintf(buf, 240, "2|Friends:%u|Tribes:%u||File:%s|Date:%s|", Agora.friendCount, Agora.tribeCount, Agora.includedBy, __DATE__);
    Serial.println(buf);
    if (hasPoliceMac)
        esp_now_send(policeMac, (uint8_t *)buf, 240);

    delay(100);

    for (int i = 0; i < Agora.tribeCount; i++)
    {
        AgoraTribe t = Agora.tribes[i];
        snprintf(buf, 240, "3|Tribe:%s|%u|%s|%u|%u", t.name, t.channel, r_strings[t.meToThem], t.autoPair, t.pairUntil);
        Serial.println(buf);
        if (hasPoliceMac)
            esp_now_send(policeMac, (uint8_t *)buf, 240);
        delay(30);
    }

    for (int i = 0; i < Agora.friendCount; i++)
    {
        AgoraFriend f = Agora.friends[i];

        snprintf(buf, 240, "4|Friend:%02x:%02x:%02x:%02x:%02x:%02x|%u|%s|%s|%s|%u|%u",
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
/* void TheAgora::establish(const char *name, bool autoPair)
{
    TheAgora::establish(name, dummyCallback, autoPair);
}
 */
void TheAgora::establish(const char *name, agora_cb_t cb, bool autoPair)
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
    newTribe.autoPair = autoPair;
    memcpy(&Agora.tribes[Agora.tribeCount], &newTribe, sizeof(AgoraTribe));
    Agora.tribeCount++;
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::join(const char *name, agora_cb_t cb, bool autoPair)
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
    newTribe.autoPair = autoPair;
    memcpy(&Agora.tribes[Agora.tribeCount], &newTribe, sizeof(AgoraTribe));
    Agora.tribeCount++;
}

//-----------------------------------------------------------------------------------------------------------------------------
// Start Pairing for cults that aren't pairing automatically
void TheAgora::conspire(int forSeconds, const char *cult)
{
    for (int i = 0; i < tribeCount; i++)
    {
        if (cult == NULL || !strcmp(tribes[i].name, cult))
        {
            tribes[i].pairUntil = millis() + 1000 * forSeconds;

            // A guru just has to listen to new membership requests
            if (tribes[i].meToThem != GURU)
            {
                tribes[i].meToThem = ASPIRING_FOLLOWER;
                //  forget my guru and start over
                for (int f = 0; f < friendCount; f++)
                {
                    if (!strcmp(tribes[i].name, friends[f].tribe))
                    {
                        friends[f].meToThem = UNKNOWN;
                        esp_now_del_peer(friends[f].mac);
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

int TheAgora::isPairing()
{
    int pairCount = 0;
    for (int i = 0; i < tribeCount; i++)
    {
        if (millis() < tribes[i].pairUntil)
        {
            if (tribes[i].meToThem == GURU)
            {
                pairCount++;
            }
            else
            {
                if (tribes[i].meToThem != FOLLOWER)
                {
                    pairCount++;
                }
            }
        }
    }
    return pairCount;
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

//-----------------------------------------------------------------------------------------------------------------------------

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
    if ((ftpEnabled) && (fileSender.bytesRemaining || fileReceiver.bytesRemaining))
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
    for (int i = 0; i < friendCount; i++)
    {
        if (!strcmp(friends[i].name, name) || !strcmp(friends[i].tribe, name))
        {
            if (ftpEnabled)
            {
                if ((fileSender.bytesRemaining || fileReceiver.bytesRemaining))
                {
                    if (macMatch(fileSender.receiverMac, friends[i].mac))
                    {
                        AGORA_LOG_E("Sharing a File, Please wait until done !!!");
                        continue;
                    }
                    if (macMatch(fileReceiver.senderMac, friends[i].mac))
                    {
                        AGORA_LOG_E("Sharing a File, Please wait until done !!!");
                        continue;
                    }
                }
            }

            if (esp_now_send(friends[i].mac, buf, len) != ESP_OK)
            {
                AGORA_LOG_E("Error sending message to member %s", friends[i].name);
            }
            friends[i].lastMessageSent = millis();
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::tell(int a)
{
    int len = 1 + sizeof(a);

    uint8_t buf[len];
    strncpy((char *)buf, AGORA_MESSAGE_SINGLE_INT.string, 1);
    memcpy(&buf[1], (uint8_t *)&a, sizeof(a));
    tell(buf, len);
}

void TheAgora::tell(int a, int b)
{
    int len = 1 + sizeof(a) + sizeof(b);

    uint8_t buf[len];
    strncpy((char *)buf, AGORA_MESSAGE_DOUBLE_INT.string, 1);
    memcpy(&buf[1], (uint8_t *)&a, sizeof(a));
    memcpy(&buf[1 + sizeof(a)], (uint8_t *)&b, sizeof(b));
    tell(buf, len);
}

void TheAgora::tell(int a, int b, int c)
{
    int len = 1 + sizeof(a) + sizeof(b) + sizeof(c);

    uint8_t buf[len];
    strncpy((char *)buf, AGORA_MESSAGE_TRIPLE_INT.string, 1);
    memcpy(&buf[1], (uint8_t *)&a, sizeof(a));
    memcpy(&buf[1 + sizeof(a)], (uint8_t *)&b, sizeof(b));
    memcpy(&buf[1 + sizeof(a) + sizeof(b)], (uint8_t *)&c, sizeof(c));
    tell(buf, len);
}

//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::answer(const uint8_t *mac, uint8_t *buf, int len)
{
    if ((ftpEnabled) && (fileSender.bytesRemaining || fileReceiver.bytesRemaining))
    {
        AGORA_LOG_E("Sharing a File, Please wait until done !!!");
        return;
    }
    if (esp_now_send(mac, buf, len) != ESP_OK)
    {
        AGORA_LOG_E("Error sending message to peer");
    }
    AgoraFriend *knownFriend = friendForMac(mac);
    if (knownFriend)
    {
        knownFriend->lastMessageSent = millis();
    }
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

void TheAgora::begin(bool addressInName)
{
    begin(AGORA_ANON_NAME, addressInName);
}

void TheAgora::begin(const char *newname, bool addressInName, const char *caller)
{

    strncpy(includedBy, caller, 127);

    AgoraPreferences.begin("Agora");
    Agora.address = AgoraPreferences.getInt("address", 0);
    Agora.friendCount = AgoraPreferences.getInt("friendCount", 0);
    Agora.wifiChannel = AgoraPreferences.getInt("wifiChannel", 0);

    if (Agora.friendCount > AGORA_MAX_FRIENDS)
        Agora.friendCount = 0;

    Agora.wifiChannel = constrain(Agora.wifiChannel, 1, ESP_NOW_MAX_CHANNEL);

    AGORA_LOG_V("Recalling %u friends.", Agora.friendCount);
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
    AGORA_LOG_V("Recalled %d bytes.", storageBytes);
    AgoraPreferences.end();

    if (imAMaster > imASlave)
    {
        AGORA_LOG_V("I'm more of a GURU type of person");
    }

    char buf[AGORA_MAX_NAME_CHARACTERS + 1];
    memset(buf, 0, AGORA_MAX_NAME_CHARACTERS + 1);

    if (address == 0)
    {
        uint8_t baseMac[6];
        esp_err_t ret = esp_efuse_mac_get_default(baseMac);
        address = ((baseMac[4] * 256 + baseMac[5]) % 512) + 1; // generate DMX-Style Address from Mac Address;
    }

    if (addressInName)
    {
        snprintf(buf, AGORA_MAX_NAME_CHARACTERS, "%s-%03u", newname, address);
        strncpy(name, buf, AGORA_MAX_NAME_CHARACTERS);
    }
    else
    {
        strncpy(name, newname, AGORA_MAX_NAME_CHARACTERS);
    }

    name[AGORA_MAX_NAME_CHARACTERS] = 0;
    pingInterval = AGORA_DEFAULT_PING_INTERVAL;

    AGORA_LOG_V("Enter the Agora as %s", name);

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

    // delay(1000);
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
    esp_now_register_recv_cb(esp_now_recv_cb_t(AgoraOnDataRecv));

    AGORA_LOG_V("IP address: %s", WiFi.localIP().toString().c_str());
    AGORA_LOG_V("MAC address: %s", WiFi.macAddress().c_str());
    AGORA_LOG_V("WiFi channel: %d", WiFi.channel());

    for (int i = 0; i < Agora.friendCount; i++)
    {
        EspNowAddPeer(Agora.friends[i].mac);
    }

    xTaskCreate(
        agoraTask,    // Function that implements the task.
        "Agora Task", // Text name for the task.
        8192,         // Stack size in words, not bytes.
        NULL,         // Parameter passed into the task.
        1,            // Priority at which the task is created.
        &AgoraTaskHandle);
}

//----------------------------------------------------------------------------------------

void TheAgora::end()
{
    esp_now_deinit();
    if (WiFi.status() != WL_CONNECTED)
    {
        WiFi.mode(WIFI_OFF);
    }
    vTaskDelete(AgoraTaskHandle);
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

AgoraFriend *friendNamed(char *name)
{
    for (int i = 0; i < Agora.friendCount; i++)
    {
        if (!strcmp(Agora.friends[i].name, name))
        {
            return &Agora.friends[i];
        }
    }
    return NULL;
}

AgoraFriend *friendNamed(const char *name)
{
    return friendNamed((char *)name);
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
    // remove peers so we don't accidentally reply to deleted friends
    for (int i = 0; i < Agora.friendCount; i++)
    {
        esp_now_del_peer(Agora.friends[i].mac);
    }
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
    AGORA_LOG_V("Stored %d bytes (plus some).", storageBytes);
}
//-----------------------------------------------------------------------------------------------------------------------------

void TheAgora::openTheGate()
{
    wifi_config_t wifi_cfg;
    esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg);
    openTheGate((const char *)wifi_cfg.sta.ssid, (const char *)wifi_cfg.sta.password);
}

void TheAgora::openTheGate(const char *ssid, const char *pass)
{
    AgoraMessage message = AGORA_MESSAGE_WIFI_PROV;
    char buf[message.size];
    memset(buf, 0, message.size);
    strcpy(buf, message.string);
    memcpy(buf + AGORA_WIFI_PROV_SSID_OFFSET, ssid, AGORA_MAX_NAME_CHARACTERS);
    memcpy(buf + AGORA_WIFI_PROV_PASS_OFFSET, pass, AGORA_MAX_NAME_CHARACTERS);
    esp_now_send(NULL, (uint8_t *)buf, message.size);
}

//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
// check if message is from a known member, if so, call this tribe's callback function
bool tribe(const char *name)
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

        AgoraTribe *t = tribeNamed(tribename);
        if (t == NULL)
            return 0; // not in my tribes list

        if (t->meToThem != GURU)
            return 0; // I am not a guru for this tribe

        if (t->autoPair == false)
        {
            if (millis() > t->pairUntil)
            {
                return 0; // I am not accepting members right now
            }
        }

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
        if (Agora.eavesdrop)
        {
            Serial.printf("Sending Message: %s to ", buf);
            AGORA_LOG_MAC(macAddr);
            Serial.println();
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
        // theTribe->lastMessageReceived = millis();

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
            if (theTribe->callback != NULL)
            {
                theTribe->callback(macAddr, incomingData, len);
            }
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

        if (esp_wifi_set_channel(theChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK)
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
        // theTribe->lastMessageReceived = millis();

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
            if (theTribe->callback != NULL)
            {
                theTribe->callback(macAddr, incomingData, len);
            }
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

    if (Agora.eavesdrop)
    {
        AGORA_LOG_MAC(macAddr);
        Serial.print(" ");
        AGORA_LOG_INCOMING_DATA(incomingData, len);
    }
    if (Agora.ftpEnabled)
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

    uint16_t washandled = 0;
    if (isMessage(incomingData, len, AGORA_MESSAGE_POLICE))
    {
        memcpy(policeMac, macAddr, 6);
        EspNowAddPeer(policeMac);
        Agora.showID();
        return;
    }

    if (isMessage(incomingData, len, AGORA_MESSAGE_WIFI_PROV))
    {
        if (WiFi.isConnected())
        {
            AGORA_LOG_V("Wifi already connected");
            return;
        }

        char ssid[AGORA_MAX_NAME_CHARACTERS + 1];
        char pass[AGORA_MAX_NAME_CHARACTERS + 1];
        memset(ssid, 0, AGORA_MAX_NAME_CHARACTERS + 1);
        memset(pass, 0, AGORA_MAX_NAME_CHARACTERS + 1);
        memcpy(ssid, incomingData + AGORA_WIFI_PROV_SSID_OFFSET, AGORA_MAX_NAME_CHARACTERS);
        memcpy(pass, incomingData + AGORA_WIFI_PROV_PASS_OFFSET, AGORA_MAX_NAME_CHARACTERS);

        AGORA_LOG_V("Trying to connect to Wifi %s using password %s", ssid, pass);

        WiFi.setHostname(Agora.name);
        WiFi.begin(ssid, pass);

        long startTime = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(10);
            if (millis() - startTime > 4000)
            {
                break;
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            AGORA_LOG_V("Failed to connect. Timout");
        }
        else
        {
            AGORA_LOG_V("Connected.")
            AGORA_LOG_V("IP address: %s", WiFi.localIP().toString().c_str());
            AGORA_LOG_V("Wifi Hostname: %s", WiFi.getHostname());
        }
        return;
    }

    // handle simple messages
    if (Agora.singleIntCallback != NULL)
    {
        if (isMessage(incomingData, len, AGORA_MESSAGE_SINGLE_INT))
        {
            int a = 0;
            int off = strlen(AGORA_MESSAGE_SINGLE_INT.string);
            memcpy(&a, &incomingData[off], sizeof(a));
            Agora.singleIntCallback(a);
            return;
        }
    }
    if (Agora.doubleIntCallback != NULL)
    {
        if (isMessage(incomingData, len, AGORA_MESSAGE_DOUBLE_INT))
        {
            int a, b = 0;
            int off = strlen(AGORA_MESSAGE_DOUBLE_INT.string);
            memcpy(&a, &incomingData[off], sizeof(a));
            off += sizeof(a);
            memcpy(&b, &incomingData[off], sizeof(b));
            Agora.doubleIntCallback(a, b);
            return;
        }
    }
    if (Agora.tripleIntCallback != NULL)
    {
        if (isMessage(incomingData, len, AGORA_MESSAGE_TRIPLE_INT))
        {
            int a, b, c = 0;
            int off = strlen(AGORA_MESSAGE_TRIPLE_INT.string);
            memcpy(&a, &incomingData[off], sizeof(a));
            off += sizeof(a);
            memcpy(&b, &incomingData[off], sizeof(b));
            off += sizeof(b);
            memcpy(&c, &incomingData[off], sizeof(c));
            Agora.tripleIntCallback(a, b, c);
            return;
        }
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
    if (Agora.eavesdrop)
    {
        Serial.printf("Sending Message: %s to ", buf);
        AGORA_LOG_MAC(macAddr);
        Serial.println();
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
// autoPair
void lookForNewGurus()
{
    static int connect_fail_count;

    for (int i = 0; i < Agora.tribeCount; i++)
    {
        if (Agora.tribes[i].autoPair == false)
        {
            if (millis() > Agora.tribes[i].pairUntil)
            {
                // skip this tribe if we're not allowed to pair
                continue;
            }
        }

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
                        AGORA_LOG_V("Recalled my GURU for tribe %s: %s", Agora.tribes[i].name, Agora.friends[f].name);
                        return;
                    }
                }
            }

            if (!WiFi.isConnected() && connect_fail_count > 5)
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
            // AGORA_LOG_V("Sending %s", AGORA_MESSAGE_LOST.string);
            AgoraFriend temp;
            memcpy(temp.mac, BROADCAST_ADDRESS, 6);
            sendMessage(&temp, AGORA_MESSAGE_LOST, Agora.tribes[i].name);
            connect_fail_count++;
        }
    }
}

void checkFTPTimeout()
{
    if (!Agora.ftpEnabled)
        return;
    if (fileSender.bytesRemaining && millis() - fileSender.lastMessage > ESPNOW_FILESHARE_TIMEOUT)
    {
        AGORA_LOG_E("File Sender Timeout, aborting file transfer");
        sendMessage(fileSender.receiverMac, AGORA_MESSAGE_FTP_ABORT);

        if (fileSender.nextReceiver)
        {

            AGORA_LOG_V("Sending file to next friend");
            memcpy(fileSender.receiverMac, Agora.friends[fileSender.nextReceiver].mac, 6);
            fileSender.nextReceiver = Agora.friendCount > fileSender.nextReceiver + 1 ? fileSender.nextReceiver + 1 : 0;
            agora_ftp_send_header();
        }

        else
        {
            AGORA_LOG_E("No more receivers, resetting file share info");
            resetFileShareInfo();
        }
    }

    if (fileReceiver.bytesRemaining && millis() - fileReceiver.lastMessage > ESPNOW_FILESHARE_TIMEOUT)
    {
        AGORA_LOG_E("File Receiver Timeout, aborting file transfer");
        sendMessage(fileReceiver.senderMac, AGORA_MESSAGE_FTP_ABORT);
        resetFileShareInfo();
    }
}
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
// Agora Task
void agoraTask(void *)
{
    AGORA_LOG_V("\n\n---------------------------\nStarting Agora Task\n\n");

    while (1)
    {
        int count = 0;
        long lastMessage = 0;
        for (int i = 0; i < Agora.friendCount; i++)
        {
            AgoraFriend f = Agora.friends[i];
            lastMessage = lastMessage < f.lastMessageReceived ? f.lastMessageReceived : lastMessage;

            if (f.meToThem == GURU)
            {
                if (millis() - f.lastMessageSent > Agora.pingInterval)
                {
                    sendMessage(&f, AGORA_MESSAGE_PING);
                }
                if (millis() - f.lastMessageReceived < 2 * Agora.pingInterval)
                {
                    count++;
                }
            }
            else if (f.meToThem == FOLLOWER)
            {
                if (millis() - f.lastMessageReceived < 2 * Agora.pingInterval)
                {
                    count++;
                }
            }
        }
        Agora.activeConnectionCount = count;

        if (Agora.timeout)
        {
            if (millis() - lastMessage > Agora.timeout)
            {
                AGORA_LOG_E("\nNo active connections after %u seconds of trying.\nQuit the Agora\n\n", millis() / 1000);
                Agora.end();
            }
        }
        checkFTPTimeout();
        lookForNewGurus();
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

//----------------------------------------------------------------------------------------

void resetFileShareInfo()
{
    memset(&fileshareHeader, 0, sizeof(fileshareHeader));

    memset(fileSender.receiverMac, 0, 6);
    fileSender.bytesRemaining = 0;
    fileSender.startTime = 0;
    fileSender.lastMessage = 0;
    if (fileSender.file)
    {
        fileSender.file.close();
    }

    fileReceiver.startTime = 0;
    fileReceiver.lastMessage = 0;
    memset(fileReceiver.senderMac, 0, 6);
    fileReceiver.bytesRemaining = 0;
    if (fileReceiver.file)
    {
        fileReceiver.file.close();
    }
}

//----------------------------------------------------------------------------------------

void agora_ftp_send_header()
{
    if (!fileSender.file)
    {
        Serial.println("No file to share ???");
        return;
    }
    fileSender.file.seek(0, SeekSet); // rewind the file to the beginning
    strcpy(fileshareHeader.magicword, "lookatthis");
    strcpy(fileshareHeader.filename, fileSender.file.name());
    fileshareHeader.filesize = fileSender.file.size();
    esp_err_t result = esp_now_send(fileSender.receiverMac, (uint8_t *)&fileshareHeader, sizeof(fileshareHeader));
    fileSender.startTime = millis();
    if (result == ESP_OK)
    {
        Serial.println("Header sent with success");
        fileSender.bytesRemaining = fileSender.file.size();
        Serial.printf("Send %d bytes\n", fileSender.bytesRemaining);
    }
    else
    {
        Serial.println("Error sending the data");
        fileSender.bytesRemaining = 0;
    }
}

//----------------------------------------------------------------------------------------

void agora_ftp_send_chunk()
{
    if (!fileSender.file)
    {
        AGORA_LOG_E("File is gone?");
        fileSender.bytesRemaining = 0;
        /*  for (int i = 0; i < Agora.friendCount; i++)
         { */
        sendMessage(fileSender.receiverMac, AGORA_MESSAGE_FTP_ABORT);
        /*   }
          return; */
    }

    if (fileSender.bytesRemaining)
    {
        char buffer[ESPNOW_FILESHARE_CHUNK_SIZE];

        int bytesToSendNow = fileSender.bytesRemaining > ESPNOW_FILESHARE_CHUNK_SIZE ? ESPNOW_FILESHARE_CHUNK_SIZE : fileSender.bytesRemaining;
        fileSender.file.readBytes(buffer, bytesToSendNow);
        esp_err_t result = esp_now_send(fileSender.receiverMac, (uint8_t *)buffer, bytesToSendNow);

        if (result == ESP_OK)
        {
            fileSender.bytesRemaining -= bytesToSendNow;
        }
        else
        {
            Serial.println("\n\nError sending the data");
            fileSender.bytesRemaining = 0;
            fileSender.file.close();
            sendMessage(fileSender.receiverMac, AGORA_MESSAGE_FTP_ABORT);
        }
    }
    if (fileSender.file)
    {
        Agora.ftpCallback(fileSender.receiverMac, fileSender.ftpStatus, fileSender.file.name(), fileSender.file.size(), fileSender.bytesRemaining);
    }
    else
    {
        Agora.ftpCallback(fileSender.receiverMac, fileSender.ftpStatus, "No file", 0, fileSender.bytesRemaining);
    }

    if (fileSender.bytesRemaining == 0)
    {
        Serial.println("\n\nDONE sending the data");
        if (fileSender.nextReceiver)
        {

            AGORA_LOG_V("Sending file to next friend");
            memcpy(fileSender.receiverMac, Agora.friends[fileSender.nextReceiver].mac, 6);
            fileSender.nextReceiver = Agora.friendCount > fileSender.nextReceiver + 1 ? fileSender.nextReceiver + 1 : 0;
            agora_ftp_send_header();
        }
        else
        {
            resetFileShareInfo();
        }
    }
}

//----------------------------------------------------------------------------------------

bool handle_agora_ftp(const uint8_t *macAddr, const uint8_t *incomingData, int len)
{

    if (isMessage(incomingData, len, AGORA_MESSAGE_FTP_ABORT))
    {
        AGORA_LOG_E("File transfer aborted;");
        resetFileShareInfo();
        return true;
    }

    if (len == 11)
    {
        if (strcmp((const char *)incomingData, "gimmemore!") == 0)
        {
            // AGORA_LOG_V("Let's send the file then. %d bytes to go\n", fileSender.bytesRemaining);
            fileSender.lastMessage = millis();
            agora_ftp_send_chunk();
            return true;
        }
    }

    if (fileReceiver.bytesRemaining)
    {
        if (len != ESPNOW_FILESHARE_CHUNK_SIZE && len != fileReceiver.bytesRemaining)
        {
            AGORA_LOG_E("Weird number of bytes received: %d.", len);
            AGORA_LOG_E("Should be %d or %d\n", fileReceiver.bytesRemaining, ESPNOW_FILESHARE_CHUNK_SIZE);
            fileReceiver.ftpStatus = ERROR;
            return false;
        }
        else
        {

            // write to the file
            fileReceiver.lastMessage = millis();
            if (fileReceiver.file.write(incomingData, len) == len)
            {
                fileReceiver.bytesRemaining -= len;
                if (fileReceiver.bytesRemaining > 0)
                {
                    const uint8_t mess[] = "gimmemore!";
                    fileReceiver.ftpStatus = RUNNING;
                    esp_err_t result = esp_now_send(macAddr, mess, sizeof(mess));
                }
                else
                {
                    if (fileReceiver.bytesRemaining < 0)
                    {
                        AGORA_LOG_E("TOO MANY BYTES RECEIVED ????");
                    }
                    fileReceiver.file.close();
                    delay(200);
                    /* char filepath[100];
                   sprintf(filepath, "/%s", fileshareHeader.filename);
                                      File file = SD.open(filepath, "r");
                                       size_t writtensize = file.size();
                                       file.close();
                                       if (writtensize != fileshareHeader.filesize)
                                       {
                                           Serial.printf("\nERROR: Written File size does not match expecttions %d written vs %d\n", writtensize, fileshareHeader.filesize);
                                       }
                                       else
                                       {
                    */
                    long timetaken = millis() - fileReceiver.startTime;
                    fileReceiver.ftpStatus = DONE;
                    AGORA_LOG_V("SUCCESS: File written %d bytes in %d.%d seconds (%d kbit/s)", fileshareHeader.filesize, timetaken / 1000, timetaken % 1000, fileshareHeader.filesize * 8000 / timetaken / 1024);
                    sendMessage(macAddr, AGORA_MESSAGE_FTP_DONE);
                    //}
                }
            }
            else
            {

                AGORA_LOG_E("Error writing file");
                fileReceiver.ftpStatus = ERROR;
                resetFileShareInfo();
                sendMessage(macAddr, AGORA_MESSAGE_FTP_ABORT);
            }

            if (fileReceiver.file)
            {

                Agora.ftpCallback(fileReceiver.senderMac, fileReceiver.ftpStatus, fileReceiver.file.name(), fileshareHeader.filesize, fileReceiver.bytesRemaining);
            }
            else
            {
                Agora.ftpCallback(fileReceiver.senderMac, fileReceiver.ftpStatus, "No file", 0, fileReceiver.bytesRemaining);
            }

            return true;
        }
    }
    else if (len == sizeof(agoraFileshareHeader_t))
    {
        memcpy(&fileshareHeader, incomingData, len);
        if (strcmp(fileshareHeader.magicword, "lookatthis") == 0)
        {
            Serial.printf("Receive File %s of size %d\n", fileshareHeader.filename, fileshareHeader.filesize);
            fileReceiver.startTime = millis();
            fileReceiver.bytesRemaining = fileshareHeader.filesize;
            memcpy(fileReceiver.senderMac, macAddr, 6);

            char filepath[100];
            sprintf(filepath, "/%s", fileshareHeader.filename);
            if (Fileshare_Filesystem.exists(filepath))
            {
                Serial.printf("File %s already exists. DELETING FILE.\n", filepath);
                Fileshare_Filesystem.remove(filepath);
            }

            fileReceiver.file = Fileshare_Filesystem.open(filepath, "w");
            if (!fileReceiver.file)
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

void TheAgora::share(const char *name, const char *path)
{
    if (!ftpEnabled)
        return;

    AgoraFriend *f = friendNamed(name);
    if (!f)
    {
        AGORA_LOG_E("I don't have a friend called %s", name);
        return;
    }
    memcpy(fileSender.receiverMac, f->mac, 6);
    fileSender.file = Fileshare_Filesystem.open(path);
    fileSender.nextReceiver = 0;
    if (!fileSender.file)
    {
        AGORA_LOG_E("Cannot open file at %s", path);
        return;
    }
    agora_ftp_send_header();
}

void TheAgora::share(const char *path)
{
    if (!ftpEnabled)
        return;

    if (!Agora.friendCount)
    {
        AGORA_LOG_E("I don't have any friends to whom I could send this file");
        return;
    }
    memcpy(fileSender.receiverMac, Agora.friends[0].mac, 6);
    if (fileSender.file)
    {
        fileSender.file.close();
    }
    fileSender.file = Fileshare_Filesystem.open(path);
    fileSender.nextReceiver = Agora.friendCount > 1 ? 1 : 0;
    if (!fileSender.file)
    {
        AGORA_LOG_E("Cannot open file at %s", path);
        return;
    }
    agora_ftp_send_header();
}