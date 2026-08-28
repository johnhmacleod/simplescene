#include <Arduino.h>
#include <WiFiUdp.h>

#ifndef GLOBALS_H
#define GLOBALS_H

// ==========================================
// COMPILER CONTROL MARGIN CONTROL OVERRIDES
// ==========================================
#define ENABLE_DEBUG 

// Hardware Mapping Constants
#define PIN_SEG_A 1
#define PIN_SEG_B 2
#define PIN_SEG_C 3
#define PIN_SEG_D 4
#define PIN_SEG_E 5
#define PIN_SEG_F 7
#define PIN_SEG_G 8

#define BTN_UP   12
#define BTN_DOWN 13

// System Architectural Values
#define DEBOUNCE_DELAY 200
#define LOCAL_UDP_PORT 51320
#define CONSOLE_PORT   51320
#define MIDI_PORT      51325
#define PING_TIMEOUT   1000

// FSM Configuration States
enum DeviceState {
  STATE_WIFI_DISCONNECTED,
  STATE_DISCOVERING,
  STATE_CONNECTED_IDLE,
  STATE_SELECTION,
  STATE_COMMITTED,
  STATE_FAULT_SQ
};

// Segment Character Matrix Layouts
const uint8_t SEGMENT_MAP[] = {
  0x00, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};
const uint8_t SEG_PATTERN_C = 0x39;
const uint8_t SEG_PATTERN_E = 0x79;

// Chaser Sequence Reference Framework
const uint8_t CHASER_SEQUENCE[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20
};

const char DISCOVERY_MSG[] = "SQ Find";

// Volatile System Registries
extern DeviceState currentState;
extern DeviceState lastReportedState;
extern int activeScene;
extern int pendingScene;
extern int chaserStep;
extern int failedPingCount;

extern unsigned long lastBlinkTime;
extern unsigned long lastBroadcastTime;
extern unsigned long lastChaserTime;
extern unsigned long lastSelectionInputTime;
extern unsigned long lastKeepAliveTime;
extern unsigned long pingSentTime;
extern unsigned long lastFaultTime;
extern unsigned long lastUpDebounceTime;
extern unsigned long lastDownDebounceTime;

extern bool blinkState;
extern bool operationalHistoryFlag;
extern bool firstBootTriggered;

extern WiFiUDP udpClient;
extern IPAddress consoleIP; // External register exposure for cross-tab mapping

// External Network Credentials
#include "Credentials.h"

// Forward Declarations of functions across tabs
void handleWiFiDisconnectedState(unsigned long currentMillis);
void handleDiscoveringState(unsigned long currentMillis);
void handleConnectedIdleState(unsigned long currentMillis);
void handleSelectionState(unsigned long currentMillis);
void handleCommittedState(unsigned long currentMillis);
void handleFaultSQState(unsigned long currentMillis);
void handleInputs(unsigned long currentMillis);
void processButtonAction(unsigned long currentMillis, int modifier);
void runUnifiedNetworkEngine(unsigned long currentMillis);
void transmitMidiSceneChange(int sceneNum);
void writeSegmentPattern(uint8_t pattern);
void clearDisplayPins();

#ifdef ENABLE_DEBUG
const char* getStateName(DeviceState state);
void handleSerialCommandIntercept(unsigned long currentMillis);
#endif

#endif
