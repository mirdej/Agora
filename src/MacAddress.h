#ifndef __AGORA_MACADDRESS_INCLUDED__
#define __AGORA_MACADDRESS_INCLUDED__
#include "Arduino.h"
/* #include <esp_wifi.h>
 */
/*

Arduino Library to simplify ESP-NOW
© 2024 by Michael Egger AT anyma.ch
MIT License


*/

namespace Agora
{
    struct MAC_Address
    {
    public:
        MAC_Address(uint8_t *addr);
        MAC_Address(const uint8_t *addr);
        MAC_Address(uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0, uint8_t e = 0, uint8_t f = 0);

        uint8_t mac[6];

        void setLocal(void);
        void set(uint8_t *addr);
        void set(uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0, uint8_t e = 0, uint8_t f = 0);

        String toString(void);
        String toArrayString(void);

        bool operator==(MAC_Address rhs) const
        {
            for (int i = 0; i < 6; i++)
            {
                if (rhs.mac[i] != mac[i])
                    return false;
            }
            return true;
        }

        bool operator!=(MAC_Address rhs) const
        {
            for (int i = 0; i < 6; i++)
            {
                if (rhs.mac[i] != mac[i])
                    return true;
            }
            return false;
        }
    };

    // MAC_Address::MAC_Address(){ set();}
    MAC_Address::MAC_Address(uint8_t *addr) { set(addr); }
    MAC_Address::MAC_Address(const uint8_t *addr) { set((uint8_t *)addr); }
    MAC_Address::MAC_Address(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
    {
        set(a, b, c, d, e, f);
    }

    void MAC_Address::setLocal(void)
    {
/*         esp_wifi_get_mac((wifi_interface_t)ESP_IF_WIFI_STA, mac);
 */    }

    void MAC_Address::set(uint8_t *addr)
    {
        memcpy(mac, addr, 6);
    }

    void MAC_Address::set(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f)
    {
        mac[0] = a;
        mac[1] = b;
        mac[2] = c;
        mac[3] = d;
        mac[4] = e;
        mac[5] = f;
    }

    String MAC_Address::toString(void)
    {
        char macStr[18] = {0};
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(macStr);
    }

    String MAC_Address::toArrayString(void)
    {
        char macStr[50] = {0};
        sprintf(macStr, "{0x%02X, 0x%02X ,0x%02X, 0x%02X, 0x%02X ,0x%02X}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(macStr);
    }
}

#endif