// Code for the NodeMCU's used during introductionweek TUe 2020
// Case organized by Volundr committee of e.t.s.v. Thor
// Created by Bart Bas on 2020-08-09

#include <Arduino.h>
#include <painlessMesh.h>
#include "pitches.h"
#include "thor_song.h"

// Defines
#define MESH_SSID                 "Thor_Volundr_Introcase"
#define MESH_PASSWORD             "Volundr=Cool"
#define MESH_PORT                 618

#define BUZZER_PIN                3   // Restrictions: RX, not usable during serial receiving
#define LED_PIN                   15  // Restrictions: via resistor on board pulled down; do not pull up at boot time
#define BUZZER_BUTTON_PIN         2   // Restrictions: via resistor on board pulled up; do not pull down at boot time; sends debug data at boot time
#define LED_BUTTON_PIN            13  // Restrictions: none
#define DIP_SWITCH_PIN1           5   // Restrictions: none
#define DIP_SWITCH_PIN2           4   // Restrictions: none
#define DIP_SWITCH_PIN3           14  // Restrictions: none
#define DIP_SWITCH_PIN4           12  // Restrictions: none
#define ONBOARD_LED_PIN           16  // Restrictions: LED is inverted
#define ONBOARD_BUTTON_PIN        0   // Restrictions: via resistor on board pulled up

#define BUTTON_DEBOUNCE_TIME_MS   30
#define SWITCH_DEBOUNCE_TIME_MS   200
#define HEARTBEAT_ON_TIME_MS      400
#define HEARTBEAT_OFF_TIME_MS     3000
#define HEARTBEAT_TIME_MS         HEARTBEAT_ON_TIME_MS + HEARTBEAT_OFF_TIME_MS
#define LED_ON_TIME_MS            2500
#define BUZZER_ON_TIME_MS         1000
#define RESPONSE_DELAY_MS         1000

//#define DEBUG // Comment out to disable serial prints
//#define DEBUG_HEARTBEAT // Comment out to disable only the heartbeat serial prints
#ifdef DEBUG
  #define PRINT(a) Serial.print(a);
#else
  #define PRINT(a)
#endif

//#define VOLUNDR // When defined this NodeMCU is allowed to broadcast the Thor song message

// Prototypes
ICACHE_RAM_ATTR void handleBuzzerButtonInterrupt();
ICACHE_RAM_ATTR void handleLedButtonInterrupt();
ICACHE_RAM_ATTR void handleDipSwitchInterrupt();
ICACHE_RAM_ATTR void handleOnboardButtonInterrupt();
void buzzerButtonCallback();
void ledButtonCallback();
void dipSwitchCallback();
void onboardButtonCallback();
void broadcastAction(String action);
void receivedMessageCallback(uint32_t from, String &message);
void changedConnectionsCallback();
void heartbeat();
void buzzerOn();
void buzzerOff();
void ledOn();
void ledOff();
void playThorSong();
void stopThorSong();

// Global variables
painlessMesh mesh;
Scheduler ts;
Task buzzerButtonTask(BUTTON_DEBOUNCE_TIME_MS, TASK_ONCE, &buzzerButtonCallback, &ts, false);
Task ledButtonTask(BUTTON_DEBOUNCE_TIME_MS, TASK_ONCE, &ledButtonCallback, &ts, false);
Task dipSwitchTask(SWITCH_DEBOUNCE_TIME_MS, TASK_ONCE, &dipSwitchCallback, &ts, false);
Task onboardButtonTask(BUTTON_DEBOUNCE_TIME_MS, TASK_ONCE, &onboardButtonCallback, &ts, false);
Task heartbeatTask(TASK_IMMEDIATE, TASK_FOREVER, &heartbeat, &ts, false);
Task buzzerOnTask(TASK_IMMEDIATE, TASK_ONCE, &buzzerOn, &ts, false);
Task buzzerOffTask(BUZZER_ON_TIME_MS, TASK_ONCE, &buzzerOff, &ts, false);
Task ledOnTask(TASK_IMMEDIATE, TASK_ONCE, &ledOn, &ts, false);
Task ledOffTask(LED_ON_TIME_MS, TASK_ONCE, &ledOff, &ts, false);
Task playThorSongTask(TASK_IMMEDIATE, TASK_ONCE, &playThorSong, &ts, false);
Task stopThorSongTask(TASK_IMMEDIATE, TASK_ONCE, &stopThorSong, &ts, false);
int channel;
bool buzzerBusy = false;
bool ledBusy = false;
bool thorSongPlaying = false;
int thorSongNote = 0;
bool onboardLedState = 0;
int numberOfNodes = 1;

