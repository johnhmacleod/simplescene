#include <WiFi.h>
#include <WiFiUdp.h>
#include "Globals.h"

// Explicit Global Core Register Instantiations
DeviceState currentState = STATE_WIFI_DISCONNECTED;
DeviceState lastReportedState = STATE_WIFI_DISCONNECTED; 
int activeScene = 1;
int pendingScene = 1;
int chaserStep = 0;
int failedPingCount = 0;

// Dynamic Asynchronous Clock Registers
unsigned long lastBlinkTime = 0;
unsigned long lastBroadcastTime = 0;
unsigned long lastChaserTime = 0;
unsigned long lastSelectionInputTime = 0;
unsigned long lastKeepAliveTime = 0;
unsigned long pingSentTime = 0;
unsigned long lastFaultTime = 0;
unsigned long lastUpDebounceTime = 0;
unsigned long lastDownDebounceTime = 0;

// Operational Flags
bool blinkState = false;
bool operationalHistoryFlag = false;
bool firstBootTriggered = false;

// Network Peripheral Subsystems & Console Coordinate Register
WiFiUDP udpClient;
IPAddress consoleIP(0, 0, 0, 0); // Discovered Console IP Address Register

void setup() {
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
  delay(500); 
  Serial.println(F("[SYS] Booting Lolin S2 Network Controller..."));
  Serial.println(F("[SYS] Debug Mode Active. Type 1-9 in Serial Monitor to force scene changes manually."));
#endif

  // Enforce explicit hardware pin layouts
  pinMode(PIN_SEG_A, OUTPUT);
  pinMode(PIN_SEG_B, OUTPUT);
  pinMode(PIN_SEG_C, OUTPUT);
  pinMode(PIN_SEG_D, OUTPUT);
  pinMode(PIN_SEG_E, OUTPUT);
  pinMode(PIN_SEG_F, OUTPUT);
  pinMode(PIN_SEG_G, OUTPUT);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  clearDisplayPins();

  // Initialize network stack
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
#ifdef ENABLE_DEBUG
  Serial.print(F("[WIFI] Connecting to SSID: "));
  Serial.println(WIFI_SSID);
#endif
}

void loop() {
  unsigned long currentMillis = millis();

  // Check physical network link status layer
  if (WiFi.status() != WL_CONNECTED) {
    if (currentState != STATE_WIFI_DISCONNECTED) {
#ifdef ENABLE_DEBUG
      Serial.println(F("[WIFI] Link Lost! Forcing disconnection procedures."));
#endif
      udpClient.stop();
      currentState = STATE_WIFI_DISCONNECTED;
    }
  }

#ifdef ENABLE_DEBUG
  if (currentState != lastReportedState) {
    Serial.print(F("[FSM] State transition: "));
    Serial.print(getStateName(lastReportedState));
    Serial.print(F(" -> "));
    Serial.println(getStateName(currentState));
    lastReportedState = currentState;
  }

  handleSerialCommandIntercept(currentMillis);
#endif

  // Execute state machine actions
  switch (currentState) {
    case STATE_WIFI_DISCONNECTED:
      handleWiFiDisconnectedState(currentMillis);
      break;

    case STATE_DISCOVERING:
      handleDiscoveringState(currentMillis);
      break;

    case STATE_CONNECTED_IDLE:
      handleConnectedIdleState(currentMillis);
      break;

    case STATE_SELECTION:
      handleSelectionState(currentMillis);
      break;

    case STATE_COMMITTED:
      handleCommittedState(currentMillis);
      break;

    case STATE_FAULT_SQ:
      handleFaultSQState(currentMillis);
      break;
  }

  // Asynchronous input routine scans
  handleInputs(currentMillis);
}

#ifdef ENABLE_DEBUG
const char* getStateName(DeviceState state) {
  switch (state) {
    case STATE_WIFI_DISCONNECTED: return "WIFI_DISCONNECTED";
    case STATE_DISCOVERING:       return "DISCOVERING";
    case STATE_CONNECTED_IDLE:    return "CONNECTED_IDLE";
    case STATE_SELECTION:         return "SELECTION_PENDING";
    case STATE_COMMITTED:         return "SCENE_COMMITTED";
    case STATE_FAULT_SQ:          return "FAULT_SQ_TIMEOUT";
    default:                      return "UNKNOWN";
  }
}

void handleSerialCommandIntercept(unsigned long currentMillis) {
  if (Serial.available() > 0) {
    char incomingChar = Serial.read();
    
    if (incomingChar == '\r' || incomingChar == '\n') {
      return;
    }

    if (incomingChar >= '1' && incomingChar <= '9') {
      int manualScene = incomingChar - '0';
      
      Serial.print(F("[INTERCEPT] Received Serial Terminal Request for Scene: "));
      Serial.println(manualScene);

      firstBootTriggered = true; 
      pendingScene = manualScene;
      lastSelectionInputTime = currentMillis;
      currentState = STATE_SELECTION;
    } else {
      Serial.print(F("[INTERCEPT] Invalid Terminal Character Input ignored: '"));
      Serial.print(incomingChar);
      Serial.println(F("'. Enter a single digit 1-9."));
    }
  }
}
#endif
