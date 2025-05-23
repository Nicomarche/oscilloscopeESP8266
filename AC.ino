/*
 * WiFi Osiolloscope+Network volt-meter firmware for ESP8266
 * Rev 1.2
 *
 * WiFi Configuration Handler File
 *
 * Author: M. Mahdi K. Kanan (mvtdesign@gmail.com)
 * WiCardTech Engineering Group
 *
 * https://store.wicard.net
 */

bool AC_config()
{
  int i;

  if (Server.arg("aps") != NULL) // Access point settings
  {
    for (i = EEP_AP_SSID; i < (EEP_AP_SSID + EEP_AP_SSID_LEN); i++)
    {
      APSsid[i - EEP_AP_SSID] = Server.arg("aps")[i - EEP_AP_SSID];
      EEPROM.write(i, APSsid[i - EEP_AP_SSID]);
    }

    if (Server.arg("app") != NULL)
    {
      for (i = EEP_AP_PASS; i < (EEP_AP_PASS + EEP_AP_PASS_LEN); i++)
      {
        APPassword[i - EEP_AP_PASS] = Server.arg("app")[i - EEP_AP_PASS];
        EEPROM.write(i, APPassword[i - EEP_AP_PASS]);
      }
    }
    else
    {
      APPassword[0] = 0;
      EEPROM.write(EEP_AP_PASS, 0);
    }

    if (Server.arg("sli") != NULL)
    {
      for (i = EEP_SECURE; i < (EEP_SECURE + EEP_SECURE_LEN); i++)
      {
        if (Server.arg("sli")[i - EEP_SECURE] == 0)
        {
          secureLink[i - EEP_SECURE] = 0;

          EEPROM.write(i, 0);

          break;
        }
        else if ((Server.arg("sli")[i - EEP_SECURE] >= '0' && Server.arg("sli")[i - EEP_SECURE] <= '9') || (Server.arg("sli")[i - EEP_SECURE] >= 'A' && Server.arg("sli")[i - EEP_SECURE] <= 'Z') || (Server.arg("sli")[i - EEP_SECURE] >= 'a' && Server.arg("sli")[i - EEP_SECURE] <= 'z'))
        {
          secureLink[i - EEP_SECURE] = Server.arg("sli")[i - EEP_SECURE];

          EEPROM.write(i, secureLink[i - EEP_SECURE]);
        }
        else
        {
          secureLink[0] = 0;

          EEPROM.write(EEP_SECURE, 0);

          break;
        }
      }
    }
    else
    {
      secureLink[0] = 0;
      EEPROM.write(EEP_SECURE, 0);
    }

    ucSystemAPSTConfig &= 0xFF ^ (EEP_AC_AP_CONFIG_HIDDEN | EEP_AC_AP_CONFIG_TEMP | EEP_AC_AP_CONFIG_ODD_IP);

    if (Server.arg("hap") != NULL)
      ucSystemAPSTConfig |= EEP_AC_AP_CONFIG_HIDDEN;

    if (Server.arg("dap") != NULL)
      ucSystemAPSTConfig |= EEP_AC_AP_CONFIG_TEMP;

    if (Server.arg("odd") != NULL)
      ucSystemAPSTConfig |= EEP_AC_AP_CONFIG_ODD_IP;

    EEPROM.write(EEP_AC_APST_CONFIG, ucSystemAPSTConfig);

    EEPROM.commit();

    PGM_P content = PSTR("<html><head><title>AutoConnect</title></head><body>Hot Spot Saved! Please re-start the module.</body></html>");

    Server.send(200, "text/html", content);
  }
  else if (Server.arg("sts") != NULL) // station settings
  {
    for (i = EEP_ST_SSID; i < (EEP_ST_SSID + EEP_ST_SSID_LEN); i++)
    {
      STSsid[i - EEP_ST_SSID] = Server.arg("sts")[i - EEP_ST_SSID];
      EEPROM.write(i, STSsid[i - EEP_ST_SSID]);
    }

    if (Server.arg("stp") != NULL)
    {
      for (i = EEP_ST_PASS; i < (EEP_ST_PASS + EEP_ST_PASS_LEN); i++)
      {
        STPassword[i - EEP_ST_PASS] = Server.arg("stp")[i - EEP_ST_PASS];
        EEPROM.write(i, STPassword[i - EEP_ST_PASS]);
      }
    }
    else
    {
      STPassword[0] = 0;
      EEPROM.write(EEP_ST_PASS, 0);
    }

    EEPROM.commit();

#ifdef LOG_ENABLE
    Serial.print(PSTR("ST SSID:"));
    Serial.println(STSsid);

    Serial.print(PSTR("ST PASS:"));
    Serial.println(STPassword);
#endif

    PGM_P content = PSTR("<html><head><meta http-equiv='refresh' content='50; url=/'><title>Connecting...</title></head><body>Please Wait...</body></html>");

    Server.send(200, "text/html", content);
  }
  else if (Server.arg("us") != NULL)
  {
    String s;

    for (i = 0; i < 8; i++)
    {
      s = "s";
      s += i;

      if (Server.arg(s) != NULL)
      {
        if (Server.arg(s) == "")
          uiAppData[i] = 0;
        else if (Server.arg(s) == "on")
          uiAppData[i] = 1;
        else
          uiAppData[i] = toUInt(Server.arg(s));
      }
      else
        uiAppData[i] = 0;
    }

    userSetSettings();

    PGM_P c = PSTR("<html><head><title>Device Configuration Saved</title></head><body>Saved.</body></html>");

    Server.send(200, "text/html", c);
  }
  else if (Server.arg("cd") != NULL)
  {
    ucSystemCmdDelay = Server.arg("cd").toInt();
    EEPROM.write(EEP_CONF_CMD_DELAY, ucSystemCmdDelay);

    ucSystemAPSTConfig &= 0xFF ^ (EEP_AC_ST_JOIN_REQUIRED);

    if (Server.arg("jr") != NULL)
      ucSystemAPSTConfig |= EEP_AC_ST_JOIN_REQUIRED;

    EEPROM.write(EEP_AC_APST_CONFIG, ucSystemAPSTConfig);

    EEPROM.commit();

    PGM_P content = PSTR("<html><head><title>Configuration Saved</title></head><body>Saved.</body></html>");

    Server.send(200, "text/html", content);
  }
  else if (Server.arg("s") != NULL) // settings feedback
  {
    String content = "";

    if (Server.arg("s") == "1")
    {
      unsigned char c = Server.client().remoteIP()[2];

      if (ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED && ((c == 4 && ((ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP) == 0)) || (c == 5 && ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP))) // BWD
      {
        if (ucSystemMode & SYSTEMMODE_NET_JOINED)
        {
          for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
          {
            http.begin(client, cApIP, 80, "/?s=1");
            httpCode = http.GET();

            if (httpCode == 200)
            {
              content += http.getString();

              http.end();

              break;
            }

            http.end();
            delay(1);
          }

          if (ucReTry == 0)
            ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_JOINED;
        }
      }
      else // FWD
      {
        if (ucSystemMode & SYSTEMMODE_NET_LINKED && Server.client().remoteIP()[3] != linkedIP[3])
        {
          for (ucReTry = CMD_RETRY; ucReTry > 0; ucReTry--)
          {
            http.begin(client, cLinkedIP, 80, "/?s=1");
            httpCode = http.GET();

            if (httpCode == 200)
            {
              content += http.getString();

              http.end();

              break;
            }

            http.end();
            delay(1);
          }

          if (ucReTry == 0)
            ucSystemMode &= 0xFF ^ SYSTEMMODE_NET_LINKED;
        }
      }
      content += APSsid;
      content += "*";
    }
    else
    {
      /* EHTJCHPSIFDRA
              E: even=0 odd=1
              H: hidden hotspot=1
              T: temp hotspot=1
              J: Join required=1
              C: conntected to SSID [RSSI]*IP*saved SSID*
              H: hotspot SSID*
              P: hotspot password*
              S: secure link*
              I: Linked ip*
              F: cmd delay*
              D: ST MAC addr
              R: AP MAC addr
              A: available aps*/
      if (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP)
        content += 1;
      else
        content += 0;

      if (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_HIDDEN)
        content += 1;
      else
        content += 0;

      if (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_TEMP)
        content += 1;
      else
        content += 0;

      if (ucSystemAPSTConfig & EEP_AC_ST_JOIN_REQUIRED)
        content += 1;
      else
        content += 0;

      userGetSettings();

      unsigned char j;
      for (i = 0; i < 8; i++)
      {
        j = uiAppData[i] & 0xFF;
        if (j < 16)
          content += "0";
        content += String(j, HEX);

        j = (uiAppData[i] >> 8) & 0xFF;
        if (j < 16)
          content += "0";
        content += String(j, HEX);

        j = (uiAppData[i] >> 16) & 0xFF;
        if (j < 16)
          content += "0";
        content += String(j, HEX);

        j = (uiAppData[i] >> 24) & 0xFF;
        if (j < 16)
          content += "0";
        content += String(j, HEX);
      }

      if (WiFi.status() == WL_CONNECTED)
      {
        content += STSsid;
        content += " [";
        content += WiFi.RSSI();
        content += "]*";
        content += WiFi.localIP().toString();
        content += "*";
      }
      else
        content += "-*-*";

      content += STSsid;
      content += "*";

      content += APSsid;
      content += "*";

      content += APPassword;
      content += "*";

      content += secureLink;
      content += "*";

      if (ucSystemMode & SYSTEMMODE_NET_LINKED)
        content += cLinkedIP;
      else
        content += "-";
      content += "*";

      content += String(ucSystemCmdDelay);
      content += "*";

      byte mac[6];

      WiFi.macAddress(mac);

#ifdef LOG_ENABLE
      Serial.print(PSTR("MAC: "));
      Serial.print(mac[0], HEX);
      Serial.print(PSTR(":"));
      Serial.print(mac[1], HEX);
      Serial.print(PSTR(":"));
      Serial.print(mac[2], HEX);
      Serial.print(PSTR(":"));
      Serial.print(mac[3], HEX);
      Serial.print(PSTR(":"));
      Serial.print(mac[4], HEX);
      Serial.print(PSTR(":"));
      Serial.println(mac[5], HEX);
#endif

      // ST Mac
      content += String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX) + "*";

      // AP Mac
      content += String((mac[0] + 2), HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX) + "*";

#ifdef LOG_ENABLE
      Serial.println(PSTR("start scan"));
#endif

      // WiFi.scanNetworks will return the number of networks found
      int n = WiFi.scanNetworks();

#ifdef LOG_ENABLE
      Serial.println(PSTR("scan done"));
#endif

      if (n != 0)
      {
#ifdef LOG_ENABLE
        Serial.print(n);
        Serial.println(PSTR(" networks found"));
#endif

        for (int i = 0; i < n; ++i)
        {
          content += "<div class='ap'><b onclick='ap(this.innerHTML);'>" + WiFi.SSID(i) + "</b><b class='rssi'>" + WiFi.RSSI(i) + "</b></div>";
          // Print SSID and RSSI for each network found
#ifdef LOG_ENABLE
          Serial.print(i + 1);
          Serial.print(PSTR(": "));
          Serial.print(WiFi.SSID(i));
          Serial.print(PSTR(" ("));
          Serial.print(WiFi.RSSI(i));
          Serial.println(PSTR(")"));
          delay(10);
#endif
        }
      }
#ifdef LOG_ENABLE
      else
      {
        Serial.println(PSTR("no networks found"));
      }
#endif
    }

    Server.send(200, "text/html", content);
  }
  else
    return false;

  return true;
}

