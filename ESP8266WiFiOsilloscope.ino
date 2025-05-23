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

  // Requirement #6: Basic structure check
  size_t len = strlen(message);
  if (len < 2 || message[0] != '{' || message[len - 1] != '}') {
    Serial.println("Invalid JSON structure: not an object");
    webSocket.sendTXT(num, "{\"error\":\"Invalid JSON structure: not an object\"}");
    return;
  }

  // Requirement #1: Command Check - Ensure the command is exactly "cmd":"getData"
  char *cmdToken = "\"cmd\":\"getData\"";
  char *cmdPtr = strstr(message, cmdToken);

  if (cmdPtr != NULL)
  {
    // Check if "getData" is followed by a comma or closing brace, to ensure exact match
    char* afterCmd = cmdPtr + strlen(cmdToken);
    while(*afterCmd == ' ' || *afterCmd == '\t' || *afterCmd == '\n' || *afterCmd == '\r') afterCmd++; // skip whitespace

    if (*afterCmd != ',' && *afterCmd != '}') {
        Serial.println("Malformed getData command: additional characters after command value");
        webSocket.sendTXT(num, "{\"error\":\"Malformed getData command\"}");
        return;
    }

    // Extract parameters from JSON
    char *modePtr = strstr(message, "\"mode\":");
    char *speedPtr = strstr(message, "\"speed\":");
    char *triggerPtr = strstr(message, "\"trigger\":");

    if (modePtr && speedPtr && triggerPtr)
    {
      long parsedMode, parsedSpeed, parsedTrigger;
      char *endptr;

      // Requirement #2: Mode Parsing and Validation
      modePtr += strlen("\"mode\":"); // Skip "\"mode\":"
      while (*modePtr == ' ' || *modePtr == '\t' || *modePtr == '\n' || *modePtr == '\r') modePtr++; // Skip whitespace
      
      if (*modePtr == '0') {
        parsedMode = 0;
        modePtr++; // Consume the digit
      } else if (*modePtr == '1') {
        parsedMode = 1;
        modePtr++; // Consume the digit
      } else {
        Serial.println("Invalid mode value: must be '0' or '1'");
        webSocket.sendTXT(num, "{\"error\":\"Invalid mode value\"}");
        return;
      }
      // Check for trailing characters for mode
      char* tempModePtr = modePtr;
      while(*tempModePtr == ' ' || *tempModePtr == '\t' || *tempModePtr == '\n' || *tempModePtr == '\r') tempModePtr++;
      if(*tempModePtr != ',' && *tempModePtr != '}') {
          Serial.println("Invalid mode value: trailing characters after mode");
          webSocket.sendTXT(num, "{\"error\":\"Invalid mode value: trailing characters\"}");
          return;
      }

      // Requirement #3: Integer Parameter Parsing (Speed)
      speedPtr += strlen("\"speed\":"); // Skip "\"speed\":"
      while (*speedPtr == ' ' || *speedPtr == '\t' || *speedPtr == '\n' || *speedPtr == '\r') speedPtr++; // Skip whitespace
      parsedSpeed = strtol(speedPtr, &endptr, 10);

      if (endptr == speedPtr) {
        Serial.println("Invalid speed value: no digits found");
        webSocket.sendTXT(num, "{\"error\":\"Invalid speed value: no digits found\"}");
        return;
      }
      char* tempSpeedPtr = endptr;
      while(*tempSpeedPtr == ' ' || *tempSpeedPtr == '\t' || *tempSpeedPtr == '\n' || *tempSpeedPtr == '\r') tempSpeedPtr++;
      if (*tempSpeedPtr != '\0' && *tempSpeedPtr != ',' && *tempSpeedPtr != '}') {
        Serial.println("Invalid speed value: non-numeric characters or malformed");
        webSocket.sendTXT(num, "{\"error\":\"Invalid speed value: non-numeric or malformed\"}");
        return;
      }

      // Requirement #3: Integer Parameter Parsing (Trigger)
      triggerPtr += strlen("\"trigger\":"); // Skip "\"trigger\":"
      while (*triggerPtr == ' ' || *triggerPtr == '\t' || *triggerPtr == '\n' || *triggerPtr == '\r') triggerPtr++; // Skip whitespace
      parsedTrigger = strtol(triggerPtr, &endptr, 10);

      if (endptr == triggerPtr) {
        Serial.println("Invalid trigger value: no digits found");
        webSocket.sendTXT(num, "{\"error\":\"Invalid trigger value: no digits found\"}");
        return;
      }
      char* tempTriggerPtr = endptr;
      while(*tempTriggerPtr == ' ' || *tempTriggerPtr == '\t' || *tempTriggerPtr == '\n' || *tempTriggerPtr == '\r') tempTriggerPtr++;
      if (*tempTriggerPtr != '\0' && *tempTriggerPtr != ',' && *tempTriggerPtr != '}') {
        Serial.println("Invalid trigger value: non-numeric characters or malformed");
        webSocket.sendTXT(num, "{\"error\":\"Invalid trigger value: non-numeric or malformed\"}");
        return;
      }

      // Requirement #4: Value Range Checks
      // Mode is already validated to be 0 or 1 by parsing logic.
      if (parsedSpeed < 0 || parsedSpeed > 2) {
        Serial.printf("Parsed speed out of range: %ld\n", parsedSpeed);
        webSocket.sendTXT(num, "{\"error\":\"Speed value out of range\"}");
        return;
      }
      if (parsedTrigger < 0 || parsedTrigger > 2) {
        Serial.printf("Parsed trigger out of range: %ld\n", parsedTrigger);
        webSocket.sendTXT(num, "{\"error\":\"Trigger value out of range\"}");
        return;
      }

      // All checks passed, populate uiAppData
      uiAppData[0] = parsedMode;
      uiAppData[1] = parsedSpeed;
      uiAppData[2] = parsedTrigger;

      Serial.printf("Parsed - Mode: %d, Speed: %d, Trigger: %d\n",
                    uiAppData[0], uiAppData[1], uiAppData[2]);

      userMainWebSocket(num);
    }
    else
    {
      Serial.println("Failed to parse WebSocket message parameters: missing fields mode, speed, or trigger");
      webSocket.sendTXT(num, "{\"error\":\"Invalid JSON structure: missing fields\"}");
    }
  }
  else // cmdPtr was NULL or other issue with command
  {
    Serial.println("Unknown WebSocket command or malformed JSON");
    webSocket.sendTXT(num, "{\"error\":\"Unknown command or malformed JSON\"}");
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
