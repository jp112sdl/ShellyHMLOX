void switchRelay(bool toState, bool transmitState) {
  RelayState = toState;
  DEBUG("Switch Relay to " + String(toState) + " with transmitState = " + String(transmitState));

  if (toState == RELAYSTATE_OFF) {
    TimerSeconds = 0;
  }

  digitalWrite(RelayPin, RelayState);

  setLastRelayState(RelayState);

  if (transmitState) {
    if (GlobalConfig.BackendType == BackendType_HomeMatic) setStateCUxD(HomeMaticConfig.ChannelName + ".SET_STATE", String(RelayState));
    if (GlobalConfig.BackendType == BackendType_Loxone) sendLoxoneUDP(String(GlobalConfig.DeviceName) + "=" + String(RelayState));
  } 
}

bool getRelayState() {
  return (digitalRead(RelayPin) == RELAYSTATE_ON);
}

void toggleRelay(bool transmitState) {
  TimerSeconds = 0;
  if (getRelayState() == RELAYSTATE_OFF) {
    switchRelay(RELAYSTATE_ON, transmitState);
  } else  {
    switchRelay(RELAYSTATE_OFF, transmitState);
  }
}