void reset_AC()
{
  int i;

#ifdef LOG_ENABLE
  Serial.print(PSTR("EE Reset..."));
#endif

  STSsid[0] = 0;
  STPassword[0] = 0;
  APSsid[0] = 'W';
  APSsid[1] = 'i';
  APSsid[2] = 'C';
  APSsid[3] = 'a';
  APSsid[4] = 'r';
  APSsid[5] = 'd';
  APSsid[6] = 'O';
  APSsid[7] = 'S';
  APSsid[8] = 'C';
  APSsid[9] = 0;

  APPassword[0] = '1';
  APPassword[1] = '2';
  APPassword[2] = '3';
  APPassword[3] = '4';
  APPassword[4] = '5';
  APPassword[5] = '6';
  APPassword[6] = '7';
  APPassword[7] = '8';
  APPassword[8] = 0;

  EEPROM.write(EEP_AC_KEY, MEM_SAVED);
  EEPROM.write(EEP_ST_SSID, 0);
  EEPROM.write(EEP_ST_PASS, 0);

  for (i = EEP_AP_SSID; i < (EEP_AP_SSID + EEP_AP_SSID_LEN); i++)
    EEPROM.write(i, APSsid[i - EEP_AP_SSID]);

  for (i = EEP_AP_PASS; i < (EEP_AP_PASS + EEP_AP_PASS_LEN); i++)
    EEPROM.write(i, APPassword[i - EEP_AP_PASS]);

  EEPROM.write(EEP_SECURE, 0);

  EEPROM.write(EEP_AC_APST_CONFIG, 0);

  EEPROM.write(EEP_CONF_CMD_DELAY, 0);

  EEPROM.commit();
}

