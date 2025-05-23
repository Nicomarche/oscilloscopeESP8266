/*
 * WiFi Oscilloscope firmware for ESP8266
 * Simplified version - Fixed Digital Reading
 */

void userMainInit()
{
  // Initialización for oscilloscope functionality
}

void userMain()
{
  int i, j;
  unsigned short usBuff[2];

  if (uiAppData[0] == 0) // analog
  {
    if (uiAppData[2] == 1) // raising
    {
      unsigned long t = millis();

      if (uiAppData[1] == 0)
      {
        system_adc_read_fast(usBuff, 1, 16);
        usBuff[0] += ADC_R_THR;

        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16);

          if (usBuff[1] > usBuff[0])
            goto __adc_start;
          else
            usBuff[0] = usBuff[1] + ADC_R_THR;

          yield();
        };

        Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

        goto __adc_end;
      }
      else
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN) + ADC_R_THR;

        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN);

          if (usBuff[1] > usBuff[0])
            goto __adc_start;
          else
            usBuff[0] = usBuff[1] + ADC_R_THR;

          yield();
        };

        Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

        goto __adc_end;
      }
    }
    else if (uiAppData[2] == 2) // falling
    {
      unsigned long t = millis();

      if (uiAppData[1] == 0)
      {
        system_adc_read_fast(usBuff, 1, 16);

        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16);

          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
            goto __adc_start;
          else
            usBuff[0] = usBuff[1];

          yield();
        };

        Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

        goto __adc_end;
      }
      else
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN);

        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN);

          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
            goto __adc_start;
          else
            usBuff[0] = usBuff[1];

          yield();
        };

        Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

        goto __adc_end;
      }
    }

  __adc_start:
    if (uiAppData[1] == 0) // 70ksps
    {
      unsigned short usBuffer[512]; // Reducido el buffer a 512 muestras

      Server.setContentLength(24576); // cambiar de 12288 a 24576 bytes para 12288 muestras de 16 bits
      Server.send(200, "image/jpeg", "");

      for (i = 0; i < 24; i++) // Ahora 24 iteraciones para totalizar 12288 muestras
      {
        yield(); // Garantizar que el watchdog no se active
        system_adc_read_fast(usBuffer, 512, 16);
        Server.sendContent_P((const char *)usBuffer, 1024);
        yield(); // Garantizar que el watchdog no se active
      }

      Server.client().stop();
    }
    else if (uiAppData[1] == 1) // 15ksps
    {
      unsigned short usBuffer[512]; // Reducir el tamaño del buffer a la mitad

      yield();

      // Inicializar el buffer con ceros para evitar datos basura
      memset(usBuffer, 0, sizeof(usBuffer));

      i = 0;
      unsigned long startTime = millis();

      while (i < 512) // Reducir el número de muestras a la mitad
      {
        // Limitar el tiempo máximo de adquisición a 500ms
        if (millis() - startTime > 500)
        {
          break;
        }

        // usBuffer[i] = analogRead(ANALOG_CH_PIN);
        usBuffer[i] = system_adc_read();

        i++;

        if (i % 32 == 0)
        {
          yield(); // Añadir yield más frecuente para evitar watchdog timeout
        }
      };

      Server.send_P(200, "image/jpeg", (const char *)usBuffer, 1024); // Reducir el tamaño de datos enviados
    }
    else // if(uiAppData[1] == 2) //300sps
    {
      unsigned short usBuffer[64];

      // Inicializar el buffer con ceros para evitar datos basura
      memset(usBuffer, 0, sizeof(usBuffer));

      i = 0;
      unsigned long startTime = millis();

      while (i < 64)
      {
        // Limitar el tiempo máximo de adquisición a 350ms
        if (millis() - startTime > 350)
        {
          break;
        }

        usBuffer[i] = analogRead(ANALOG_CH_PIN);
        i++;

        // Tiempo entre muestras ajustado para mantener aproximadamente 300sps
        delayMicroseconds(2800);

        if (i % 8 == 0)
        {
          yield(); // Añadir yield más frecuente para evitar watchdog timeout
        }
      };

      Server.send_P(200, "image/jpeg", (const char *)usBuffer, 128);
    }
  }
  else // digital
  {
    if (uiAppData[2] == 1) // raising
    {
      unsigned long t = millis();
      bool val1 = digitalRead(DIGITAL_CH_PIN), val2;

      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == HIGH && val2 == LOW)
          goto __dig_start;

        yield();
      };

      Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

      goto __adc_end;
    }
    else if (uiAppData[2] == 2) // falling
    {
      unsigned long t = millis();
      bool val1 = digitalRead(DIGITAL_CH_PIN), val2;

      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == LOW && val2 == HIGH)
          goto __dig_start;

        yield();
      };

      Server.send_P(200, "image/jpeg", (const char *)usBuff, 4);

      goto __adc_end;
    }

  __dig_start:
    if (uiAppData[1] == 0) // 3msps
    {
      unsigned short usBuffer[512]; // Reducir el tamaño del buffer

      yield();

      // Limpiar el buffer antes de usarlo
      memset(usBuffer, 0, sizeof(usBuffer));

      // En vez de muestrear multiples bits por bucle, simplemente captura un bit por iteración
      for (i = 0; i < 512; i++)
      {
        // Leer el pin digital y guardar 0xFFFF si es HIGH, o 0 si es LOW
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 64 == 0)
          yield(); // Permitir otras tareas cada 64 muestras
      }
      yield();

      Server.send_P(200, "image/jpeg", (const char *)usBuffer, 1024); // Enviar los datos digitales
    }
    else if (uiAppData[1] == 1) // 100ksps
    {
      unsigned short usBuffer[384]; // Reducir el tamaño del buffer

      // Limpiar el buffer antes de usarlo
      memset(usBuffer, 0, sizeof(usBuffer));

      for (i = 0; i < 384; i++)
      {
        // Agregar un pequeño retardo para controlar la velocidad de muestreo
        delayMicroseconds(10);

        // Leer el pin digital y guardar 0xFFFF si es HIGH, o 0 si es LOW
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 32 == 0)
          yield(); // Permitir otras tareas cada 32 muestras
      }

      Server.send_P(200, "image/jpeg", (const char *)usBuffer, 768); // Enviar los datos digitales
    }
    else // if(uiAppData[1] == 2) //3ksps
    {
      unsigned short usBuffer[64];

      // Limpiar el buffer antes de usarlo
      memset(usBuffer, 0, sizeof(usBuffer));

      for (i = 0; i < 64; i++)
      {
        // Agregar un retardo mayor para menor velocidad de muestreo
        delayMicroseconds(330);

        // Leer el pin digital y guardar 0xFFFF si es HIGH, o 0 si es LOW
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 8 == 0)
          yield(); // Permitir otras tareas cada 8 muestras
      }

      Server.send_P(200, "image/jpeg", (const char *)usBuffer, 128); // Enviar los datos digitales
    }
  }

