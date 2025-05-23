/*
 * WiFi Osiolloscope+Network volt-meter firmware for ESP8266
 * Rev 1.2
 *
 * WiFi Configuration Header File
 *
 * Author: M. Mahdi K. Kanan (mvtdesign@gmail.com)
 * WiCardTech Engineering Group
 *
 * https://store.wicard.net
 */

#ifndef ac_H_
#define ac_H_

#define LOG_ENABLE
#define MEM_SAVED 0xAB

// 0-161 is allocated for AutoConnect (162 bytes)
// 162-255 is free for the program (94 bytes)
#define EEP_AC_KEY 0
#define EEP_ST_SSID 1
#define EEP_ST_PASS 33
#define EEP_AP_SSID 65
#define EEP_AP_PASS 97
#define EEP_SECURE 129
#define EEP_AC_APST_CONFIG 161
unsigned char ucSystemAPSTConfig;
#define EEP_AC_AP_CONFIG_HIDDEN 0x01
#define EEP_AC_AP_CONFIG_TEMP 0x02
#define EEP_AC_AP_CONFIG_ODD_IP 0x04
#define EEP_AC_ST_JOIN_REQUIRED 0x10

#define EEP_CONF_CMD_DELAY 162
unsigned char ucSystemCmdDelay;

#define EEP_ST_SSID_LEN 32
#define EEP_ST_PASS_LEN 32
#define EEP_AP_SSID_LEN 32
#define EEP_AP_PASS_LEN 32
#define EEP_SECURE_LEN 32

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsServer.h>

#include "EEPROM.h"

ESP8266WebServer Server;
extern WebSocketsServer webSocket;

int httpCode;

HTTPClient http;

WiFiClient client;

IPAddress apIP;
IPAddress linkedIP;

char cApIP[16];     // IP of the AP
char cLinkedIP[16]; // IP received from linked ST

char STSsid[EEP_ST_SSID_LEN];     // 1-32
char STPassword[EEP_ST_PASS_LEN]; // 33-64
char APSsid[EEP_AP_SSID_LEN];     // 65-96
char APPassword[EEP_AP_PASS_LEN]; // 97-128
char secureLink[EEP_SECURE_LEN];  // 129-160

#endif // INCLUDED
