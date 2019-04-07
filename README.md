# mqtt-fan
An ESP8266 project to control a 3 speed fan with an oscillate motor.

## Config

The code is designed to power 4 relays. The fan motor has 3 windings (for each of the 3 speeds),
with one relay to control each winding. There is a 4th relay to turn the oscillate motor on/of.

Edit config.h to configure which pins you connected the relay input to.

The code blinks the status LED to indicate if it is connecting to WiFi, and if it is connecting
to MQTT. If you want to use an external LED (i.e. not the onboard one) change CONFIG_PIN_STATUS to
be the pin the LED is connected to. Change CONFIG_LED_ON and CONFIG_LED_OFF to indicate if this
pin should be LOW or HIGH to turn your LED on or off. The default uses the onboard LED.

### Speed switch

In the function readSpeedSwitch() in mqtt-fan.ino it is trying to determine where you set the switch
to based on the input voltage. My original circuit has a 1k resisitor between 0V and the low speed setting,
1k between low and medium, and 1k between medium and high. The high setting then connects to +3.3V. (Note that
with the NodeMCU board the analog input should be between 0-3V3, but with the raw ESP modele itself, this is
between 0-1V) The values it is checking for in this function were the approximate midpoints between the expected
readings I got for each of the swtich settings. If your circuit is different you might need different values here.

## WiFi Setup

When first powered up the flash config will not be configured. The device will go into access point
mode so you can configure it. The access point name will be like FanControlab12cd34. Connect to the access
point (e.g. on your phone), and you will get the configuration form.

Tap the link to scan for access points. Choose the one you want it to connect to, and enter the password for
that access point.

There are a number of other things you need to configure:
* MQTT Server: This is the address (e.g. IP) of the MQTT server on your network.
* MQTT Port: The port number that the MQTT server listens on. (normally 1833)
* MQTT User: The user name to log in to the MQTT server as.
* MQTT Password: The password to log in to the MQTT server with.
* MQTT Power State Topic: The topic name that the fan will use to tell Home Assistant whether the power is on/off.
* MQTT Power Set Topic: The topic name that Home Assistant will use to tell the fan to turn the power on/off.
* MQTT Speed State Topic: The topic name that the fan will use to tell Home Assistant what speed it is running at.
* MQTT Speed Set Topic: The topic name that Home Assistant will use to tell the fan what speed to run at.
* MQTT Osc State Topic: The topic name that the fan will use to tell Home Assistant whether the oscillate motor is on/off.
* MQTT Osc Set Topic: The topic name that Home Assistant will use to tell the fan to turn the oscillate motor on/off.
* MQTT Device: The device name the fan will identify itself as to MQTT.

Once you save the config, the controller will restart and try to connect to the access point.

Remember slow blinking means it is trying to connect to WiFi. Fast blinking means it is trying to connect to MQTT.

If something goes wrong, hold down the flash button on the board until you get slow blinking. This will restart the wifi
setup. You can also use this if you need to change topic names or WiFI details.

## Home Assistant Setup

In Home Assistant you need to tell it the topic names so that it can control the fan. The configuration will look like this:

```
fan:
  - platform: mqtt
    state_topic: "ha/boxfan/on/state"
    command_topic: "ha/boxfan/on/set"
    oscillation_state_topic: "ha/boxfan/osc/state"
    oscillation_command_topic: "ha/boxfan/osc/set"
    speed_state_topic: "ha/boxfan/speed/state"
    speed_command_topic: "ha/boxfan/speed/set"
    name: Box Fan
```

Note that if you want to have multiple fans, just use different topics for each fan.