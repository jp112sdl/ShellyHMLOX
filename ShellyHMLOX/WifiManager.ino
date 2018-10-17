bool doWifiConnect() {
  String _ssid = WiFi.SSID();
  String _psk = WiFi.psk();

  String _pskMask = "";
  for (int i = 0; i < _psk.length(); i++) {
    _pskMask += "*";
  }
  DEBUG("ssid = " + _ssid + ", psk = " + _pskMask);


  const char* ipStr = ShellyNetConfig.ip; byte ipBytes[4]; parseBytes(ipStr, '.', ipBytes, 4, 10);
  const char* netmaskStr = ShellyNetConfig.netmask; byte netmaskBytes[4]; parseBytes(netmaskStr, '.', netmaskBytes, 4, 10);
  const char* gwStr = ShellyNetConfig.gw; byte gwBytes[4]; parseBytes(gwStr, '.', gwBytes, 4, 10);

  if (!startWifiManager && _ssid != "" && _psk != "" ) {
    DEBUG(F("Connecting WLAN the classic way..."));
    WiFi.mode(WIFI_STA);
    WiFi.hostname(GlobalConfig.Hostname);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.setAutoReconnect(true);
    if (String(ShellyNetConfig.ip) != "0.0.0.0") {
      WiFi.config(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));
      ETS_UART_INTR_DISABLE();
      wifi_station_disconnect();
      ETS_UART_INTR_ENABLE();
    }
    WiFi.begin(_ssid.c_str(), _psk.c_str());
    int waitCounter = 0;

    while (WiFi.status() != WL_CONNECTED) {
      waitCounter++;
      Serial.print(".");
      if (waitCounter == 30) {
        return false;
      }
      delay(500);
    }

    DEBUG("Wifi Connected");
    WiFiConnected = true;
    return true;
  } else {
    WiFiManager wifiManager;
    wifiManager.setDebugOutput(wifiManagerDebugOutput);
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    WiFiManagerParameter custom_ccuip("ccu", "IP der CCU2", GlobalConfig.ccuIP, IPSIZE, 0, "pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$'");
    //WiFiManagerParameter custom_loxusername("loxusername", "Loxone Username", "", VARIABLESIZE);
    //WiFiManagerParameter custom_loxpassword("loxpassword", "Loxone Password", "", VARIABLESIZE,4);
    WiFiManagerParameter custom_loxudpport("loxudpport", "Loxone UDP Port", LoxoneConfig.UDPPort, 10, 0, "pattern='[0-9]{1,5}'");
    WiFiManagerParameter custom_shellyname("shelly", "Shelly Ger&auml;tename", GlobalConfig.DeviceName, VARIABLESIZE, 0, "pattern='[A-Za-z0-9_ -]+'");

    String strRestoreOldState = "";
    switch (GlobalConfig.restoreOldRelayState) {
      case RelayStateOnBoot_OFF:
        strRestoreOldState = F("<option selected value='0'>Aus</option><option value='1'>letzter Zustand</option><option value='2'>Ein</option>");
        break;
      case RelayStateOnBoot_LAST:
        strRestoreOldState = F("<option value='0'>Aus</option><option selected value='1'>letzter Zustand</option><option value='2'>Ein</option>");
        break;
      case RelayStateOnBoot_ON:
        strRestoreOldState = F("<option value='0'>Aus</option><option value='1'>letzter Zustand</option><option selected value='2'>Ein</option>");
        break;
      default:
        strRestoreOldState = F("<option selected value='0'>Aus</option><option value='1'>letzter Zustand</option><option value='2'>Ein</option>");
        break;
    }
    WiFiManagerParameter custom_restorestate("restorestate", "Schaltzustand bei Boot", "", 8, 2, strRestoreOldState.c_str());

    String backend = "";
    switch (GlobalConfig.BackendType) {
      case BackendType_HomeMatic:
        backend = F("<option selected value='0'>HomeMatic</option><option value='1'>Loxone</option>");
        break;
      case BackendType_Loxone:
        backend = F("<option value='0'>HomeMatic</option><option selected value='1'>Loxone</option>");
        break;
      default:
        backend = F("<option value='0'>HomeMatic</option><option value='1'>Loxone</option>");
        break;
    }
    WiFiManagerParameter custom_backendtype("backendtype", "Backend", "", 8, 2, backend.c_str());

    String gpio5 = "";
    switch (GlobalConfig.GPIO5Mode) {
      case GPIO5Mode_OFF:
        gpio5 = F("<option selected value='0'>nicht verwendet</option><option value='1'>Taster</option><option value='2'>Schalter (absolut)</option><option value='3'>Schalter (toggle)</option>");
        break;
      case GPIO5Mode_KEY:
        gpio5 = F("<option value='0'>nicht verwendet</option><option selected value='1'>Taster</option><option value='2'>Schalter (absolut)</option><option value='3'>Schalter (toggle)</option>");
        break;
      case GPIO5Mode_SWITCH_ABSOLUT:
        gpio5 = F("<option value='0'>nicht verwendet</option><option value='1'>Taster</option><option selected value='2'>Schalter (absolut)</option><option value='3'>Schalter (toggle)</option>");
        break;
      case GPIO5Mode_SWITCH_TOGGLE:
        gpio5 = F("<option value='0'>nicht verwendet</option><option value='1'>Taster</option><option value='2'>Schalter (absolut)</option><option selected value='3'>Schalter (toggle)</option>");
        break;
      default:
        gpio5 = F("<option selected value='0'>nicht verwendet</option><option value='1'>Taster</option><option value='2'>Schalter</option>");
        break;
    }
    WiFiManagerParameter custom_gpio5mode("gpio5mode_switch", "GPIO5 Mode", "", 8, 2, gpio5.c_str());

    WiFiManagerParameter custom_gpio5assender("custom_gpio5assender_switch", "GPIO5 nur Sender: ", (GlobalConfig.GPIO5asSender) ? "1" : "0", 8, 1);

    WiFiManagerParameter custom_ip("custom_ip", "IP-Adresse", (String(ShellyNetConfig.ip) != "0.0.0.0") ? ShellyNetConfig.ip : "", IPSIZE, 0, "pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$'");
    WiFiManagerParameter custom_netmask("custom_netmask", "Netzmaske", (String(ShellyNetConfig.netmask) != "0.0.0.0") ? ShellyNetConfig.netmask : "", IPSIZE, 0, "pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$'");
    WiFiManagerParameter custom_gw("custom_gw", "Gateway",  (String(ShellyNetConfig.gw) != "0.0.0.0") ? ShellyNetConfig.gw : "", IPSIZE, 0, "pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$'");
    WiFiManagerParameter custom_text("<br/><br><div>Statische IP (wenn leer, dann DHCP):</div>");
    wifiManager.addParameter(&custom_ccuip);
    //wifiManager.addParameter(&custom_loxusername);
    //wifiManager.addParameter(&custom_loxpassword);
    wifiManager.addParameter(&custom_loxudpport);
    wifiManager.addParameter(&custom_shellyname);
    wifiManager.addParameter(&custom_restorestate);
    wifiManager.addParameter(&custom_gpio5mode);
    wifiManager.addParameter(&custom_gpio5assender);
    wifiManager.addParameter(&custom_backendtype);
    wifiManager.addParameter(&custom_text);
    wifiManager.addParameter(&custom_ip);
    wifiManager.addParameter(&custom_netmask);
    wifiManager.addParameter(&custom_gw);

    wifiManager.setConfigPortalTimeout(ConfigPortalTimeout);

    if (startWifiManager == true) {
      if (_ssid == "" || _psk == "" ) {
        wifiManager.resetSettings();
      }
      else {
        if (!wifiManager.startConfigPortal()) {
          DEBUG(F("WM: failed to connect and hit timeout"));
          delay(500);
          ESP.restart();
        }
      }
    }

    wifiManager.setSTAStaticIPConfig(IPAddress(ipBytes[0], ipBytes[1], ipBytes[2], ipBytes[3]), IPAddress(gwBytes[0], gwBytes[1], gwBytes[2], gwBytes[3]), IPAddress(netmaskBytes[0], netmaskBytes[1], netmaskBytes[2], netmaskBytes[3]));

    wifiManager.autoConnect();

    DEBUG(F("Wifi Connected"));
    DEBUG("CUSTOM STATIC IP: " + String(ShellyNetConfig.ip) + " Netmask: " + String(ShellyNetConfig.netmask) + " GW: " + String(ShellyNetConfig.gw));
    if (wm_shouldSaveConfig) {
      if (String(custom_ip.getValue()).length() > 5) {
        DEBUG("Custom IP Address is set!");
        strcpy(ShellyNetConfig.ip, custom_ip.getValue());
        strcpy(ShellyNetConfig.netmask, custom_netmask.getValue());
        strcpy(ShellyNetConfig.gw, custom_gw.getValue());

      } else {
        strcpy(ShellyNetConfig.ip,      "0.0.0.0");
        strcpy(ShellyNetConfig.netmask, "0.0.0.0");
        strcpy(ShellyNetConfig.gw,      "0.0.0.0");
      }

      GlobalConfig.restoreOldRelayState = (atoi(custom_restorestate.getValue()));
      GlobalConfig.GPIO5asSender = (atoi(custom_gpio5assender.getValue()) == 1);
      GlobalConfig.BackendType = (atoi(custom_backendtype.getValue()));
      GlobalConfig.GPIO5Mode = (atoi(custom_gpio5mode.getValue()));

      strcpy(GlobalConfig.ccuIP, custom_ccuip.getValue());
      strcpy(GlobalConfig.DeviceName, custom_shellyname.getValue());
      //strcpy(LoxoneConfig.Username, custom_loxusername.getValue());
      //strcpy(LoxoneConfig.Password, custom_loxpassword.getValue());
      strcpy(LoxoneConfig.UDPPort, custom_loxudpport.getValue());

      saveSystemConfig();

      delay(100);
      ESP.restart();
    }
    return true;
  }
}

void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG("AP-Modus ist aktiv!");
}

void saveConfigCallback () {
  DEBUG("Should save config");
  wm_shouldSaveConfig = true;
}

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);
    str = strchr(str, sep);
    if (str == NULL || *str == '\0') {
      break;
    }
    str++;
  }
}

void printWifiStatus() {
  DEBUG("SSID: " + WiFi.SSID());
  DEBUG("IP Address: " + IpAddress2String(WiFi.localIP()));
  DEBUG("Gateway Address: " + IpAddress2String(WiFi.gatewayIP()));
  DEBUG("signal strength (RSSI):" + String(WiFi.RSSI()) + " dBm");
}
