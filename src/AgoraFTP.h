
#ifndef __Agora_FTP_INCLUDED__
#define __Agora_FTP_INCLUDED__

#include <esp_now.h>
#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

#define ESPNOW_FILESHARE_CHUNK_SIZE 240

typedef struct
{
    char magicword[11];
    int filesize;
    char filename[48];
} esp_fileshare_header_t;

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
                    Serial.printf("SUCCESS: File written %d bytes in %d.%d seconds (%d kbit/s)", esp_fileshare_header.filesize, timetaken / 1000, timetaken % 1000, esp_fileshare_header.filesize * 8000 / timetaken);
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

    return false;       // this was not a FTP message
}
#endif