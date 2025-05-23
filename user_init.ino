/*
 * WiFi Oscilloscope firmware for ESP8266
 * Simplified version
 */

void userInit()
{
  // Config pins for oscilloscope functionality
  pinMode(DIGITAL_CH_PIN, INPUT);
  pinMode(ON_CHIP_LED_PIN, OUTPUT);

  delay(100);

  pinMode(ANALOG_CH_PIN, INPUT);
  //pinMode(ANALOG_CH_PIN, ANALOG);

  delay(100);

  // Initialize ADC
  analogRead(ANALOG_CH_PIN);
  analogRead(ANALOG_CH_PIN);
  analogRead(ANALOG_CH_PIN);

  delay(100);

  analogRead(ANALOG_CH_PIN);
  analogRead(ANALOG_CH_PIN);
  analogRead(ANALOG_CH_PIN);

  system_adc_read();
  
  // Initialize EEPROM settings
  if(EEPROM.read(EEDATA_SIGNATURE_ADDR) != EEDATA_SIGNATURE) //for the first initial load
  {
    EEPROM.write(EEDATA_SIGNATURE_ADDR, EEDATA_SIGNATURE); //next time will not set to default
    EEPROM.write(EEDATA_NVM_CH_ADDR, 0);
    EEPROM.commit();
  }
  else //the default will be loaded instead
  {
    ucNVMCh = EEPROM.read(EEDATA_NVM_CH_ADDR);
  }
  
  digitalWrite(ON_CHIP_LED_PIN, LOW);
}
