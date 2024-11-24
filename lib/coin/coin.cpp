#include <Arduino.h>
#include <ArduinoJson.h>
#include <coin.h>
#include <HTTPClient.h>

const String API = "https://api.binance.com/api/v3/avgPrice?symbol=";

CoinData getCoinData(String c1, String c2) {
  HTTPClient http;
  http.begin(API + c1 + c2);
  http.GET();
  String payload = http.getString();
  http.end();
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload);
  if(error) {
    Serial.print("Erro ao parsear JSON: ");
    Serial.println(error.c_str());
    return CoinData{price: .0, updatedTime: tm{}};
  }

  const char* price = doc["price"];
  const long long closeTime = doc["closeTime"];

  time_t closeTimeSec = closeTime / 1000; 
  struct tm timeInfo;
  localtime_r(&closeTimeSec, &timeInfo); 

  Serial.println(price);
  return CoinData{price: static_cast<float>(atof(price)), updatedTime: timeInfo};
}