#include <FS.h> // Make sure this is first

#include <Arduino.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <elapsedMillis.h>

#include "config.h"

#define WIFI_RECONNECT_POLLING_PERIOD 5000

elapsedMillis wifiReconnectTimeElapsed;

bool shouldSaveConfig = false;

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

void readParameter(JsonObject& json, char* holder, const char *paramName) {
  if (json.containsKey(paramName)) {
    strcpy(holder, json[paramName]);
  } else {
    Serial.print("Error: Config file did not contain a value for ");
    Serial.println(paramName);
  }
}

void readConfigFromFilesystem() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("parsed json");

          readParameter(json, mqtt_server, "mqtt_server");
          readParameter(json, mqtt_port, "mqtt_port");
          readParameter(json, mqtt_user, "mqtt_user");
          readParameter(json, mqtt_password, "mqtt_password");
          readParameter(json, mqtt_power_state_topic, "mqtt_power_state_topic");
          readParameter(json, mqtt_power_set_topic, "mqtt_power_set_topic");
          readParameter(json, mqtt_speed_state_topic, "mqtt_speed_state_topic");
          readParameter(json, mqtt_speed_set_topic, "mqtt_speed_set_topic");
          readParameter(json, mqtt_osc_state_topic, "mqtt_osc_state_topic");
          readParameter(json, mqtt_osc_set_topic, "mqtt_osc_set_topic");
          readParameter(json, mqtt_device, "mqtt_device");
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end 
}

void writeConfigToFilesystem() {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_password"] = mqtt_password;
    json["mqtt_power_state_topic"] = mqtt_power_state_topic;
    json["mqtt_power_set_topic"] = mqtt_power_set_topic;
    json["mqtt_speed_state_topic"] = mqtt_speed_state_topic;
    json["mqtt_speed_set_topic"] = mqtt_speed_set_topic;
    json["mqtt_osc_state_topic"] = mqtt_osc_state_topic;
    json["mqtt_osc_set_topic"] = mqtt_osc_set_topic;
    json["mqtt_device"] = mqtt_device;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
}

void configureWifi(bool autoConfig) {
  readConfigFromFilesystem();
  
  shouldSaveConfig = false;
  
  ticker.attach(0.2, tick);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "MQTT server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT user", mqtt_user, 63);
  WiFiManagerParameter custom_mqtt_password("pass", "MQTT password", mqtt_password, 63);
  WiFiManagerParameter custom_mqtt_power_state_topic("mqtt_power_state_topic", "Power state topic", mqtt_power_state_topic, 63);
  WiFiManagerParameter custom_mqtt_power_set_topic("mqtt_power_set_topic", "Power set topic", mqtt_power_set_topic, 63);
  WiFiManagerParameter custom_mqtt_speed_state_topic("mqtt_speed_state_topic", "Speed state topic", mqtt_speed_state_topic, 63);
  WiFiManagerParameter custom_mqtt_speed_set_topic("mqtt_speed_set_topic", "Speed set topic", mqtt_speed_set_topic, 63);
  WiFiManagerParameter custom_mqtt_osc_state_topic("mqtt_osc_state_topic", "Osc state topic", mqtt_osc_state_topic, 63);
  WiFiManagerParameter custom_mqtt_osc_set_topic("mqtt_osc_set_topic", "Osc set topic", mqtt_osc_set_topic, 63);
  WiFiManagerParameter custom_mqtt_device("device", "MQTT device", mqtt_device, 63);
 
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  
  //set minimu quality of signal so it ignores APs under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_power_set_topic);
  wifiManager.addParameter(&custom_mqtt_power_state_topic);
  wifiManager.addParameter(&custom_mqtt_speed_set_topic);
  wifiManager.addParameter(&custom_mqtt_speed_state_topic);
  wifiManager.addParameter(&custom_mqtt_osc_set_topic);
  wifiManager.addParameter(&custom_mqtt_osc_state_topic);
  wifiManager.addParameter(&custom_mqtt_device);

  String apName = "FanControl";
  apName += WiFi.softAPmacAddress();
  apName.replace(":", "");
  if (autoConfig) {
    Serial.println("Auto connect to wifi");
    if (!wifiManager.autoConnect(apName.c_str())) {
      Serial.println("Auto connect failed");
      delay(3000);
      // if autoConfig is true, then we are at startup, so reset and try again
      ESP.reset();
      delay(5000);
    }
  } else {
    Serial.println("Reconfiguring...");
    if (!wifiManager.startConfigPortal(apName.c_str())) {
      Serial.println("Reconfigure failed to connect to wifi");
      shouldSaveConfig = false;
    }
  }

  ticker.detach();
  digitalWrite(CONFIG_PIN_STATUS, CONFIG_LED_OFF);
  
  if (shouldSaveConfig) {
    //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_password, custom_mqtt_password.getValue());
    strcpy(mqtt_power_state_topic, custom_mqtt_power_state_topic.getValue());
    strcpy(mqtt_power_set_topic, custom_mqtt_power_set_topic.getValue());
    strcpy(mqtt_speed_state_topic, custom_mqtt_speed_state_topic.getValue());
    strcpy(mqtt_speed_set_topic, custom_mqtt_speed_set_topic.getValue());
    strcpy(mqtt_osc_state_topic, custom_mqtt_osc_state_topic.getValue());
    strcpy(mqtt_osc_set_topic, custom_mqtt_osc_set_topic.getValue());
    strcpy(mqtt_device, custom_mqtt_device.getValue());
  
    Serial.print("MQTT Server: ");
    Serial.println(mqtt_server);
    Serial.print("MQTT Port: ");
    Serial.println(mqtt_port);
    Serial.print("MQTT User: ");
    Serial.println(mqtt_user);
    Serial.print("MQTT Password: ");
    Serial.println(mqtt_password);
    Serial.print("MQTT Power State Topic: ");
    Serial.println(mqtt_power_state_topic);
    Serial.print("MQTT Power Set Topic: ");
    Serial.println(mqtt_power_set_topic);
    Serial.print("MQTT Speed State Topic: ");
    Serial.println(mqtt_speed_state_topic);
    Serial.print("MQTT Speed Set Topic: ");
    Serial.println(mqtt_speed_set_topic);
    Serial.print("MQTT Osc State Topic: ");
    Serial.println(mqtt_osc_state_topic);
    Serial.print("MQTT Osc Set Topic: ");
    Serial.println(mqtt_osc_set_topic);
    Serial.print("MQTT Device: ");
    Serial.println(mqtt_device);

    writeConfigToFilesystem();
  } else {
    Serial.println("Config was not updated");
  }
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiReconnectTimeElapsed > WIFI_RECONNECT_POLLING_PERIOD) {
      wifiReconnectTimeElapsed = 0;
      Serial.print("Reconnecting to WiFi...");
      ticker.attach(0.2, tick);
      WiFi.begin();
      digitalWrite(CONFIG_PIN_STATUS, CONFIG_LED_OFF);
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi");        
      } else {
        Serial.println("WiFi Connected");   
        ticker.detach();     
      }
    }
  }
}