void init_AC()
{
#ifdef LOG_ENABLE
  Serial.print(PSTR("Autoconnect Init!"));
#endif

  int i;

  if (byte(EEPROM.read(EEP_AC_KEY)) == MEM_SAVED)
  {
#ifdef LOG_ENABLE
    Serial.print(PSTR("EE OK!"));
#endif
    for (i = EEP_SECURE; i < (EEP_SECURE + EEP_SECURE_LEN); i++)
      secureLink[i - EEP_SECURE] = EEPROM.read(i);

    for (i = EEP_ST_SSID; i < (EEP_ST_SSID + EEP_ST_SSID_LEN); i++)
      STSsid[i - EEP_ST_SSID] = EEPROM.read(i);

    for (i = EEP_ST_PASS; i < (EEP_ST_PASS + EEP_ST_PASS_LEN); i++)
      STPassword[i - EEP_ST_PASS] = EEPROM.read(i);

    for (i = EEP_AP_SSID; i < (EEP_AP_SSID + EEP_AP_SSID_LEN); i++)
      APSsid[i - EEP_AP_SSID] = EEPROM.read(i);

    for (i = EEP_AP_PASS; i < (EEP_AP_PASS + EEP_AP_PASS_LEN); i++)
      APPassword[i - EEP_AP_PASS] = EEPROM.read(i); // Configuración modificada para no intentar conectarse a redes WiFi guardadas
    // y operar solo en modo punto de acceso (AP)

    // Siempre configurar en modo AP solamente
    WiFi.mode(WIFI_AP);

#ifdef LOG_ENABLE
    Serial.println(PSTR("Funcionando solo en modo punto de acceso"));
#endif
  }
  else
  {
#ifdef LOG_ENABLE
    Serial.print(PSTR("EE Init..."));
#endif

    reset_AC();
  }

  ucSystemAPSTConfig = EEPROM.read(EEP_AC_APST_CONFIG);

  if (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_ODD_IP)
    WiFi.softAPConfig(IPAddress(192, 168, 5, 1), IPAddress(192, 168, 5, 1), IPAddress(255, 255, 255, 0));
  else
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));

  if (ucSystemAPSTConfig & EEP_AC_AP_CONFIG_HIDDEN)
    WiFi.softAP(APSsid, APPassword, 1, true);
  else
    WiFi.softAP(APSsid, APPassword);

  ucSystemCmdDelay = EEPROM.read(EEP_CONF_CMD_DELAY);

#ifdef LOG_ENABLE
  Serial.println(PSTR("Autoconnect Init Done!"));
#endif
}