void setup() {
  #ifdef DEBUG
    Serial.begin(115200);
  #endif

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_BUTTON_PIN, INPUT);
  pinMode(LED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(DIP_SWITCH_PIN1, INPUT_PULLUP);
  pinMode(DIP_SWITCH_PIN2, INPUT_PULLUP);
  pinMode(DIP_SWITCH_PIN3, INPUT_PULLUP);
  pinMode(DIP_SWITCH_PIN4, INPUT_PULLUP);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(ONBOARD_BUTTON_PIN, INPUT);
  
  digitalWrite(ONBOARD_LED_PIN, HIGH); // Onboard LED off
  digitalWrite(BUZZER_PIN, HIGH); // Buzzer off
  digitalWrite(LED_PIN, LOW); // External LED off
  
  // Initialize the mesh
  mesh.init(MESH_SSID, MESH_PASSWORD, &ts, MESH_PORT);
  mesh.onReceive(&receivedMessageCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);

  heartbeatTask.enable(); // Enable the heartbeat
  dipSwitchCallback(); // Initialize the channel

  // Everything is now intitialized, attach interrupts last
  attachInterrupt(digitalPinToInterrupt(BUZZER_BUTTON_PIN), handleBuzzerButtonInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LED_BUTTON_PIN), handleLedButtonInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DIP_SWITCH_PIN1), handleDipSwitchInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DIP_SWITCH_PIN2), handleDipSwitchInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DIP_SWITCH_PIN3), handleDipSwitchInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(DIP_SWITCH_PIN4), handleDipSwitchInterrupt, CHANGE);
  #ifdef VOLUNDR
    attachInterrupt(digitalPinToInterrupt(ONBOARD_BUTTON_PIN), handleOnboardButtonInterrupt, CHANGE);
  #endif
}

void loop() {
  mesh.update(); // Internally this will also call ts.execute()
}

// Function to handle the buzzer button interrupt
// The button is debounced by restarting the task delayed on every change
ICACHE_RAM_ATTR void handleBuzzerButtonInterrupt() {
  buzzerButtonTask.restartDelayed(BUTTON_DEBOUNCE_TIME_MS);

  PRINT("Entered buzzer button interrupt handler.\n");
}

// Function to handle the LED button interrupt
// The button is debounced by restarting the task delayed on every change
ICACHE_RAM_ATTR void handleLedButtonInterrupt() {
  ledButtonTask.restartDelayed(BUTTON_DEBOUNCE_TIME_MS);

  PRINT("Entered LED button interrupt handler.\n");
}

// Function to handle the DIP switch interrupt
// The DIP switch is debounced by restarting the task delayed on every change
ICACHE_RAM_ATTR void handleDipSwitchInterrupt() {
  dipSwitchTask.restartDelayed(SWITCH_DEBOUNCE_TIME_MS);

  PRINT("Entered DIP switch interrupt handler.\n");
}

// Function to handle the onboard button interrupt
// The button is debounced by restarting the task delayed on every change
ICACHE_RAM_ATTR void handleOnboardButtonInterrupt() {
  onboardButtonTask.restartDelayed(BUTTON_DEBOUNCE_TIME_MS);

  PRINT("Entered onboard button interrupt handler.\n");
}

// Function that is called when the buzzer button changes value (already debounced)
void buzzerButtonCallback() {
  int buttonState = digitalRead(BUZZER_BUTTON_PIN);

  if(buttonState == 0) { // Button pressed down
    if(buzzerBusy == false && thorSongPlaying == false) {
      broadcastAction("B"); // Send message to turn on buzzer
    }
  }

  PRINT("Entered buzzer button callback. Value of button: ");
  PRINT(buttonState);
  PRINT(".\n");
}

