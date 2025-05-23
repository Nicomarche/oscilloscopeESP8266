/*
 * WiFi Oscilloscope firmware for ESP8266
 * Simplified version
 */

void rootPage()
{
  if (!AC_config())
  {
    if (Server.arg("m") != NULL)
    {
      if (Server.arg("r0") != NULL)
      {
        uiAppData[0] = toUInt(Server.arg("r0"));
        uiAppData[1] = toUInt(Server.arg("r1"));
        uiAppData[2] = toUInt(Server.arg("r2"));
        uiAppData[3] = toUInt(Server.arg("r3"));
      }

      // WebSocket handles oscilloscope data now - HTTP requests deprecated
      yield();
      // Old oscilloscope HTTP handling removed - now using WebSocket
      Server.send(200, "text/plain", "Use WebSocket for oscilloscope data");
    }
    else
    {
      webapp();
    }
  }

  ulSystemResetTimer = millis() + RESET_TIMER_CYCLE;
}
