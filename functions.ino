/*
 * WiFi Osiolloscope+Network volt-meter firmware for ESP8266
 * Rev 1.2
 *
 * Global Functions File
 *
 * Author: M. Mahdi K. Kanan (mvtdesign@gmail.com)
 * WiCardTech Engineering Group
 *
 * https://store.wicard.net
 */

void systemInit()
{
  int i;

  for (i = 0; i < 8; i++)
    uiAppData[i] = 0;

  delay(50);

  // 0-161 is allocated for AutoConnect (162 bytes)
  // 162-255 is reserved (94 bytes)
  // 256-511 is free for the user program (256 bytes)
  EEPROM.begin(512);

  delay(50);

  ucSystemMode = 0;

  ulSystemTimerZero = 0;
  bSystemTimerMinus = false;
  ulSystemResetTimer = millis() + RESET_TIMER_CYCLE;

  if ((ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED) == 0)
    ucSystemMode |= SYSTEMMODE_TIME_SYNC;

  ulCmdMillis = 0;
}

// Function removed in simplified project
bool sendGlobalCmd_me()
{
  // This function is kept for compatibility but does nothing in simplified project
  return true;
}

void getTime()
{
  ulSystemTimer = millis();

  if (bSystemTimerMinus)
    ulSystemTimer -= ulSystemTimerZero;
  else
    ulSystemTimer += ulSystemTimerZero;
}

// Function removed in simplified project
bool sendTimeSynchCmd()
{
  // This function is kept for compatibility but does nothing in simplified project
  return true;
}

