#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../lock/LockManager.h"
#include "../sensor/SensorManager.h"

class MqttManager {
private:
  WiFiClientSecure secureClient;
  PubSubClient mqttClient;
  LockManager& lockManager;
  SensorManager& sensorManager;
  String controlTopic;
  String statusTopic;

  void reconnect();

public:
  MqttManager(LockManager& lock, SensorManager& sensor);
  void begin();
  void update();
  void publishStatus(const char* eventType);
  void handleMessage(char* topic, byte* payload, unsigned int length);
};
