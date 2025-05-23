/*
 * WiFi Oscilloscope firmware for ESP8266
 * Simplified version
 */

void userGetSettings()
{
  // Fill the uiAppData variables with the settings
  uiAppData[0] = EEPROM.read(EEDATA_NVM_CH_ADDR);
}

void userSetSettings()
{
  // Update settings from uiAppData
  EEPROM.write(EEDATA_NVM_CH_ADDR, (uiAppData[0] & 0xFF));
  ucNVMCh = uiAppData[0] & 0xFF;
  EEPROM.commit();
}