// Function that is called when the LED button changes value (already debounced)
void ledButtonCallback() {
  int buttonState = digitalRead(LED_BUTTON_PIN);

  if(buttonState == 0) { // Button pressed down
    if(ledBusy == false) {
      broadcastAction("L"); // Send message to turn on LED
    }
  }

  PRINT("Entered LED button callback. Value of button: ");
  PRINT(buttonState);
  PRINT(".\n");
}

// Function that is called when the DIP switch changes value (already debounced)
void dipSwitchCallback() {
  bool dipSwitch1 = !digitalRead(DIP_SWITCH_PIN1);
  bool dipSwitch2 = !digitalRead(DIP_SWITCH_PIN2);
  bool dipSwitch3 = !digitalRead(DIP_SWITCH_PIN3);
  bool dipSwitch4 = !digitalRead(DIP_SWITCH_PIN4);
  channel = byte(dipSwitch1)<<3 | byte(dipSwitch2)<<2 | byte(dipSwitch3)<<1 | byte(dipSwitch4);

  PRINT("Entered DIP switch callback. Value of switch: ");
  PRINT(channel);
  PRINT(" (");
  PRINT(dipSwitch1);
  PRINT(dipSwitch2);
  PRINT(dipSwitch3);
  PRINT(dipSwitch4);
  PRINT(").\n");
}

// Function that is called when the onboard button changes value (already debounced) (only if VOLUNDR is defined)
void onboardButtonCallback() {
  int buttonState = digitalRead(ONBOARD_BUTTON_PIN);
  if(buttonState == 0) { // Button pressed down
    if(thorSongPlaying == false) {
      broadcastAction("T"); // Send message to play the Thor song
    }
    else {
      broadcastAction("S"); // Send message to stop the Thor song earlier
    }
  }

  PRINT("Entered onboard button callback. Value of button: ");
  PRINT(buttonState);
  PRINT(".\n");
}

// Function that sends a message to all nodes in the mesh (including itself)
// Sends the action the node needs to perform, on which time and at which channel
void broadcastAction(String action) {
  uint32_t actionTime = mesh.getNodeTime() + (RESPONSE_DELAY_MS * 1000);
  String message = String(actionTime) + "." + action + "." + String(channel);

  PRINT("Send message: \"");
  PRINT(message);
  PRINT("\".\n");

  mesh.sendBroadcast(message, true);
}

// Function that is called when this node receives a message
void receivedMessageCallback(uint32_t from, String &message) {
  PRINT("Received message: \"");
  PRINT(message);
  PRINT("\".\n");

  // Parse message
  int size = message.length() + 1;
  char buffer[size];
  message.toCharArray(buffer, size);
  uint32_t actionTime = strtoul(strtok(buffer, "."), NULL, 10);
  String action = strtok(NULL, ".");
  int targetChannel = atoi(strtok(NULL, "."));

  uint32_t nodeTime = mesh.getNodeTime();
  int delay = (actionTime - nodeTime) / 1000;

  if(action == "T") { // Play Thor song works on all channels
    if(actionTime < nodeTime) {
      PRINT("To late to play the Thor song..");
    }
    else {
      thorSongPlaying = true;
      playThorSongTask.restartDelayed(delay);

      PRINT("Started play Thor song task with delay of ");
      PRINT(delay)
      PRINT(" ms.");
    }
  }
  else if(action == "S") { // Stop Thor song works on all channels
    if(actionTime < nodeTime) {
      PRINT("To late to stop the Thor song..");
    }
    else {
      stopThorSongTask.restartDelayed(delay);

      PRINT("Started stop Thor song task with delay of ");
      PRINT(delay)
      PRINT(" ms.");
    }
  }
  else if(channel == targetChannel) { // LED and buzzer signal only work on the same channel
    if(action == "B" && buzzerBusy == false && thorSongPlaying == false) {
      if(actionTime < nodeTime) {
        PRINT("To late to turn on buzzer..");
      }
      else {
        buzzerBusy = true;
        buzzerOnTask.restartDelayed(delay);

        PRINT("Started buzzer on task with delay of ");
        PRINT(delay)
        PRINT(" ms.");
      }
    }
    else if(action == "L" && ledBusy == false) {
      if(actionTime < nodeTime) {
        PRINT("To late to turn on LED..");
      }
      else {
        ledBusy = true;
        ledOnTask.restartDelayed(delay);

        PRINT("Started LED on task with delay of ");
        PRINT(delay)
        PRINT(" ms.");
      }
    }
  }

  PRINT(" (nodeTime ");
  PRINT(nodeTime);
  PRINT(") (actionTime ");
  PRINT(actionTime);
  PRINT(")\n");
}

