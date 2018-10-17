/*
  Generic ESP8286 Module
  Flash Size: 2M (512k SPIFFS)
*/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiUdp.h>
#include "WM.h"
#include "css_global.h"
#include "js_global.h"
#include "js_fwupd.h"

const String FIRMWARE_VERSION = "1.0.0";
#define                       SERIALDEBUG
#define RelayPin               4
#define SwitchGPIOPin5         5
#define SwitchGPIOPin5LoP      HIGH

#define MillisKeyBounce      100  //Millisekunden zwischen 2xtasten
#define ConfigPortalTimeout  180  //Timeout (Sekunden) des AccessPoint-Modus
#define HTTPTimeOut         1500  //Timeout (Millisekunden) für http requests
#define IPSIZE                16
#define VARIABLESIZE         255
#define UDPPORT             6676
#define KEYPRESSLONGMILLIS  1500 //Millisekunden für langen Tastendruck
#define On                     1
#define Off                    0
const char GITHUB_REPO_URL[] PROGMEM = "https://api.github.com/repos/jp112sdl/ShellyHMLOX/releases/latest";

enum BackendTypes_e {
  BackendType_HomeMatic,
  BackendType_Loxone
};

enum RelayStates_e {
  RELAYSTATE_OFF,
  RELAYSTATE_ON
};

enum TransmitStates_e {
  NO_TRANSMITSTATE,
  TRANSMITSTATE
};

enum GPIO5Modes_e {
  GPIO5Mode_OFF,
  GPIO5Mode_KEY,
  GPIO5Mode_SWITCH_ABSOLUT,
  GPIO5Mode_SWITCH_TOGGLE
};

enum RelayStateOnBoot_e {
  RelayStateOnBoot_OFF,
  RelayStateOnBoot_LAST,
  RelayStateOnBoot_ON
};

struct globalconfig_t {
  char ccuIP[IPSIZE]   = "";
  char DeviceName[VARIABLESIZE] = "";
  uint8_t restoreOldRelayState = RelayStateOnBoot_OFF;
  bool lastRelayState = false;
  bool loadEcOnBoot = false;
  int  MeasureInterval = 10;
  byte BackendType = BackendType_HomeMatic;
  byte GPIO5Mode = GPIO5Mode_KEY;
  bool GPIO5asSender = false;
  String Hostname = "Shelly1";
} GlobalConfig;

struct hmconfig_t {
  String ChannelName = "";
  String ChannelNameSender = "";
  char PowerVariableName[VARIABLESIZE] = "";
  char EnergyCounterVariableName[VARIABLESIZE] = "";
  bool EnergyCounterVariableAvailable = false;
} HomeMaticConfig;

struct loxoneconfig_t {
  char Username[VARIABLESIZE] = "";
  char Password[VARIABLESIZE] = "";
  char UDPPort[10] = "";
} LoxoneConfig;

struct shellynetconfig_t {
  char ip[IPSIZE]      = "0.0.0.0";
  char netmask[IPSIZE] = "0.0.0.0";
  char gw[IPSIZE]      = "0.0.0.0";
} ShellyNetConfig;

const String bootConfigModeFilename = "bootcfg.mod";
const String lastRelayStateFilename = "laststat.txt";
const String configJsonFile         = "config.json";
bool RelayState = LOW;
bool KeyPress = false;
bool WiFiConnected = false;
bool LastSwitchGPIOPin5State = HIGH;
bool CurrentSwitchGPIO5State = HIGH;
unsigned long LastMillisKeyPress = 0;
unsigned long TimerStartMillis = 0;
unsigned long LastHlwMeasureMillis = 0;
unsigned long LastHlwCollectMillis = 0;
unsigned long KeyPressDownMillis = 0;
unsigned long TimerSeconds = 0;
unsigned long LastWiFiReconnectMillis = 0;
bool OTAStart = false;
bool UDPReady = false;
bool startWifiManager = false;
bool wm_shouldSaveConfig        = false;
bool PRESS_LONGsent = false;
#define wifiManagerDebugOutput   true

ESP8266WebServer WebServer(80);
ESP8266HTTPUpdateServer httpUpdater;

struct udp_t {
  WiFiUDP UDP;
  char incomingPacket[255];
} UDPClient;

