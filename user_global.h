/*
 * WiFi Osiolloscope+Network volt-meter firmware for ESP8266
 * Rev 1.2
 *
 * Firmware Header File
 *
 * Author: M. Mahdi K. Kanan (mvtdesign@gmail.com)
 * WiCardTech Engineering Group
 *
 * https://store.wicard.net
 */

#ifndef user_H_
#define user_H_

// Define your global variables here

#define ON_CHIP_LED_PIN 2
#define DIGITAL_CH_PIN 5
#define ANALOG_CH_PIN A0
#define ADC_R_THR 20
#define ADC_F_THR 20

#define EEDATA_NVM_CH_ADDR EEDATA_BASE_ADDR

unsigned char ucNVMCh;

// Function declarations
void userMainWebSocket(uint8_t clientNum);

// --------------------------------

#endif // INCLUDED
