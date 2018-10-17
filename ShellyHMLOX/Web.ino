const char HTTP_TITLE_LABEL[] PROGMEM = "<div class='l lt'><label>{v}</label><hr /></div>";
const char HTTP_CURRENT_STATE_LABEL[] PROGMEM = "<div class='l ls'><label id='_ls'>{ls}</label></div>";
const char HTTP_FW_LABEL[] PROGMEM = "<div class='l c k'><label>Firmware: {fw}</label></div>";
const char HTTP_POWER_LABEL[] PROGMEM = "<table><tr><td class=tdl>Spannung</td><td class=tdr id='_v'>{hlw_v}</td><td class=tdl>V</td></tr><tr><td class=tdl>Strom</td><td class=tdr id='_c'>{hlw_c}</td><td class=tdl>A</td><tr><td class=tdl>Leistung</td><td class=tdr id='_w'>{hlw_w}</td><td class=tdl>W</td></tr><tr><td class=tdl>Leistung</td><td class=tdr id='_va'>{hlw_va}</td><td class=tdl>VA</td></tr><tr><td class=tdl>Energiez&auml;hler</td><td class=tdr id='_ec'>{hlw_ec}</td><td class=tdl>Wh</td></tr></table>";
const char HTTP_ONOFF_BUTTONS[] PROGMEM = "<span class='l'><div><button name='btnAction' onclick='SetState(\"/1?ts=1&t=\"+document.getElementById(\"timer\").value); return false;'>AN</button></div><div><table><tr><td>Timer:</td><td align='right'><input class='i' type='text' id='timer' name='timer' placeholder='Sekunden' pattern='[0-9]{1,5}' value='' maxlength='5'></td></tr></table></div><div><button name='btnAction' onclick='SetState(\"/0?ts=1\"); return false;'>AUS</button></div></span>";
const char HTTP_CONFIG_BUTTON[] PROGMEM = "<div></div><hr /><div></div><div><input class='lnkbtn' type='button' value='Konfiguration' onclick=\"window.location.href='/config'\" /></div>";
const char HTTP_HOME_BUTTON[] PROGMEM = "<div><input class='lnkbtn' type='button' value='Zur&uuml;ck' onclick=\"window.location.href='/'\" /></div>";
const char HTTP_SAVE_BUTTON[] PROGMEM = "<div><button name='btnSave' value='1' type='submit'>Speichern</button></div>";
const char HTTP_CONF[] PROGMEM = "<div><label>{st}:</label></div><div><input type='text' id='ccuip' name='ccuip' pattern='((^|\\.)((25[0-5])|(2[0-4]\\d)|(1\\d\\d)|([1-9]?\\d))){4}$' maxlength=16 placeholder='{st}' value='{ccuip}'></div><div><label>Ger&auml;tename:</label></div><div><input type='text' id='devicename' name='devicename' pattern='[A-Za-z0-9_ -]+' placeholder='Ger&auml;tename' value='{dn}'></div>";
const char HTTP_CONF_ADD_RESTORESTATE[] PROGMEM = "<div><label for='restorestate'>Schaltzustand bei Boot</label><span class='ckb cob'><select id='restorestate' name='restorestate'><option {restore_off} value='0'>Aus</option><option {restore_last} value='1'>letzter Zustand</option><option {restore_on} value='2'>Ein</option></select></span></div>";
const char HTTP_CONF_ADD_SWITCH[] PROGMEM = "<hr /><div><div id='div_gpio5mode'></div><label for='gpio5mode'>GPIO5 Mode</label><span class='ckb cob'><select id='gpio5mode' name='gpio5mode'><option {gpio5mode_off} value='0'>nicht verwendet</option><option {gpio5mode_key} value='1'>Taster</option><option {gpio5mode_switch_abs} value='2'>Schalter (absolut)</option><option {gpio5mode_switch_tog} value='3'>Schalter (toggle)</option></select></span></div><div><label class='lcb' for='gpio5assender'><input id='gpio5assender' class='cb' type='checkbox' name='gpio5assender' {gpio5assender} value=1> GPIO5 nur Sender</label><hr /></div>";
const char HTTP_CONF_LOX[] PROGMEM = "<div><label>UDP Port:</label></div><div><input type='text' id='lox_udpport' pattern='[0-9]{1,5}' maxlength='5' name='lox_udpport' placeholder='UDP Port' value='{udp}'></div>";
const char HTTP_STATUSLABEL[] PROGMEM = "<div class='l c'>{sl}</div>";
const char HTTP_NEWFW_BUTTON[] PROGMEM = "<div><input class='fwbtn' id='fwbtn' type='button' value='Neue Firmware verf&uuml;gbar' onclick=\"window.open('{fwurl}')\" /></div><div><input class='fwbtn' id='fwbtnupdt' type='button' value='Firmwaredatei einspielen' onclick=\"window.location.href='/update'\" /></div>";

