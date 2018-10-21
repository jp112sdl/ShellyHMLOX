void switchHandling() {
  CurrentSwitchGPIO5State = digitalRead(SwitchGPIOPin5);

  if (GlobalConfig.GPIO5Mode == GPIO5Mode_KEY) {
    if (CurrentSwitchGPIO5State == SwitchGPIOPin5LoP) {
      if (!KeyPress) {
        KeyPressDownMillis = millis();
        if (millis() - LastMillisKeyPress > MillisKeyBounce) {
          LastMillisKeyPress = millis();
          if (!GlobalConfig.GPIO5asSender) {
            toggleRelay(TRANSMITSTATE);
          }
          KeyPress = true;
        }
      }

      if ((GlobalConfig.GPIO5asSender) && (millis() - KeyPressDownMillis) > KEYPRESSLONGMILLIS && !PRESS_LONGsent) {
        //PRESS_LONG
        DEBUG("GPIO5 as Sender: PRESS_LONG");
        if (GlobalConfig.BackendType == BackendType_HomeMatic) setStateCUxD(HomeMaticConfig.ChannelNameSender + ".PRESS_LONG", "true");
        if (GlobalConfig.BackendType == BackendType_Loxone) sendLoxoneUDP(String(GlobalConfig.DeviceName) + ":1 = PRESS_LONG");
        PRESS_LONGsent = true;
      }
    } else {
      if (GlobalConfig.GPIO5asSender) {
        if (KeyPress) {
          if ((millis() - KeyPressDownMillis) < KEYPRESSLONGMILLIS) {
            //PRESS_SHORT
            DEBUG("GPIO5 as Sender: PRESS_SHORT");
            if (GlobalConfig.BackendType == BackendType_HomeMatic) setStateCUxD(HomeMaticConfig.ChannelNameSender + ".PRESS_SHORT", "true");
            if (GlobalConfig.BackendType == BackendType_Loxone) sendLoxoneUDP(String(GlobalConfig.DeviceName) + ":1 = PRESS_SHORT");
          }
        }
      }
      KeyPress = false;
      PRESS_LONGsent = false;
    }
  }

  //GPIO5 als Schalter
  if (GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_ABSOLUT || GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_TOGGLE) {
    if (CurrentSwitchGPIO5State != LastSwitchGPIOPin5State) {
      DEBUG("GPIO5 neuer Status = " + String(CurrentSwitchGPIO5State));
      LastSwitchGPIOPin5State = CurrentSwitchGPIO5State;
      if (GlobalConfig.GPIO5asSender) {
        if (GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_ABSOLUT) {
          DEBUG(F("GPIO5Mode_SWITCH_ABSOLUT"));
          if (GlobalConfig.BackendType == BackendType_HomeMatic) setStateCUxD(HomeMaticConfig.ChannelNameSender + ".SET_STATE",  (!CurrentSwitchGPIO5State ? "1" : "0"));
        }
        if (GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_TOGGLE) {
          DEBUG(F("GPIO5Mode_SWITCH_TOGGLE"));
          if (GlobalConfig.BackendType == BackendType_HomeMatic) {
            String currentState = getStateCUxD(HomeMaticConfig.ChannelNameSender + ".STATE", "State");
            DEBUG("CUxD Switch currentState = " + String(currentState));
            setStateCUxD(HomeMaticConfig.ChannelNameSender + ".SET_STATE",  (currentState == "false" ? "1" : "0"));
          }
        }

      } else {
        if (GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_ABSOLUT)
          switchRelay((CurrentSwitchGPIO5State == SwitchGPIOPin5LoP), TRANSMITSTATE); //HIGH = off, LOW = on
        if (GlobalConfig.GPIO5Mode == GPIO5Mode_SWITCH_TOGGLE)
          toggleRelay(TRANSMITSTATE);
      }
    }
  }
}