__adc_end:;
}

// WebSocket version of userMain - sends data via WebSocket instead of HTTP
void userMainWebSocket(uint8_t clientNum)
{
  extern WebSocketsServer webSocket;
  int i, j;
  unsigned short usBuff[2];

  Serial.printf("WebSocket data request - Mode: %s, Speed: %d\n",
                uiAppData[0] ? "Digital" : "Analog", uiAppData[1]);

  if (uiAppData[0] == 0) // analog
  {
    if (uiAppData[2] == 1) // raising
    {
      unsigned long t = millis();

      if (uiAppData[1] == 0)
      {
        system_adc_read_fast(usBuff, 1, 16);
        usBuff[0] += ADC_R_THR;

        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16);

          if (usBuff[1] > usBuff[0])
            goto __adc_start_ws;
          else
            usBuff[0] = usBuff[1] + ADC_R_THR;

          yield();
        };

        webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
        return;
      }
      else
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN) + ADC_R_THR;

        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN);

          if (usBuff[1] > usBuff[0])
            goto __adc_start_ws;
          else
            usBuff[0] = usBuff[1] + ADC_R_THR;

          yield();
        };

        webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
        return;
      }
    }
    else if (uiAppData[2] == 2) // falling
    {
      unsigned long t = millis();

      if (uiAppData[1] == 0)
      {
        system_adc_read_fast(usBuff, 1, 16);

        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16);

          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
            goto __adc_start_ws;
          else
            usBuff[0] = usBuff[1];

          yield();
        };

        webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
        return;
      }
      else
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN);

        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN);

          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
            goto __adc_start_ws;
          else
            usBuff[0] = usBuff[1];

          yield();
        };

        webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
        return;
      }
    }

  __adc_start_ws:
    if (uiAppData[1] == 0) // 70ksps
    {
      unsigned short usBuffer[512];

      // Send data in chunks via WebSocket
      for (i = 0; i < 24; i++)
      {
        yield();
        system_adc_read_fast(usBuffer, 512, 16);
        webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024);
        yield();
      }
    }
    else if (uiAppData[1] == 1) // 15ksps
    {
      unsigned short usBuffer[512];

      yield();
      memset(usBuffer, 0, sizeof(usBuffer));

      i = 0;
      unsigned long startTime = millis();

      while (i < 512)
      {
        if (millis() - startTime > 500)
        {
          break;
        }

        usBuffer[i] = system_adc_read();
        i++;

        if (i % 32 == 0)
        {
          yield();
        }
      };

      webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024);
    }
    else // if(uiAppData[1] == 2) //300sps
    {
      unsigned short usBuffer[64];

      memset(usBuffer, 0, sizeof(usBuffer));

      i = 0;
      unsigned long startTime = millis();

      while (i < 64)
      {
        if (millis() - startTime > 350)
        {
          break;
        }

        usBuffer[i] = analogRead(ANALOG_CH_PIN);
        i++;

        delayMicroseconds(2800);

        if (i % 8 == 0)
        {
          yield();
        }
      };

      webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 128);
    }
  }
  else // digital
  {
    if (uiAppData[2] == 1) // raising
    {
      unsigned long t = millis();
      bool val1 = digitalRead(DIGITAL_CH_PIN), val2;

      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == HIGH && val2 == LOW)
          goto __dig_start_ws;

        yield();
      };

      webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
      return;
    }
    else if (uiAppData[2] == 2) // falling
    {
      unsigned long t = millis();
      bool val1 = digitalRead(DIGITAL_CH_PIN), val2;

      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == LOW && val2 == HIGH)
          goto __dig_start_ws;

        yield();
      };

      webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4);
      return;
    }

  __dig_start_ws:
    if (uiAppData[1] == 0) // 3msps
    {
      unsigned short usBuffer[512];

      yield();
      memset(usBuffer, 0, sizeof(usBuffer));

      for (i = 0; i < 512; i++)
      {
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 64 == 0)
          yield();
      }
      yield();

      webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024);
    }
    else if (uiAppData[1] == 1) // 100ksps
    {
      unsigned short usBuffer[384];

      memset(usBuffer, 0, sizeof(usBuffer));

      for (i = 0; i < 384; i++)
      {
        delayMicroseconds(10);
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 32 == 0)
          yield();
      }

      webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 768);
    }
    else // if(uiAppData[1] == 2) //3ksps
    {
      unsigned short usBuffer[64];

      memset(usBuffer, 0, sizeof(usBuffer));

      for (i = 0; i < 64; i++)
      {
        delayMicroseconds(330);
        usBuffer[i] = digitalRead(DIGITAL_CH_PIN) ? 0xFFFF : 0;

        if (i % 8 == 0)
          yield();
      }

      webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 128);
    }
  }
}