void initWebServerHandler() {
  WebServer.on("/0", webSwitchRelayOff);
  WebServer.on("/off", webSwitchRelayOff);
  WebServer.on("/1", webSwitchRelayOn);
  WebServer.on("/on", webSwitchRelayOn);
  WebServer.on("/2", webToggleRelay);
  WebServer.on("/toggle", webToggleRelay);
  WebServer.on("/getState", replyRelayState);
  WebServer.on("/bootConfigMode", setBootConfigMode);
  WebServer.on("/reboot", []() {
    WebServer.send(200, "text/plain", "rebooting");
    delay(100);
    ESP.restart();
  });
  WebServer.on("/restart", []() {
    WebServer.send(200, "text/plain", "rebooting");
    delay(100);
    ESP.restart();
  });
  WebServer.on("/version", versionHtml);
  WebServer.on("/firmware", versionHtml);
  WebServer.on("/config", configHtml);
  WebServer.on("/reloadCUxD", []() {
    String ret = reloadCUxDAddress(TRANSMITSTATE);
    WebServer.send(200, "text/plain", ret);
  });
  WebServer.onNotFound(defaultHtml);
}

void webSwitchRelayOn() {
  bool _transmitstate = NO_TRANSMITSTATE;
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "t") {
        TimerSeconds = WebServer.arg(i).toInt();
        if (TimerSeconds > 0) {
          TimerStartMillis = millis();
          DEBUG("webSwitchRelayOn(), Timer aktiviert, Sekunden: " + String(TimerSeconds));
        } else {
          DEBUG(F("webSwitchRelayOn(), Parameter, aber mit TimerSeconds = 0"));
        }
      }
      if (WebServer.argName(i) == "ts") {
        _transmitstate = WebServer.arg(i).toInt();
      }
    }
  } else {
    TimerSeconds = 0;
    DEBUG(F("webSwitchRelayOn(), keine Parameter, TimerSeconds = 0"));
  }
  switchRelay(RELAYSTATE_ON, _transmitstate);
  sendDefaultWebCmdReply();
}

void webToggleRelay() {
  bool _transmitstate = NO_TRANSMITSTATE;
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "ts") {
        _transmitstate = WebServer.arg(i).toInt();
      }
    }
  }
  toggleRelay(_transmitstate);
  sendDefaultWebCmdReply();
}
void webSwitchRelayOff() {
  bool _transmitstate = NO_TRANSMITSTATE;
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "ts") {
        _transmitstate = WebServer.arg(i).toInt();
      }
    }
  }
  switchRelay(RELAYSTATE_OFF, _transmitstate);
  sendDefaultWebCmdReply();
}

void replyRelayState() {
  sendDefaultWebCmdReply();
}

