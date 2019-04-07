#ifndef _config_h
#define _config_h

#include <Ticker.h>
#include <PubSubClient.h>

// Pins
// D1
//#define CONFIG_PIN_RED 5
#define FAN_LOW_SPEED_PIN 5
// D2
//#define CONFIG_PIN_GREEN 4
#define FAN_MED_SPEED_PIN 4
// D3 is the flash button on the ESP-12E/NodeMCU board
// D4 is the onboard LED
// D5
#define FAN_HIGH_SPEED_PIN 14
// D6
#define FAN_LOUVRE_ROTATE_PIN 12

// This is used to flash and report status.
#define CONFIG_PIN_STATUS BUILTIN_LED
// D7
//#define CONFIG_PIN_STATUS 13

// What value to set to turn the config LED on/off.
#define CONFIG_LED_ON LOW
#define CONFIG_LED_OFF HIGH

enum FanSpeeds {unknown, off, low, medium, high};

extern Ticker ticker;
extern void tick();

extern char mqtt_server[];
extern char mqtt_port[];
extern char mqtt_user[];
extern char mqtt_password[];
extern char mqtt_power_state_topic[];
extern char mqtt_power_set_topic[];
extern char mqtt_speed_state_topic[];
extern char mqtt_speed_set_topic[];
extern char mqtt_osc_state_topic[];
extern char mqtt_osc_set_topic[];
extern char mqtt_device[];

extern PubSubClient mqttClient;

extern void updateFanPower(boolean enabled);
extern void updateFanSpeed(FanSpeeds newSpeed);
extern void updateFanOsc(boolean enabled);

extern FanSpeeds readSpeedSwitch();
extern void debugSpeed(FanSpeeds value);
extern void updateFanSpeedFromSwitch();

#endif _config_h
