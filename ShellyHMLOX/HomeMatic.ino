bool setStateCUxD(String id, String value) {
  if (id.indexOf(".null.") == -1 && String(GlobalConfig.ccuIP) != "0.0.0.0") {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      id.replace(" ", "%20");
      String url = "http://" + String(GlobalConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22).State(" + value + ")";
      DEBUG("setStateCUxD url: " + url);
      http.begin(url);
      int httpCode = http.GET();
      String payload = "";

      if (httpCode > 0) {
        DEBUG("HTTP " + id + " success");
        payload = http.getString();
      }
      if (httpCode != 200) {
        DEBUG("HTTP " + id + " failed with HTTP Error Code " + String(httpCode));
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));

      DEBUG("result: " + payload);

      return (payload != "null");

    } else {
      DEBUG("setStateCUxD: WiFi.status() != WL_CONNECTED, trying to reconnect");
      return false;
      /*if (!doWifiConnect()) {
        DEBUG("setStateCUxD: doWifiConnect() failed.", "setStateCUxD()", _slError);
        //ESP.restart();
        }*/
    }
  } else return true;
}

String getStateCUxD(String id, String type) {
  if (id != "" && id.indexOf(".null.") == -1 && String(GlobalConfig.ccuIP) != "0.0.0.0") {
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.setTimeout(HTTPTimeOut);
      id.replace(" ", "%20");
      String url = "http://" + String(GlobalConfig.ccuIP) + ":8181/cuxd.exe?ret=dom.GetObject(%22" + id + "%22)." + type + "()";
      DEBUG("getStateFromCUxD url: " + url);
      http.begin(url);
      int httpCode = http.GET();
      String payload = "error";
      if (httpCode > 0) {
        payload = http.getString();
      }
      if (httpCode != 200) {
        DEBUG("HTTP " + id + " fail");
      }
      http.end();

      payload = payload.substring(payload.indexOf("<ret>"));
      payload = payload.substring(5, payload.indexOf("</ret>"));
      DEBUG("result: " + payload);

      return payload;
    } else {
      DEBUG("getStateCUxD: WiFi.status() != WL_CONNECTED, trying to reconnect");
      return "null";
      /*if (!doWifiConnect()) {
        DEBUG("getStateCUxD: doWifiConnect() failed.", "getStateCUxD()", _slError);
        //ESP.restart();
        }*/
    }
  } else return "null";
}

String reloadCUxDAddress(bool transmitState) {
  String ret = "";
  HomeMaticConfig.ChannelName =  "CUxD." + getStateCUxD(String(GlobalConfig.DeviceName), "Address");
  ret += "CUxD Address = " + HomeMaticConfig.ChannelName;
  DEBUG("HomeMaticConfig.ChannelName = " + HomeMaticConfig.ChannelName);


  if (GlobalConfig.GPIO5Mode != GPIO5Mode_OFF && GlobalConfig.GPIO5asSender) {
    HomeMaticConfig.ChannelNameSender =  "CUxD." + getStateCUxD(String(GlobalConfig.DeviceName) + ":1", "Address");
    ret += " ; CUxD Address Sender = " + HomeMaticConfig.ChannelNameSender;
    DEBUG("HomeMaticConfig.ChannelNameSender = " + HomeMaticConfig.ChannelNameSender);
  }

  if (transmitState == TRANSMITSTATE)
    setStateCUxD(HomeMaticConfig.ChannelName + ".SET_STATE", String(getRelayState()));
  return ret;
}