void defaultHtml() {
  if (WebServer.args() > 0) {
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnAction") {
        switchRelay(WebServer.arg(i).toInt(), TRANSMITSTATE);
      }
      if (WebServer.argName(i) == "timer") {
        TimerSeconds = WebServer.arg(i).toInt();
        if (TimerSeconds > 0) {
          TimerStartMillis = millis();
        }
      }
    }
  }

  String page = FPSTR(HTTP_HEAD);
  //page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<div class='fbg'>");

  //page += F("<form method='post' action='control'>");
  page += FPSTR(HTTP_TITLE_LABEL);
  page += FPSTR(HTTP_CURRENT_STATE_LABEL);
  page.replace("{v}", GlobalConfig.DeviceName);

  page.replace("{ls}", ((getRelayState() == RELAYSTATE_ON) ? "AN" : "AUS"));

  page += FPSTR(HTTP_ONOFF_BUTTONS);

  page += FPSTR(HTTP_CONFIG_BUTTON);
  String restZeit = "";
  if (TimerSeconds > 0) restZeit =  String(TimerSeconds - (millis() - TimerStartMillis) / 1000) ;
  page.replace("{ts}", restZeit);

  page += FPSTR(HTTP_FW_LABEL);

  page += FPSTR(HTTP_NEWFW_BUTTON);
  String fwurl = FPSTR(GITHUB_REPO_URL);
  String fwjsurl = FPSTR(GITHUB_REPO_URL);
  fwurl.replace("api.", "");
  fwurl.replace("repos/", "");
  page.replace("{fwurl}", fwurl);

  page += F("</div><script>");
  page += FPSTR(HTTP_CUSTOMSCRIPT);
  page += FPSTR(HTTP_CUSTOMUPDATESCRIPT);
  page.replace("{fwjsurl}", fwjsurl);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += F("</script></div></body></html>");
  WebServer.sendHeader("Content-Length", String(page.length()));
  WebServer.send(200, "text/html", page);
}

