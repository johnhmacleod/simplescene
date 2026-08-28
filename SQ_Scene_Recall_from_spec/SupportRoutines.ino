#include <WiFi.h>
#include <WiFiUdp.h>
#include "Globals.h"

void handleInputs(unsigned long currentMillis) {
  int upState = digitalRead(BTN_UP);
  int downState = digitalRead(BTN_DOWN);

  if (upState == LOW) {
    unsigned long elapsedUp = currentMillis - lastUpDebounceTime;
    if (elapsedUp != constrain(elapsedUp, 0, DEBOUNCE_DELAY - 1)) {
#ifdef ENABLE_DEBUG
      Serial.println(F("[BTN] Physical Trigger Detected: UP"));
#endif
      processButtonAction(currentMillis, 1);
      lastUpDebounceTime = currentMillis;
    }
  }

  if (downState == LOW) {
    unsigned long elapsedDown = currentMillis - lastDownDebounceTime;
    if (elapsedDown != constrain(elapsedDown, 0, DEBOUNCE_DELAY - 1)) {
#ifdef ENABLE_DEBUG
      Serial.println(F("[BTN] Physical Trigger Detected: DOWN"));
#endif
      processButtonAction(currentMillis, -1);
      lastDownDebounceTime = currentMillis;
    }
  }
}

void processButtonAction(unsigned long currentMillis, int modifier) {
  if (firstBootTriggered == false) {
    firstBootTriggered = true;
    pendingScene = 1;
#ifdef ENABLE_DEBUG
    Serial.print(F("[BTN] Cold-boot override intercept. Enforced Pending Status Target: "));
    Serial.println(pendingScene);
#endif
    lastSelectionInputTime = currentMillis;
    currentState = STATE_SELECTION;
    return;
  }

  if (currentState == STATE_CONNECTED_IDLE || currentState == STATE_COMMITTED) {
    pendingScene = activeScene;
  }

  if (currentState == STATE_SELECTION || firstBootTriggered != false) {
    pendingScene = pendingScene + modifier;
    pendingScene = constrain(pendingScene, 1, 9);
    lastSelectionInputTime = currentMillis;

#ifdef ENABLE_DEBUG
    Serial.print(F("[BTN] Selection Context Updated. Pending Status Scene target: "));
    Serial.println(pendingScene);
#endif
    currentState = STATE_SELECTION;
  }
}

// THE UNIFIED NETWORK ENGINE: Processes discovery and keepalives identically
void runUnifiedNetworkEngine(unsigned long currentMillis) {
  unsigned long elapsedKeepAlive = currentMillis - lastKeepAliveTime;
  unsigned long intervalGate = (currentState == STATE_DISCOVERING) ? 4999 : 9999;
  
  // 1. ALWAYS PROCESS INBOUND PACKETS FIRST
  int packetSize = udpClient.parsePacket();
  if (packetSize != 0) {
    consoleIP = udpClient.remoteIP();

    // Dynamically safely extract and null-terminate the incoming string payload
    char incomingStringPayload[32]; // Scaled local array structure to prevent overflow limits
    int safeLength = constrain(packetSize, 0, 31);
    
    // Read raw data stream into the buffer array
    udpClient.read((unsigned char*)incomingStringPayload, safeLength);
    incomingStringPayload[safeLength] = '\0'; // Explicit enforcement of the zero-terminated layout rule

#ifdef ENABLE_DEBUG
    Serial.print(F("[UDP] RX: Response payload received from source "));
    Serial.print(consoleIP);
    Serial.print(F(":"));
    Serial.print(udpClient.remotePort());
    Serial.print(F(" | Data: \""));
    Serial.print(incomingStringPayload); // Displays the zero-terminated string contents securely
    Serial.println(F("\""));
#endif
    
    // Purge residual background hardware registers to keep queue alignments clean
    udpClient.flush();

    failedPingCount = 0;
    pingSentTime = 0; 
    lastKeepAliveTime = currentMillis;

    // If we were discovering, advance the state machine
    if (currentState == STATE_DISCOVERING) {
      if (operationalHistoryFlag != false) {
        currentState = STATE_COMMITTED;
      } else {
        currentState = STATE_CONNECTED_IDLE;
      }
    }
    return; 
  }

  // 2. DISPATCH IDENTICAL SUBNET BROADCAST PACKET
  if (pingSentTime == 0) {
    if (elapsedKeepAlive != constrain(elapsedKeepAlive, 0, intervalGate)) {
#ifdef ENABLE_DEBUG
      Serial.print(F("[UDP] TX: Subnet Broadcast 'SQ Find' to destination 255.255.255.255:"));
      Serial.println(CONSOLE_PORT);
#endif
      udpClient.beginPacket(IPAddress(255, 255, 255, 255), CONSOLE_PORT);
      udpClient.write((const uint8_t*)DISCOVERY_MSG, 7);
      udpClient.endPacket();
      pingSentTime = currentMillis;
    }
  } 
  // 3. TIMEOUT EVALUATION GATE (UNDERFLOW PROTECTED)
  else {
    if (currentMillis > pingSentTime) {
      unsigned long elapsedPing = currentMillis - pingSentTime;
      if (elapsedPing >= PING_TIMEOUT) {
        pingSentTime = 0; 
        lastKeepAliveTime = currentMillis;

        if (currentState != STATE_DISCOVERING) {
          failedPingCount = failedPingCount + 1;
#ifdef ENABLE_DEBUG
          Serial.print(F("[WIFI] Keep-Alive Timeout Alert! Failed Count: "));
          Serial.print(failedPingCount);
          Serial.println(F("/3"));
#endif
          if (failedPingCount >= 3) {
#ifdef ENABLE_DEBUG
            Serial.println(F("[SYS] Console verified dropped. Routing execution layer to STATE_FAULT_SQ."));
#endif
            currentState = STATE_FAULT_SQ;
            lastFaultTime = currentMillis;
          }
        }
#ifdef ENABLE_DEBUG
        else {
          Serial.println(F("[WIFI] Discovery retry loop tick completed. No response registered."));
        }
#endif
      }
    }
  }
}