// Function that is called when the number of nodes in the mesh changes
void changedConnectionsCallback() {
  numberOfNodes = mesh.getNodeList().size() + 1;
}

// Function that gives a heartbeat on the onboard LED that is synchronised between all nodes of the mesh
// Heartbeat is only shown if there is more than one node in the mesh
void heartbeat() {
  if(onboardLedState) { // Onboard LED is on
    digitalWrite(ONBOARD_LED_PIN, HIGH); // Turn LED off

    // Base delay on node time to synchronize the heartbeats between all nodes of the mesh:
    int interval = HEARTBEAT_OFF_TIME_MS - (mesh.getNodeTime() % (HEARTBEAT_TIME_MS*1000)) / 1000;
    heartbeatTask.setInterval(interval);
    
    #ifdef DEBUG_HEARTBEAT
      PRINT("Onboard LED turned off! Heartbeat task interval: ");
      PRINT(interval);
      PRINT(".\n");
    #endif
  }
  else { // Onboard LED is off
    if(numberOfNodes > 1) { // Only actually show the heartbeat if you are not allone
      digitalWrite(ONBOARD_LED_PIN, LOW); // Turn LED on
    }
    heartbeatTask.setInterval(HEARTBEAT_ON_TIME_MS);

    #ifdef DEBUG_HEARTBEAT
      PRINT("Onboard LED turned on! Heartbeat task interval: ");
      PRINT(HEARTBEAT_ON_TIME_MS);
      PRINT(".\n");
    #endif
  }
  onboardLedState = !onboardLedState;
}

// Function that turns the buzzer on and sets a task to turn it back off
void buzzerOn() {
  int frequency = (channel + 4) * 30; // Give every channel a different frequency
  tone(BUZZER_PIN, frequency);
  buzzerOffTask.restartDelayed(BUZZER_ON_TIME_MS);
  
  PRINT("Buzzer turned on with a frequency of ");
  PRINT(frequency);
  PRINT(" Hz!\n")
}

// Function that turns the buzzer off
void buzzerOff() {
  // It is possible that the Thor song gets played in the meantime, then the buzzer should not be turned off
  if(thorSongPlaying == false) {
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, HIGH); // Keep the pin high to reduce noise
  }
  buzzerBusy = false;

  PRINT("Buzzer turned off!\n");
}

// Function that turns the LED on and sets a task to turn it back off
void ledOn() {
  digitalWrite(LED_PIN, HIGH);
  ledOffTask.restartDelayed(LED_ON_TIME_MS);

  PRINT("LED turned on!\n");
}

// Function that turns the LED off
void ledOff() {
  digitalWrite(LED_PIN, LOW);
  ledBusy = false;

  PRINT("LED turned off!\n");
}

// Function that plays the Thor song
void playThorSong() {
  if(thorSongNote == 0) {
    noTone(BUZZER_PIN); // Make sure there is no tone playing at the start of the Thor song

    PRINT("Now playing Thor song!\n");
  }
  tone(BUZZER_PIN, thorSongMelody[thorSongNote], thorSongDuration[thorSongNote]);
  thorSongNote += 1;
  if(thorSongNote > THOR_SONG_LENGTH) { // Last note is played
    stopThorSong();
  }
  else { // Still more notes to come, schedule the next one
    playThorSongTask.restartDelayed(thorSongDuration[thorSongNote-1] + BETWEEN_NOTE_DELAY_MS);
  }
}

// Function that stops the Thor song (earlier)
void stopThorSong() {
  playThorSongTask.disable();
  noTone(BUZZER_PIN);
  digitalWrite(BUZZER_PIN, HIGH); // Keep the pin high to reduce noise
  thorSongNote = 0;
  thorSongPlaying = false;

  PRINT("Playing Thor song stopped :(\n");
}