void configHtml() {
  bool sc = false;
  bool saveSuccess = false;
  bool showHMDevError = false;
  if (WebServer.args() > 0) {
    GlobalConfig.restoreOldRelayState = RelayStateOnBoot_OFF;
    GlobalConfig.GPIO5asSender = false;
    for (int i = 0; i < WebServer.args(); i++) {
      if (WebServer.argName(i) == "btnSave")
        sc = (WebServer.arg(i).toInt() == 1);
      if (WebServer.argName(i) == "ccuip")
        strcpy(GlobalConfig.ccuIP, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "devicename")
        strcpy(GlobalConfig.DeviceName, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "lox_udpport")
        strcpy(LoxoneConfig.UDPPort, WebServer.arg(i).c_str());
      if (WebServer.argName(i) == "restorestate")
        GlobalConfig.restoreOldRelayState = String(WebServer.arg(i)).toInt();
      if (WebServer.argName(i) == "gpio5mode")
        GlobalConfig.GPIO5Mode = String(WebServer.arg(i)).toInt();
      if (WebServer.argName(i) == "gpio5assender")
        GlobalConfig.GPIO5asSender = (String(WebServer.arg(i)).toInt() == 1);
    }
    if (sc) {
      setLastRelayState(getRelayState() == RELAYSTATE_ON);
      saveSuccess = saveSystemConfig();
      if (GlobalConfig.BackendType == BackendType_HomeMatic) {
        String devName = getStateCUxD(String(GlobalConfig.DeviceName), "Address") ;
        if (devName != "null") {
          showHMDevError = false;
          HomeMaticConfig.ChannelName =  "CUxD." + devName;
        } else {
          showHMDevError = true;
        }
      }
    }
  }
  String page = FPSTR(HTTP_HEAD);

  //page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_ALL_STYLE);
  if (GlobalConfig.BackendType == BackendType_HomeMatic)
    page += FPSTR(HTTP_HM_STYLE);
  if (GlobalConfig.BackendType == BackendType_Loxone)
    page += FPSTR(HTTP_LOX_STYLE);
  page += FPSTR(HTTP_HEAD_END);
  page += F("<div class='fbg'>");
  page += F("<form method='post' action='config'>");
  page += FPSTR(HTTP_TITLE_LABEL);
  page += FPSTR(HTTP_CONF);

  page += FPSTR(HTTP_CONF_ADD_RESTORESTATE);
  switch (GlobalConfig.restoreOldRelayState) {
    case RelayStateOnBoot_OFF:
      page.replace("{restore_off}", "selected");
      page.replace("{restore_last}", "");
      page.replace("{restore_on}", "");
      break;
    case RelayStateOnBoot_LAST:
      page.replace("{restore_off}", "");
      page.replace("{restore_last}", "selected");
      page.replace("{restore_on}", "");
      break;
    case RelayStateOnBoot_ON:
      page.replace("{restore_off}", "");
      page.replace("{restore_last}", "");
      page.replace("{restore_on}", "selected");
      break;
  }

  page += FPSTR(HTTP_CONF_ADD_SWITCH);
  switch (GlobalConfig.GPIO5Mode) {
    case GPIO5Mode_OFF:
      page.replace("{gpio5mode_off}", "selected");
      page.replace("{gpio5mode_key}", "");
      page.replace("{gpio5mode_switch_abs}", "");
      page.replace("{gpio5mode_switch_tog}", "");
      break;
    case GPIO5Mode_KEY:
      page.replace("{gpio5mode_off}", "");
      page.replace("{gpio5mode_key}", "selected");
      page.replace("{gpio5mode_switch_abs}", "");
      page.replace("{gpio5mode_switch_tog}", "");
      break;
    case GPIO5Mode_SWITCH_ABSOLUT:
      page.replace("{gpio5mode_off}", "");
      page.replace("{gpio5mode_key}", "");
      page.replace("{gpio5mode_switch_abs}", "selected");
      page.replace("{gpio5mode_switch_tog}", "");
      break;
    case GPIO5Mode_SWITCH_TOGGLE:
      page.replace("{gpio5mode_off}", "");
      page.replace("{gpio5mode_key}", "");
      page.replace("{gpio5mode_switch_abs}", "");
      page.replace("{gpio5mode_switch_tog}", "selected");
      break;
    default:
      page.replace("{gpio5mode_off}", "selected");
      page.replace("{gpio5mode_key}", "");
      page.replace("{gpio5mode_switch_abs}", "");
      page.replace("{gpio5mode_switch_tog}", "");
      break;
  }
  page.replace("{gpio5assender}", ((GlobalConfig.GPIO5asSender) ? "checked" : ""));
  if (GlobalConfig.BackendType == BackendType_HomeMatic) {
    page.replace("{st}", "CCU2 IP");
    page.replace("{remanenz}", "Schaltzustand wiederherstellen");
  }
  if (GlobalConfig.BackendType == BackendType_Loxone) {
    page += FPSTR(HTTP_CONF_LOX);
    page.replace("{st}", "MiniServer IP");
    page.replace("{udp}", LoxoneConfig.UDPPort);
    page.replace("{remanenz}", "Remanenzeingang");
  }

  page.replace("{dn}", GlobalConfig.DeviceName);
  page.replace("{ccuip}", GlobalConfig.ccuIP);

  page += FPSTR(HTTP_STATUSLABEL);

  if (sc && !showHMDevError) {
    if (saveSuccess) {
      page.replace("{sl}", F("<label class='green'>Speichern erfolgreich.</label>"));
    } else {
      page.replace("{sl}", F("<label class='red'>Speichern fehlgeschlagen.</label>"));
    }
  }

  if (showHMDevError)
    page.replace("{sl}", F("<label class='red'>Ger&auml;tenamen in CUxD pr&uuml;fen!</label>"));

  if (!sc && !showHMDevError)
    page.replace("{sl}", "");

  page += FPSTR(HTTP_SAVE_BUTTON);
  page += FPSTR(HTTP_HOME_BUTTON);
  page += FPSTR(HTTP_FW_LABEL);
  page.replace("{fw}", FIRMWARE_VERSION);

  page += F("</form></div>");
  page += F("</div></body></html>");
  page.replace("{v}", GlobalConfig.DeviceName);

  WebServer.send(200, "text/html", page);
}

void sendDefaultWebCmdReply() {
  String reply = createReplyString();
  DEBUG("Sending Web-Reply: " + reply);
  WebServer.send(200, "application/json", reply);
}

String createReplyString() {
  return "{\"state\": " + String(getRelayState() == RELAYSTATE_ON) + ", \"timer\": " + String(TimerSeconds) + ", \"resttimer\": " + String((TimerSeconds > 0) ? (TimerSeconds - (millis() - TimerStartMillis) / 1000) : 0) + ", \"fw\": \"" + FIRMWARE_VERSION + "\"}";
}

void versionHtml() {
  WebServer.send(200, "text/plain", "<fw>" + FIRMWARE_VERSION + "</fw>");
}