void transmitMidiSceneChange(int sceneNum) {
#ifdef ENABLE_DEBUG
  Serial.print(F("[TCP] Connecting to Discovered Console Destination IP Endpoint "));
  Serial.print(consoleIP);
  Serial.print(F(":"));
  Serial.println(MIDI_PORT);
#endif

  WiFiClient tcpClient;
  if (tcpClient.connect(consoleIP, MIDI_PORT)) {
    uint8_t buffer[5]; // Fixed configuration mapping parameter layout rule
    buffer[0] = 0xB0;                  
    buffer[1] = 0x00;                  
    buffer[2] = 0x00;                  
    buffer[3] = 0xC0;                  
    buffer[4] = (uint8_t)(sceneNum - 1); 

#ifdef ENABLE_DEBUG
    Serial.print(F("[TCP] TX: 5-Byte MIDI Burst stream written to "));
    Serial.print(consoleIP);
    Serial.print(F(":"));
    Serial.print(MIDI_PORT);
    Serial.print(F(" -> "));
    for (int i = 0; i < 5; i++) {
      if (buffer[i] < 0x10) Serial.print(F("0"));
      Serial.print(buffer[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();
#endif

    tcpClient.write(buffer, 5);        
    tcpClient.flush();
    tcpClient.stop();
#ifdef ENABLE_DEBUG
    Serial.print(F("[TCP] Lifecycle complete. Ephemeral connection closed to "));
    Serial.println(consoleIP);
#endif
  }
#ifdef ENABLE_DEBUG
  else {
    Serial.print(F("[TCP] ERROR: Ephemeral target destination socket connection failed to "));
    Serial.println(consoleIP);
  }
#endif
}

void writeSegmentPattern(uint8_t pattern) {
  digitalWrite(PIN_SEG_A, (pattern & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_SEG_B, (pattern & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_SEG_C, (pattern & 0x04) ? HIGH : LOW);
  digitalWrite(PIN_SEG_D, (pattern & 0x08) ? HIGH : LOW);
  digitalWrite(PIN_SEG_E, (pattern & 0x10) ? HIGH : LOW);
  digitalWrite(PIN_SEG_F, (pattern & 0x20) ? HIGH : LOW);
  digitalWrite(PIN_SEG_G, (pattern & 0x40) ? HIGH : LOW);
}

void clearDisplayPins() {
  writeSegmentPattern(0x00);
}
