#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../box/BoxController.h"

class MqttManager {
private:
  WiFiClientSecure secureClient;
  PubSubClient mqttClient;
  BoxController& boxController;
  String controlTopic;
  String statusTopic;

  void reconnect();

public:
  MqttManager(BoxController& boxes);
  void begin();
  void update();
  void publishStatus(const char* eventType, int boxNumber = 0);
  void handleMessage(char* topic, byte* payload, unsigned int length);
};
