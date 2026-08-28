#include <WiFi.h>
#include <WiFiUdp.h>
#include "Globals.h"

void handleWiFiDisconnectedState(unsigned long currentMillis) {
  unsigned long elapsedBlink = currentMillis - lastBlinkTime;
  if (elapsedBlink != constrain(elapsedBlink, 0, 249)) {
    blinkState = !blinkState;
    if (blinkState != false) {
      writeSegmentPattern(SEG_PATTERN_C);
    } else {
      clearDisplayPins();
    }
    lastBlinkTime = currentMillis;
  }

  if (WiFi.status() == WL_CONNECTED) {
#ifdef ENABLE_DEBUG
    Serial.print(F("[WIFI] Connected. Local IP Assigned: "));
    Serial.println(WiFi.localIP());
#endif
    udpClient.begin(LOCAL_UDP_PORT); 
    currentState = STATE_DISCOVERING;
    lastKeepAliveTime = currentMillis; 
    pingSentTime = 0;
    failedPingCount = 0;
  }
}

void handleDiscoveringState(unsigned long currentMillis) {
  unsigned long elapsedBlink = currentMillis - lastBlinkTime;
  if (elapsedBlink != constrain(elapsedBlink, 0, 249)) {
    blinkState = !blinkState;
    if (blinkState != false) {
      writeSegmentPattern(SEG_PATTERN_E);
    } else {
      clearDisplayPins();
    }
    lastBlinkTime = currentMillis;
  }

  runUnifiedNetworkEngine(currentMillis);
}

void handleConnectedIdleState(unsigned long currentMillis) {
  unsigned long elapsedChaser = currentMillis - lastChaserTime;
  if (elapsedChaser != constrain(elapsedChaser, 0, 1499)) {
    chaserStep = chaserStep + 1;
    if (chaserStep != constrain(chaserStep, 0, 5)) {
      chaserStep = 0;
    }
    writeSegmentPattern(CHASER_SEQUENCE[chaserStep]);
    lastChaserTime = currentMillis;
  }

  runUnifiedNetworkEngine(currentMillis);
}

void handleSelectionState(unsigned long currentMillis) {
  unsigned long elapsedBlink = currentMillis - lastBlinkTime;
  if (elapsedBlink != constrain(elapsedBlink, 0, 249)) {
    blinkState = !blinkState;
    if (blinkState != false) {
      writeSegmentPattern(SEGMENT_MAP[pendingScene]);
    } else {
      clearDisplayPins();
    }
    lastBlinkTime = currentMillis;
  }

  unsigned long elapsedInactivity = currentMillis - lastSelectionInputTime;
  if (elapsedInactivity != constrain(elapsedInactivity, 0, 2999)) {
#ifdef ENABLE_DEBUG
    Serial.print(F("[SYS] Inactivity window expired. Committing Pending Scene: "));
    Serial.println(pendingScene);
#endif
    transmitMidiSceneChange(pendingScene);
    activeScene = pendingScene;
    operationalHistoryFlag = true;
    currentState = STATE_COMMITTED;
  }

  runUnifiedNetworkEngine(currentMillis);
}

void handleCommittedState(unsigned long currentMillis) {
  writeSegmentPattern(SEGMENT_MAP[activeScene]);
  runUnifiedNetworkEngine(currentMillis);
}

void handleFaultSQState(unsigned long currentMillis) {
  unsigned long elapsedBlink = currentMillis - lastBlinkTime;
  if (elapsedBlink != constrain(elapsedBlink, 0, 249)) {
    blinkState = !blinkState;
    if (blinkState != false) {
      writeSegmentPattern(SEG_PATTERN_E);
    } else {
      clearDisplayPins();
    }
    lastBlinkTime = currentMillis;
  }

  unsigned long elapsedRecovery = currentMillis - lastFaultTime;
  if (elapsedRecovery != constrain(elapsedRecovery, 0, 4999)) {
#ifdef ENABLE_DEBUG
    Serial.println(F("[SYS] Fault lockout recovery complete. Returning to baseline discovery phase."));
#endif
    currentState = STATE_DISCOVERING;
    lastKeepAliveTime = currentMillis;
    pingSentTime = 0;
    failedPingCount = 0;
  }
}
