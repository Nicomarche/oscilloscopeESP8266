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
    bool trigger_acquired = false;
    unsigned long t;

    if (uiAppData[2] == 0) // No trigger
    {
      trigger_acquired = true;
    }
    else if (uiAppData[2] == 1) // raising
    {
      t = millis();
      if (uiAppData[1] == 0) // speed 0
      {
        system_adc_read_fast(usBuff, 1, 16); // Initial read for usBuff[0]
        usBuff[0] += ADC_R_THR;
        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16); // Read current value into usBuff[1]
          if (usBuff[1] > usBuff[0])
          {
            trigger_acquired = true;
            break; 
          }
          else
          {
            usBuff[0] = usBuff[1] + ADC_R_THR; // Update threshold
          }
          yield();
        }
      }
      else // speed non-0
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN) + ADC_R_THR; // Initial read for usBuff[0]
        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN); // Read current value into usBuff[1]
          if (usBuff[1] > usBuff[0])
          {
            trigger_acquired = true;
            break;
          }
          else
          {
            usBuff[0] = usBuff[1] + ADC_R_THR; // Update threshold
          }
          yield();
        }
      }
    }
    else if (uiAppData[2] == 2) // falling
    {
      t = millis();
      if (uiAppData[1] == 0) // speed 0
      {
        system_adc_read_fast(usBuff, 1, 16); // Initial read for usBuff[0]
        // No ADC_F_THR adjustment for the initial usBuff[0] here as per original logic
        while ((millis() - t) < 1000)
        {
          system_adc_read_fast(usBuff + 1, 1, 16); // Read current value into usBuff[1]
          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
          {
            trigger_acquired = true;
            break;
          }
          else
          {
            usBuff[0] = usBuff[1]; // Update reference
          }
          yield();
        }
      }
      else // speed non-0
      {
        usBuff[0] = analogRead(ANALOG_CH_PIN); // Initial read for usBuff[0]
        while ((millis() - t) < 1000)
        {
          usBuff[1] = analogRead(ANALOG_CH_PIN); // Read current value into usBuff[1]
          if ((usBuff[1] + ADC_F_THR) < usBuff[0])
          {
            trigger_acquired = true;
            break;
          }
          else
          {
            usBuff[0] = usBuff[1]; // Update reference
          }
          yield();
        }
      }
    }

    if (!trigger_acquired && uiAppData[2] != 0) // Trigger was required but not acquired (timed out)
    {
      // Determine which error message to log based on speed, matching previous specific messages
      if (uiAppData[2] == 1) { // raising
          if (uiAppData[1] == 0) {
              if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) {
                Serial.printf("[%u] WebSocket sendBIN failed (analog, raising, speed 0, trigger timeout)\n", clientNum);
              }
          } else {
              if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) {
                Serial.printf("[%u] WebSocket sendBIN failed (analog, raising, speed non-0, trigger timeout)\n", clientNum);
              }
          }
      } else if (uiAppData[2] == 2) { // falling
          if (uiAppData[1] == 0) {
              if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) {
                Serial.printf("[%u] WebSocket sendBIN failed (analog, falling, speed 0, trigger timeout)\n", clientNum);
              }
          } else {
              if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) {
                Serial.printf("[%u] WebSocket sendBIN failed (analog, falling, speed non-0, trigger timeout)\n", clientNum);
              }
          }
      }
      return;
    }

    // If trigger_acquired is true, or if uiAppData[2] == 0 (no trigger needed), proceed with data acquisition
    // The label __adc_start_ws: is effectively replaced by this conditional execution path.
    if (uiAppData[1] == 0) // 70ksps
    {
      unsigned short usBuffer[512];

      // Send data in chunks via WebSocket
      for (i = 0; i < 24; i++)
      {
        yield();
        system_adc_read_fast(usBuffer, 512, 16);
        if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024)) {
          Serial.printf("[%u] WebSocket sendBIN failed during 70ksps analog stream, chunk %d\n", clientNum, i);
          break; // Stop sending more chunks for this request
        }
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

      if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024)) {
        Serial.printf("[%u] WebSocket sendBIN failed (analog, 15ksps)\n", clientNum);
      }
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

      if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 128)) {
        Serial.printf("[%u] WebSocket sendBIN failed (analog, 300sps)\n", clientNum);
      }
    }
  }
  else // digital
  {
    bool trigger_acquired = false;
    unsigned long t;
    bool val1, val2; // Declare val1, val2 here for wider scope if needed for timeout send

    if (uiAppData[2] == 0) // No trigger
    {
      trigger_acquired = true;
    }
    else if (uiAppData[2] == 1) // raising
    {
      t = millis();
      val1 = digitalRead(DIGITAL_CH_PIN); // Initial read for val1
      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == HIGH && val2 == LOW)
        {
          trigger_acquired = true;
          break;
        }
        yield();
      }
    }
    else if (uiAppData[2] == 2) // falling
    {
      t = millis();
      val1 = digitalRead(DIGITAL_CH_PIN); // Initial read for val1
      while ((millis() - t) < 1000)
      {
        val2 = val1;
        val1 = digitalRead(DIGITAL_CH_PIN);
        if (val1 == LOW && val2 == HIGH)
        {
          trigger_acquired = true;
          break;
        }
        yield();
      }
    }

    if (!trigger_acquired && uiAppData[2] != 0) // Trigger was required but not acquired (timed out)
    {
      // Determine which error message to log based on trigger type
      if (uiAppData[2] == 1) { // raising
          if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) { // usBuff may contain stale data here, original code also sends it
            Serial.printf("[%u] WebSocket sendBIN failed (digital, raising, trigger timeout)\n", clientNum);
          }
      } else if (uiAppData[2] == 2) { // falling
          if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuff, 4)) { // usBuff may contain stale data here
            Serial.printf("[%u] WebSocket sendBIN failed (digital, falling, trigger timeout)\n", clientNum);
          }
      }
      return;
    }
    
    // If trigger_acquired is true, or if uiAppData[2] == 0 (no trigger needed), proceed with data acquisition
    // The label __dig_start_ws: is effectively replaced by this conditional execution path.
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

      if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 1024)) {
        Serial.printf("[%u] WebSocket sendBIN failed (digital, 3msps)\n", clientNum);
      }
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

      if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 768)) {
        Serial.printf("[%u] WebSocket sendBIN failed (digital, 100ksps)\n", clientNum);
      }
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

      if (!webSocket.sendBIN(clientNum, (uint8_t *)usBuffer, 128)) {
        Serial.printf("[%u] WebSocket sendBIN failed (digital, 3ksps)\n", clientNum);
      }
    }
  }
}
