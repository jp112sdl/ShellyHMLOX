#define JSONCONFIG_IP                    "ip"
#define JSONCONFIG_NETMASK               "netmask"
#define JSONCONFIG_GW                    "gw"
#define JSONCONFIG_CCUIP                 "ccuip"
#define JSONCONFIG_SHELLY                "shelly"
#define JSONCONFIG_LOXUDPPORT            "loxudpport"
#define JSONCONFIG_LOXUSERNAME           "loxusername"
#define JSONCONFIG_LOXPASSWORD           "loxpassword"
#define JSONCONFIG_HMPOWERVARIABLE       "powervariable"
#define JSONCONFIG_BACKENDTYPE           "backendtype"
#define JSONCONFIG_RESTOREOLDSTATE       "restoreOldState"
#define JSONCFONIG_GPIO5MODE             "gpio5mode"
#define JSONCFONIG_GPIO5ASSENDER         "gpio5assender"

bool loadSystemConfig() {
  DEBUG(F("loadSystemConfig mounting FS..."));
  if (SPIFFS.begin()) {
    DEBUG(F("loadSystemConfig mounted file system"));
    if (SPIFFS.exists("/" + configJsonFile)) {
      DEBUG(F("loadSystemConfig reading config file"));
      File configFile = SPIFFS.open("/" + configJsonFile, "r");
      if (configFile) {
        DEBUG(F("loadSystemConfig opened config file"));
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument doc;
        DeserializationError error = deserializeJson(doc, buf.get());
        if (error) {
          DEBUG(F("loadSystemConfig JSON DeserializationError"));
          return false;
        }
        JsonObject json = doc.as<JsonObject>();
        DEBUG("Content of JSON Config-File: /" + configJsonFile);
#ifdef SERIALDEBUG
        serializeJson(doc, Serial);
        Serial.println();
#endif
        DEBUG("\nJSON OK");
        ((json[JSONCONFIG_IP]).as<String>()).toCharArray(ShellyNetConfig.ip, IPSIZE);
        ((json[JSONCONFIG_NETMASK]).as<String>()).toCharArray(ShellyNetConfig.netmask, IPSIZE);
        ((json[JSONCONFIG_GW]).as<String>()).toCharArray(ShellyNetConfig.gw, IPSIZE);
        ((json[JSONCONFIG_CCUIP]).as<String>()).toCharArray(GlobalConfig.ccuIP, IPSIZE);
        ((json[JSONCONFIG_SHELLY]).as<String>()).toCharArray(GlobalConfig.DeviceName, VARIABLESIZE);

        //((json[JSONCONFIG_LOXUSERNAME]).as<String>()).toCharArray(LoxoneConfig.Username, VARIABLESIZE);
        //((json[JSONCONFIG_LOXPASSWORD]).as<String>()).toCharArray(LoxoneConfig.Password, VARIABLESIZE);
        ((json[JSONCONFIG_LOXUDPPORT]).as<String>()).toCharArray(LoxoneConfig.UDPPort, 10);

        GlobalConfig.BackendType = json[JSONCONFIG_BACKENDTYPE];
        GlobalConfig.GPIO5Mode = json[JSONCFONIG_GPIO5MODE];
        GlobalConfig.restoreOldRelayState = json[JSONCONFIG_RESTOREOLDSTATE];
        GlobalConfig.GPIO5asSender = json[JSONCFONIG_GPIO5ASSENDER];
        GlobalConfig.Hostname = "Shelly-" + String(GlobalConfig.DeviceName);
      }
      return true;
    } else {
      DEBUG("/" + configJsonFile + " not found.");
      return false;
    }
    SPIFFS.end();
  } else {
    DEBUG(F("loadSystemConfig failed to mount FS"));
    return false;
  }
}

