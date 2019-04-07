#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <elapsedMillis.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "configure_wifi.h"
#include "mqtt.h"

char mqtt_server[40] = "";
char mqtt_port[16] = "1883";
char mqtt_user[64] = "";
char mqtt_password[64] = "";
char mqtt_power_state_topic[64] = "";
char mqtt_power_set_topic[64] = "";
char mqtt_speed_state_topic[64] = "";
char mqtt_speed_set_topic[64] = "";
char mqtt_osc_state_topic[64] = "";
char mqtt_osc_set_topic[64] = "";
char mqtt_device[64] = "";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Ticker ticker;
FanSpeeds fanSpeed = unknown;
FanSpeeds currentSpeedSwitchSetting = unknown;
boolean fanOn = false;
boolean oscOn = false;
unsigned long lastAnalogRead = 0L;

void tick() {
  //toggle state
  int state = digitalRead(CONFIG_PIN_STATUS);  // get the current state of GPIO1 pin
  digitalWrite(CONFIG_PIN_STATUS, !state);     // set pin to the opposite state
}

void setup() {
  Serial.begin(115200);

  //set led pin as output
  pinMode(CONFIG_PIN_STATUS, OUTPUT);
  digitalWrite(CONFIG_PIN_STATUS, HIGH); // Turn off the on-board LED

  //use flash button as input
  pinMode(D3, INPUT);

  Serial.println("Initialising pins");
  pinMode(FAN_LOW_SPEED_PIN, OUTPUT);
  digitalWrite(FAN_LOW_SPEED_PIN, LOW);
  pinMode(FAN_MED_SPEED_PIN, OUTPUT);
  digitalWrite(FAN_MED_SPEED_PIN, LOW);
  pinMode(FAN_HIGH_SPEED_PIN, OUTPUT);
  digitalWrite(FAN_HIGH_SPEED_PIN, LOW);
  pinMode(FAN_LOUVRE_ROTATE_PIN, OUTPUT);
  digitalWrite(FAN_LOUVRE_ROTATE_PIN, LOW);
  pinMode(A0, INPUT);

  configureWifi(true);

  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(processMessage);

  currentSpeedSwitchSetting = readSpeedSwitch();
  Serial.print("Initial setting for speed switch is: ");
  debugSpeed(currentSpeedSwitchSetting);
  Serial.println();
}

void loop() {
  // Check for user pressing the onboard NodeMCU/ESP-12E "FLASH" button, which means reconfigure
  if (digitalRead(D3) == LOW) {
    configureWifi(false);
  }

  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  } else {
    if (!mqttClient.connected()) {
      reconnectMqtt();
    }
    if (mqttClient.connected()) {
      mqttClient.loop();
    }
  }

  updateFanSpeedFromSwitch();
}

void updateFanSpeedFromSwitch() {
  // Only check A0 at most 10 times per second, or otherwise wifi disconnects :(
  unsigned long currentMillis = millis();
  if ((currentMillis != lastAnalogRead) &&
      (currentMillis % 100 == 0)) {
    lastAnalogRead = currentMillis;
    FanSpeeds speedSwitchSetting = readSpeedSwitch();

    if (speedSwitchSetting != currentSpeedSwitchSetting) {
      Serial.print("Speed switch changed to ");
      debugSpeed(speedSwitchSetting);
      Serial.println();
      updateFanSpeed(speedSwitchSetting);
      currentSpeedSwitchSetting = speedSwitchSetting;
    }
  }
}

FanSpeeds readSpeedSwitch() {
  FanSpeeds speedSwitchSetting = unknown;
  pinMode(A0, INPUT);
  int value = analogRead(A0);
  if (value < 130) {
    speedSwitchSetting = off; // Expected value of off is about 0
  } else if (value < 550) {
    speedSwitchSetting = low; // Expected value of low is about 360
  } else if (value < 870) {
    speedSwitchSetting = medium; // Expected value of medium is about 870
  } else {
    speedSwitchSetting = high; // Expected value of high is about 1024;
  }

  return speedSwitchSetting;
}

void debugSpeed(FanSpeeds value) {
  if (value == unknown) {
    Serial.print("unknown");
  } else if (value == off) {
    Serial.print("off");
  } else if (value == low) {
    Serial.print("low");
  } else if (value == medium) {
    Serial.print("medium");
  } else {
    Serial.print("high");
  }
}

void controlRelays(boolean powerEnabled, boolean oscEnabled, FanSpeeds newSpeed) {
  if (!powerEnabled || newSpeed == off) {
    Serial.println("All off");
    digitalWrite(FAN_LOW_SPEED_PIN, LOW);
    digitalWrite(FAN_MED_SPEED_PIN, LOW);
    digitalWrite(FAN_HIGH_SPEED_PIN, LOW);
    digitalWrite(FAN_LOUVRE_ROTATE_PIN, LOW);
  } else {
    if (oscEnabled) {
      Serial.println("Louvres enabled");
      digitalWrite(FAN_LOUVRE_ROTATE_PIN, HIGH);
    } else {
      Serial.println("Louvres disabled");
      digitalWrite(FAN_LOUVRE_ROTATE_PIN, LOW);
    }

    if (newSpeed == low) {
      digitalWrite(FAN_MED_SPEED_PIN, LOW);
      digitalWrite(FAN_HIGH_SPEED_PIN, LOW);
      digitalWrite(FAN_LOW_SPEED_PIN, HIGH);
    } else if (newSpeed == medium) {
      digitalWrite(FAN_HIGH_SPEED_PIN, LOW);
      digitalWrite(FAN_LOW_SPEED_PIN, LOW);
      digitalWrite(FAN_MED_SPEED_PIN, HIGH);
    } else if (newSpeed == high) {
      digitalWrite(FAN_LOW_SPEED_PIN, LOW);
      digitalWrite(FAN_MED_SPEED_PIN, LOW);
      digitalWrite(FAN_HIGH_SPEED_PIN, HIGH);
    }
  }
}

void updateFanPower(boolean enabled) {
  bool updateFanSpeed = false;
  bool updateOsc = false;

  if (enabled && fanSpeed == unknown) {
    // go to high!
    fanSpeed = high;
    updateFanSpeed = true;
  }

  fanOn = enabled;
  if (!enabled && oscOn) {
    oscOn = false;
    updateOsc = true;
  }

  controlRelays(fanOn, oscOn, fanSpeed);
  sendPowerState(fanOn);
  if (updateOsc) {
    sendOscState(oscOn);
  }
  if (updateFanSpeed) {
    sendSpeedState(fanSpeed);
  }
}

void updateFanSpeed(FanSpeeds newSpeed) {
  if (newSpeed == unknown) {
    return;
  }

  if (newSpeed == off) {
    // Changed to off
    fanOn = false;
    oscOn = false;
  } else if (!fanOn) {
    // Changed from off to on
    fanOn = true;
  }
  fanSpeed = newSpeed;
  controlRelays(fanOn, oscOn, newSpeed);
  sendSpeedState(fanSpeed);
}

void updateFanOsc(boolean enabled) {
  oscOn = enabled && fanOn;
  controlRelays(fanOn, oscOn, fanSpeed);
  sendOscState(oscOn);
}

