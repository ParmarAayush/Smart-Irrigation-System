#include <Arduino.h>
#include <WiFi.h>
#include <Bonezegei_DHT11.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "OPPO A77 Ayush"
#define WIFI_PASSWORD "123456789"
#define API_KEY "AIzaSyAWMqOfjaS_W2B-Gi9ef2N3Nt8xmTwZNpw"
#define DATABASE_URL "https://iot-devices-1f8c0-default-rtdb.firebaseio.com/"


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Bonezegei_DHT11 dht(14);

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;


struct SensorData
{
  float temperatureC;
  float temperatureF;
  int humidity;
  bool success;
};
void connectWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wifi....");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}
void configDatabase()
{
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) // last two parameter is for id and pwd firebase auth but current we use anonymous signin
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("Error From Config Database");
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

SensorData getDataFromSensor()
{
  SensorData data;
  data.success = dht.getData();

  if (data.success)
  {
    data.temperatureC = dht.getTemperature();     //  celsius
    data.temperatureF = dht.getTemperature(true); // fahrenheit if true celsius of false
    data.humidity = dht.getHumidity();
    Serial.print("Temprature:");
    Serial.println(data.temperatureC);
  }
  else
  {
    data.temperatureC = -1;
    data.temperatureF = -1;
    data.humidity = -1;
    Serial.printf("Failed To sense data");
  }
  return data;
}

String floatToString(float value) {
  char buffer[32]; // Adjust buffer size as needed
  dtostrf(value, 1, 2, buffer); // Convert float to string with 2 decimal places
  return String(buffer);
}

void sendDataToFirebase(float temp)
{
  String floatString = floatToString(temp);
  Serial.printf("From Function %f", temp);
  Serial.print(temp);
  
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    if (Firebase.RTDB.setInt(&fbdo, "new/path", temp))
    {
      Serial.println("PASSED");
      Serial.printf("PATH: %s\n", fbdo.dataPath().c_str());
      Serial.printf("TYPE: %s\n", fbdo.dataType().c_str());
    }
    else
    {
      Serial.println("FAILED");
      Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
    }
  }
  else
  {
    Serial.println("FAILED");
    Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
  }
}
void setup()
{
  Serial.begin(115200);
  dht.begin();
  connectWifi();
  configDatabase();
}

void loop()
{
  SensorData data = getDataFromSensor();
  Serial.print("Sensor Data: ");
  Serial.print(data.temperatureC);
  sendDataToFirebase(data.temperatureC);
  delay(3000);
}

