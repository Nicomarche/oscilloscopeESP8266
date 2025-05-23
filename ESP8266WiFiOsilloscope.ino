/*
 * WiFi Osiolloscope+Network volt-meter firmware for ESP8266b
 * Rev 1.2
 *
 * Main File
 *
 * Author: M. Mahdi K. Kanan (mvtdesign@gmail.com)
 * WiCardTech Engineering Group
 *
 * https://store.wicard.net
 */

#include <time.h>
#include <Arduino.h>
#include "EEPROM.h"
#include <WebSocketsServer.h>

#include "AC.h"

#include "webapp.h"

#include "user_global.h"

#define DEFAULT_CMD_DELAY 30 // 100mS unit
#define DEFAULT_CMD_DELAY_STRING "030"

// eeprom addr for user
#define EEDATA_SIGNATURE 0xAC
#define EEDATA_SIGNATURE_ADDR 254
#define EEDATA_SYSTEM_CONFIG 255
#define EEDATA_BASE_ADDR 256

// Network functionality removed in simplified version
unsigned char ucSystemMode;
#define SYSTEMMODE_NET_LINKED 0x01
#define SYSTEMMODE_NET_JOINED 0x02
#define SYSTEMMODE_TIME_SYNC 0x04
#define SYSTEMMODE_GLOBAL_CMD 0x08
#define SYSTEMMODE_SAVE_DATA 0x10

unsigned char ucReTry;
#define CMD_RETRY 15

unsigned int uiAppData[8];
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket server on port 81

unsigned long ulTimediff;
unsigned long ulA;
unsigned long ulB;
unsigned long ulSystemTimer;
unsigned long ulSystemTimerZero;
bool bSystemTimerMinus;

unsigned long ulCmdMillis;
unsigned long ulSystemResetTimer;
#define RESET_TIMER_CYCLE 500000
#define RESET_TIMER_ME_CYCLE 600000

// WebSocket event handler
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // Send connection confirmation
    webSocket.sendTXT(num, "{\"type\":\"connected\",\"message\":\"WebSocket ready for oscilloscope data\"}");
    break;
  }

  case WStype_TEXT:
    Serial.printf("[%u] Received Text: %s\n", num, payload);
    handleWebSocketMessage(num, (char *)payload);
    break;

  case WStype_BIN:
    Serial.printf("[%u] Received binary length: %u\n", num, length);
    break;

  default:
    break;
  }
}

// Handle WebSocket messages
void handleWebSocketMessage(uint8_t num, char *message)
{
  Serial.printf("Received WebSocket message: %s\n", message);

  // Parse JSON message - simple string parsing for efficiency
  if (strstr(message, "\"cmd\":\"getData\"") != NULL)
  {
    // Extract parameters from JSON
    char *modePtr = strstr(message, "\"mode\":");
    char *speedPtr = strstr(message, "\"speed\":");
    char *triggerPtr = strstr(message, "\"trigger\":");

    if (modePtr && speedPtr && triggerPtr)
    {
      // Parse mode (0 or 1)
      modePtr += 7; // Skip "mode":
      while (*modePtr == ' ')
        modePtr++; // Skip spaces
      uiAppData[0] = (*modePtr == '1') ? 1 : 0;

      // Parse speed
      speedPtr += 8; // Skip "speed":
      while (*speedPtr == ' ')
        speedPtr++; // Skip spaces
      uiAppData[1] = atoi(speedPtr);

      // Parse trigger
      triggerPtr += 10; // Skip "trigger":
      while (*triggerPtr == ' ')
        triggerPtr++; // Skip spaces
      uiAppData[2] = atoi(triggerPtr);

      Serial.printf("Parsed - Mode: %d, Speed: %d, Trigger: %d\n",
                    uiAppData[0], uiAppData[1], uiAppData[2]);

      // Call the oscilloscope data acquisition function
      userMainWebSocket(num);
    }
    else
    {
      Serial.println("Failed to parse WebSocket message parameters");
    }
  }
}

void setup()
{ // ESP.wdtDisable();

  // Forzar modo AP solamente (sin intentar conectarse a otras redes WiFi)
  WiFi.mode(WIFI_AP);

  Serial.begin(115200);
  Serial.setDebugOutput(false);

  delay(50);

#ifdef LOG_ENABLE
  Serial.println(PSTR(" "));
  Serial.println(PSTR("** WiCardTech **"));
  Serial.println(PSTR("** WiFi Osiolloscope+Network volt-meter Firmware For ESP8266 (rev 1.2) **"));
  Serial.println(PSTR("** https://store.wicard.net **"));
  Serial.println(PSTR("** Funcionando solo en modo punto de acceso (sin conexión a redes WiFi) **"));
#endif

  systemInit();

  // Establecer un SSID vacío en la configuración para evitar conexiones automáticas
  STSsid[0] = 0;
  EEPROM.write(EEP_ST_SSID, 0);
  EEPROM.commit();

  init_AC();
  // Configuring Internal Page
  if (secureLink[0] == 0)
  {
    Server.on("/", rootPage);
  }
  else
  {
    Server.on("/" + ((String)secureLink), rootPage);
    Server.on("/" + ((String)secureLink) + "/", rootPage);
  }
  http.setTimeout(5000);

  // Initialize WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  delay(50);
  Server.begin();
  delay(50);

  userInit();
}

void SystemDelay(unsigned int i)
{
  unsigned long ulUserTimer = millis();

  while ((millis() - ulUserTimer) < i)
  {
    Server.handleClient();
  };
}

void delayHandleClient(unsigned int i)
{
  unsigned long ulUserTimer = millis();

  while ((millis() - ulUserTimer) < i)
  {
    Server.handleClient();
  };
}

void loop()
{
  // Handle WebSocket events
  webSocket.loop();

  // Simplified loop function focused only on handling client requests
  // Network functionality has been removed in the simplified project

  if (ucSystemMode & SYSTEMMODE_SAVE_DATA)
  {
    EEPROM.commit();
    ucSystemMode &= 0xFF ^ SYSTEMMODE_SAVE_DATA;
  }

  delayHandleClient(1000);
}