void setup() {
  Serial.begin(115200);
  Serial.println("\nShelly " + WiFi.macAddress() + " startet... (FW: " + FIRMWARE_VERSION + ")");
  pinMode(RelayPin,        OUTPUT);
  pinMode(SwitchGPIOPin5,  INPUT);

  Serial.println(F("Config-Modus durch bootConfigMode aktivieren? "));
  if (SPIFFS.begin()) {
    Serial.println(F("-> mounted file system"));
    if (SPIFFS.exists("/" + bootConfigModeFilename)) {
      startWifiManager = true;
      Serial.println("-> " + bootConfigModeFilename + " existiert, starte Config-Modus");
      SPIFFS.remove("/" + bootConfigModeFilename);
      SPIFFS.end();
    } else {
      Serial.println("-> " + bootConfigModeFilename + " existiert NICHT");
    }
  } else {
    Serial.println(F("-> Nein, SPIFFS mount fail!"));
  }

  if (!loadSystemConfig()) startWifiManager = true;
  //Ab hier ist die Config geladen und alle Variablen sind mit deren Werten belegt!

  if (!startWifiManager) {
    Serial.println(F("Config-Modus mit Taster aktivieren?"));
    for (int i = 0; i < 20; i++) {
      if (digitalRead(SwitchGPIOPin5) == SwitchGPIOPin5LoP ) {
        startWifiManager = true;
        break;
      }
    }
    Serial.println("Config-Modus " + String(((startWifiManager) ? "" : "nicht ")) + "aktiviert.");
  }

  if (doWifiConnect()) {
    Serial.println(F("\nWLAN erfolgreich verbunden!"));
    printWifiStatus();
  } else ESP.restart();

#ifndef SERIALDEBUG
  Serial.println("\nSerieller Debug nicht konfiguriert. Deshalb ist hier jetzt ENDE.\n");
  delay(20); //to flush serial buffer
  Serial.end();
#endif

  initWebServerHandler();

  httpUpdater.setup(&WebServer);
  WebServer.begin();

  if (!MDNS.begin(GlobalConfig.Hostname.c_str())) {
    DEBUG("Error setting up MDNS responder!");
  }

  startOTAhandling();

  DEBUG("Starte UDP-Handler an Port " + String(UDPPORT) + "...");
  UDPClient.UDP.begin(UDPPORT);
  UDPReady = true;

  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    reloadCUxDAddress(NO_TRANSMITSTATE);
    byte tryCount = 0;
    byte tryCountMax = 5;
    while (HomeMaticConfig.ChannelName == "CUxD.") {
      tryCount++;
      DEBUG("Failed getting CUxD Device from HomeMaticConfig.ChannelName. Retry " + String(tryCount) + " / " + String(tryCountMax));
      delay(1000);
      reloadCUxDAddress(NO_TRANSMITSTATE);
      if (tryCount == tryCountMax) break;
    }
  }

  GlobalConfig.lastRelayState = getLastRelayState();
  if (((GlobalConfig.restoreOldRelayState == RelayStateOnBoot_LAST) && GlobalConfig.lastRelayState == true) || GlobalConfig.restoreOldRelayState == RelayStateOnBoot_ON) {
    switchRelay(RELAYSTATE_ON, TRANSMITSTATE);
  } else {
    switchRelay(RELAYSTATE_OFF, TRANSMITSTATE);
  }


  DEBUG(String(GlobalConfig.DeviceName) + " - Boot abgeschlossen, SSID = " + WiFi.SSID() + ", IP = " + String(IpAddress2String(WiFi.localIP())) + ", RSSI = " + WiFi.RSSI() + ", MAC = " + WiFi.macAddress());
}

void loop() {
  //Überlauf der millis() abfangen
  if (LastMillisKeyPress > millis())
    LastMillisKeyPress = millis();
  if (TimerStartMillis > millis())
    TimerStartMillis = millis();
  if (LastHlwMeasureMillis > millis())
    LastHlwMeasureMillis = millis();
  if (LastHlwCollectMillis > millis())
    LastHlwCollectMillis = millis();
  if (LastWiFiReconnectMillis > millis())
    LastWiFiReconnectMillis = millis();

  //Reconnect WiFi wenn nicht verbunden (alle 30 Sekunden)
  if (WiFi.status() != WL_CONNECTED) {
    WiFiConnected = false;
    if (millis() - LastWiFiReconnectMillis > 30000) {
      LastWiFiReconnectMillis = millis();
      DEBUG("WiFi Connection lost! Reconnecting...");
      WiFi.reconnect();
    }
  } else {
    if (!WiFiConnected) {
      DEBUG("WiFi reconnected!");
      WiFiConnected = true;
    }
  }

  //auf OTA Anforderung reagieren
  ArduinoOTA.handle();

  if (!OTAStart) {
    //eingehende UDP Kommandos abarbeiten
    String udpMessage = handleUDP();
    if (udpMessage == "bootConfigMode")
      setBootConfigMode;
    if (udpMessage == "reboot")
      ESP.restart();
    if (udpMessage == "1" || udpMessage == "on")
      switchRelay(RELAYSTATE_ON, NO_TRANSMITSTATE);
    if (udpMessage == "0" || udpMessage == "off")
      switchRelay(RELAYSTATE_OFF, NO_TRANSMITSTATE);
    if (udpMessage == "2" || udpMessage == "toggle")
      toggleRelay(false);
    if (udpMessage.indexOf("1?t=") != -1) {
      TimerSeconds = (udpMessage.substring(4, udpMessage.length())).toInt();
      if (TimerSeconds > 0) {
        TimerStartMillis = millis();
        DEBUG("webSwitchRelayOn(), Timer aktiviert, Sekunden: " + String(TimerSeconds));
      } else {
        DEBUG(F("webSwitchRelayOn(), Parameter, aber mit TimerSeconds = 0"));
      }
      switchRelay(RELAYSTATE_ON, NO_TRANSMITSTATE);
    }

    //eingehende HTTP Anfragen abarbeiten
    WebServer.handleClient();

    //Tasterbedienung am Shelly abarbeiten
    switchHandling();

    //Timer
    if (TimerSeconds > 0 && millis() - TimerStartMillis > TimerSeconds * 1000) {
      DEBUG(F("Timer abgelaufen. Schalte Relais aus."));
      switchRelay(RELAYSTATE_OFF, TRANSMITSTATE);
    }

    //needed for UDP packet parser
    delay(10);
  }
}