String fwdbwdCmd()
{
  String s = "";

  if (secureLink[0] == 0)
    s = "/?m=" + Server.arg("m");
  else
    s = "/" + ((String)secureLink) + "/?m=" + Server.arg("m");

  s += "&r0=" + String(uiAppData[0]);
  s += "&r1=" + String(uiAppData[1]);
  s += "&r2=" + String(uiAppData[2]);
  s += "&r3=" + String(uiAppData[3]);
  s += "&r4=" + String(uiAppData[4]);
  s += "&r5=" + String(uiAppData[5]);
  s += "&r6=" + String(uiAppData[6]);
  s += "&r7=" + String(uiAppData[7]);

  if (Server.arg("i") == NULL || Server.arg("i") == "")
  {
    unsigned char c = Server.client().remoteIP()[2];

    if ((ucSystemMode & SYSTEMMODE_GLOBAL_CMD) == 0)
    {
      s = "000";

      goto _Make_resp_str;
    }
    else if (ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED && ((c == 4 && ((ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP) == 0)) || (c == 5 && ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP))) // BWD
    {
      ucSystemMode &= 0xFF ^ SYSTEMMODE_GLOBAL_CMD;

      if (Server.arg("t") != NULL)
        ulCmdMillis = toULong(Server.arg("t"));
      else
      {
        getTime();

        ulCmdMillis = ulSystemTimer;
      }

      if (ucSystemMode & SYSTEMMODE_NET_JOINED)
      {
        s += "&t=" + String(ulCmdMillis);

        for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
        {
          http.begin(client, cApIP, 80, s);
          httpCode = http.GET();

          if (httpCode == 200)
          {
            s = http.getString();
            http.end();
            ulCmdMillis += toULong(s.substring(0, 3)) * 100;
            ulCmdMillis += ucSystemCmdDelay * 100;

            return s;
          }

          http.end();
          delay(1);
        }
      }

      ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_JOINED;

      if (Server.arg("t") == NULL)
      {
        s = "";
        if (ucSystemCmdDelay < 100)
        {
          s += "0";
          if (ucSystemCmdDelay < 10)
            s += "0";
        }
        s += String(ucSystemCmdDelay);
      }
      else
      {
        s = DEFAULT_CMD_DELAY_STRING;

        ulCmdMillis += DEFAULT_CMD_DELAY * 100;
      }

      ulCmdMillis += ucSystemCmdDelay * 100;

      goto _Make_resp_str;
    }
    else if ((ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED) == 0 && Server.arg("t") != NULL)
    {
      ucSystemMode &= 0xFF ^ SYSTEMMODE_GLOBAL_CMD;

      s = "";
      if (ucSystemCmdDelay < 100)
      {
        s += "0";
        if (ucSystemCmdDelay < 10)
          s += "0";
      }
      s += String(ucSystemCmdDelay);

      ulCmdMillis = toULong(Server.arg("t"));
      ulCmdMillis += ucSystemCmdDelay * 100;

      goto _Make_resp_str;
    }
    else // FWD-Global
    {
      ucSystemMode &= 0xFF ^ SYSTEMMODE_GLOBAL_CMD;

      if (Server.arg("q") != NULL) // >= WiCard #2
        ulCmdMillis = toULong(Server.arg("q"));
      else // Only WiCard #1
      {
        getTime();

        ulCmdMillis = ulSystemTimer;
        ulCmdMillis += ucSystemCmdDelay * 100;
      }

      if (ucSystemMode & SYSTEMMODE_NET_LINKED && ((c != 4 && (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP) == 0) || (c != 5 && ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP) || Server.client().remoteIP()[3] != linkedIP[3]))
      {
        s += "&q=" + String(ulCmdMillis);

        for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
        {
          http.begin(client, cLinkedIP, 80, s);
          httpCode = http.GET();

          if (httpCode == 200)
          {
            if (Server.arg("q") == NULL)
            {
              s = "";
              if (ucSystemCmdDelay < 100)
              {
                s += "0";
                if (ucSystemCmdDelay < 10)
                  s += "0";
              }
              s += String(ucSystemCmdDelay);
              s += http.getString();
            }
            else
            {
              s = http.getString();
              ulCmdMillis += (ucSystemCmdDelay * 100);
            }

            http.end();

            return s;
          }

          http.end();
          delay(1);
        }

        ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_LINKED;
      }

      s = "";
      if (Server.arg("q") == NULL)
      {
        if (ucSystemCmdDelay < 100)
        {
          s += "0";
          if (ucSystemCmdDelay < 10)
            s += "0";
        }
        s += String(ucSystemCmdDelay);
      }

      goto _Make_resp_str;
    }
  }
  else if (Server.arg("i").equals(APSsid) == false)
  {
    ucSystemMode &= 0xFF ^ SYSTEMMODE_GLOBAL_CMD;

    unsigned char c = Server.client().remoteIP()[2];

    s += "&i=" + Server.arg("i");

    if (ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED && ((c == 4 && ((ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP) == 0)) || (c == 5 && ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP))) // BWD
    {
      if (ucSystemMode & SYSTEMMODE_NET_JOINED)
      {
        for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
        {
          http.begin(client, cApIP, 80, s);

          httpCode = http.GET();

          if (httpCode == 200)
          {
            s = http.getString();
            http.end();

            return s;
          }

          http.end();
          delay(1);
        }

        ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_JOINED;
      }

      s = "";
    }
    else // FWD
    {
      if (ucSystemMode & SYSTEMMODE_NET_LINKED && Server.client().remoteIP()[3] != linkedIP[3])
      {
        for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
        {
          http.begin(client, cLinkedIP, 80, s);

          httpCode = http.GET();

          if (httpCode == 200)
          {
            s = http.getString();
            http.end();

            return s;
          }

          http.end();
          delay(1);
        }

        ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_LINKED;
      }

      s = "";
    }
  }
  else // if(Server.arg("i") != NULL && Server.arg("i").equals(APSsid))
  {
    ucSystemMode &= 0xFF ^ SYSTEMMODE_GLOBAL_CMD;

    if (ucSystemMode & SYSTEMMODE_GLOBAL_CMD)
    {
      getTime();

      s = "";
      if (ucSystemCmdDelay < 100)
      {
        s += "0";
        if (ucSystemCmdDelay < 10)
          s += "0";
      }
      s += String(ucSystemCmdDelay);

      ulCmdMillis = ulSystemTimer + (ucSystemCmdDelay * 100);
    }
    else
      s = "000";

  _Make_resp_str:
    unsigned char i, j;

    for (i = 0; i < 8; i++)
    {
      j = uiAppData[i] & 0xFF;
      if (j < 16)
        s += "0";
      s += String(j, HEX);

      j = (uiAppData[i] >> 8) & 0xFF;
      if (j < 16)
        s += "0";
      s += String(j, HEX);

      j = (uiAppData[i] >> 16) & 0xFF;
      if (j < 16)
        s += "0";
      s += String(j, HEX);

      j = (uiAppData[i] >> 24) & 0xFF;
      if (j < 16)
        s += "0";
      s += String(j, HEX);
    }
  }

  return s;
}

// Function removed in simplified project
void globalCmdEnable()
{
  // This function is kept for compatibility but does nothing in simplified project
}

bool networkStarterCheck()
{
  if (Server.arg("q") != NULL || Server.arg("t") != NULL)
    return false;

  return true;
}

unsigned int toUInt(String str)
{
  unsigned int i = 0;
  unsigned char j = str.length() + 1, k;
  char buff[j];

  str.toCharArray(buff, j);

  j--;
  for (k = 0; k < j; k++)
  {
    i *= 10;
    i += buff[k] - '0';
  };

  return i;
}

unsigned long toULong(String str)
{
  unsigned long i = 0;
  unsigned char j = str.length() + 1, k;
  char buff[j];

  str.toCharArray(buff, j);

  j--;
  for (k = 0; k < j; k++)
  {
    i *= 10;
    i += buff[k] - '0';
  };

  return i;
}