bool saveSystemConfig() {
  SPIFFS.begin();
  DEBUG(F("saving config"));
  DynamicJsonDocument doc;
  JsonObject json = doc.to<JsonObject>();
  json[JSONCONFIG_IP] = ShellyNetConfig.ip;
  json[JSONCONFIG_NETMASK] = ShellyNetConfig.netmask;
  json[JSONCONFIG_GW] = ShellyNetConfig.gw;
  json[JSONCONFIG_CCUIP] = GlobalConfig.ccuIP;
  json[JSONCONFIG_SHELLY] = GlobalConfig.DeviceName;
  json[JSONCONFIG_RESTOREOLDSTATE] = GlobalConfig.restoreOldRelayState;
  json[JSONCONFIG_BACKENDTYPE] = GlobalConfig.BackendType;
  //json[JSONCONFIG_LOXUSERNAME] = LoxoneConfig.Username;
  //json[JSONCONFIG_LOXPASSWORD] = LoxoneConfig.Password;
  json[JSONCONFIG_LOXUDPPORT] = LoxoneConfig.UDPPort;
  json[JSONCFONIG_GPIO5MODE] = GlobalConfig.GPIO5Mode;
  json[JSONCFONIG_GPIO5ASSENDER] = GlobalConfig.GPIO5asSender;

  SPIFFS.remove("/" + configJsonFile);
  File configFile = SPIFFS.open("/" + configJsonFile, "w");
  if (!configFile) {
    DEBUG(F("failed to open config file for writing"));
    return false;
  }

#ifdef SERIALDEBUG
  serializeJson(doc, Serial);
  Serial.println();
#endif
  serializeJson(doc, configFile);
  configFile.close();
  SPIFFS.end();
  return true;
}

void setLastRelayState(bool state) {
  GlobalConfig.lastRelayState = state;
  if (GlobalConfig.restoreOldRelayState == RelayStateOnBoot_LAST) {
    if (SPIFFS.begin()) {
      DEBUG(F("setLastState mounted file system"));
      //SPIFFS.remove("/" + lastStateFilename);
      File setLastStateFile = SPIFFS.open("/" + lastRelayStateFilename, "w");
      setLastStateFile.print(state);
      setLastStateFile.close();
      SPIFFS.end();
      DEBUG("setLastState (" + String(state) + ") saved.");
    } else {
      DEBUG(F("setLastState SPIFFS mount fail!"));
    }
  }
}

bool getLastRelayState() {
  if (GlobalConfig.restoreOldRelayState == RelayStateOnBoot_LAST) {
    if (SPIFFS.begin()) {
      DEBUG(F("getLastState mounted file system"));
      if (SPIFFS.exists("/" + lastRelayStateFilename)) {
        DEBUG(lastRelayStateFilename + " existiert");
        File lastStateFile = SPIFFS.open("/" + lastRelayStateFilename, "r");
        bool bLastState = false;
        if (lastStateFile && lastStateFile.size()) {
          String content = String(char(lastStateFile.read()));
          DEBUG("getLastState FileContent = " + content);
          bLastState = (content == "1");
        }
        SPIFFS.end();
        return bLastState;
      } else {
        DEBUG(lastRelayStateFilename + " existiert nicht");
      }
    } else {
      DEBUG(F("getLastState SPIFFS mount fail!"));
      false;
    }
  } else {
    return false;
  }
}

void setBootConfigMode() {
  if (SPIFFS.begin()) {
    DEBUG(F("setBootConfigMode mounted file system"));
    if (!SPIFFS.exists("/" + bootConfigModeFilename)) {
      File bootConfigModeFile = SPIFFS.open("/" + bootConfigModeFilename, "w");
      bootConfigModeFile.print("0");
      bootConfigModeFile.close();
      SPIFFS.end();
      DEBUG(F("Boot to ConfigMode requested. Restarting..."));
      WebServer.send(200, "text/plain", F("<state>enableBootConfigMode - Rebooting</state>"));
      delay(500);
      ESP.restart();
    } else {
      WebServer.send(200, "text/plain", F("<state>enableBootConfigMode - FAILED!</state>"));
      SPIFFS.end();
    }
  }
}
