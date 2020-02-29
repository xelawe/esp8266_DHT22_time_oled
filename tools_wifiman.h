/* WifiMan */

#include <ESP8266WiFi.h>
//needed for Wifi Manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//flag for saving data
bool shouldSaveConfig = false;


//callback notifying us of the need to save config
void saveConfigCallback () {
  //Serial.println("Should save config");
  dis_stat_prog( "Should save config");
  shouldSaveConfig = true;
}

void WifimanAPcb (WiFiManager *myWiFiManager) {
  dis_stat_prog( "Wifi config mode");
  dis_stat_prog( myWiFiManager->getConfigPortalSSID());
}

void init_wifi() {

  //read configuration from FS json
  dis_stat_prog("mounting FS...", 5);

  if (SPIFFS.begin()) {
    dis_stat_prog("mounted file system", 6);
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      dis_stat_prog("reading config file", 7);
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {

        dis_stat_prog("opened config file", 8);
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
#ifdef serdebug
        json.printTo(Serial);
#endif
        if (json.success()) {
          //Serial.println("\nparsed json");
          dis_stat_prog("parsed json");

          strcpy(gv_sensnbr_t_c, json["sens_nbr_t"]);
          strcpy(gv_sensnbr_h_c, json["sens_nbr_h"]);
          gv_sens_nbr_t = atoi(gv_sensnbr_t_c);
          gv_sens_nbr_h = atoi(gv_sensnbr_h_c);
          if (json.containsKey("LDRThres")) {
            //Serial.println("Key found in file");
            strcpy(gv_LDRThres_c, json["LDRThres"]);
            gv_LDRThres = atoi(gv_LDRThres_c);
          } else {
            //Serial.println("Key not in file");
            itoa(gv_LDRThres, gv_LDRThres_c, 3);
          }
        } else {
          dis_stat_prog("failed to load json config");
        }
      }
    }
  } else {
    dis_stat_prog("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_sens_nbr_t("sens_nbr_t", "sens nbr t", gv_sensnbr_t_c, 3);
  WiFiManagerParameter custom_sens_nbr_h("sens_nbr_h", "sens nbr h", gv_sensnbr_h_c, 3);
  WiFiManagerParameter custom_LDRThres("LDRThres", "LDR Thres", gv_LDRThres_c, 3);


  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
#ifdef serdebug
#else
  wifiManager.setDebugOutput(false);
#endif
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_sens_nbr_t);
  wifiManager.addParameter(&custom_sens_nbr_h);
  wifiManager.addParameter(&custom_LDRThres);

  //reset saved settings
  //wifiManager.resetSettings();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);

  dis_stat_prog("WiFi: wait to connect", 20);

  //fetches ssid and pass and tries to connect
  wifiManager.setAPCallback(WifimanAPcb);
  //if it does not connect it starts an access point
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    dis_stat_prog("failed to connect");
    //Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  dis_stat_prog("connected...yeey", 60);


  //read updated parameters
  strcpy(gv_sensnbr_t_c, custom_sens_nbr_t.getValue());
  strcpy(gv_sensnbr_h_c, custom_sens_nbr_h.getValue());
  strcpy(gv_LDRThres_c, custom_LDRThres.getValue());
  gv_sens_nbr_t = atoi(gv_sensnbr_t_c);
  gv_sens_nbr_h = atoi(gv_sensnbr_h_c);
  gv_LDRThres = atoi(gv_LDRThres_c);
  //Serial.println(gv_LDRThres);

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    dis_stat_prog("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["sens_nbr_t"] = gv_sensnbr_t_c;
    json["sens_nbr_h"] = gv_sensnbr_h_c;
    json["LDRThres"] = gv_LDRThres_c;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      dis_stat_prog("failed to open config file for writing");
      //  Serial.println("failed to open config file for writing");
    }

#ifdef serdebug
    json.printTo(Serial);
#endif
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}

