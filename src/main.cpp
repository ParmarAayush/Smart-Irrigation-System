#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Bonezegei_DHT11.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define USER_EMAIL "ayush@gmail.com"
#define USER_PASSWORD "ayush@123"
#define API_KEY "AIzaSyAWMqOfjaS_W2B-Gi9ef2N3Nt8xmTwZNpw"
#define DATABASE_URL "https://iot-devices-1f8c0-default-rtdb.firebaseio.com/"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

Bonezegei_DHT11 dht(14);

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
int fireStoreCount = 1;

struct SensorData
{
  float temperatureC;
  float temperatureF;
  int humidity;
  bool success;
};

// helper functions
String floatToString(float value)
{
  char buffer[32];              // Adjust buffer size as needed
  dtostrf(value, 1, 2, buffer); // Convert float to string with 2 decimal places
  return String(buffer);
}

void connectWifi()
{
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectAP", "password");
  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}
void configDatabase()
{
  config.database_url = DATABASE_URL;
}
void authenticateUser()
{
  Serial.print("Try to authenticate user");
  String uid;
  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;

  Firebase.begin(&config, &auth);
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "")
  {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.print(uid);
  signupOK = true;
  Serial.print("User Authenticated");
}

void sendDataToFirestore(float temp, int humidity)
{
  Serial.printf("\n Firestore Temprature %0.2f and Humidity %0.2f and int %d ", temp, humidity, humidity);
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 60000 || sendDataPrevMillis == 0))
  {
    FirebaseJson content;
    FirebaseJson content2;
    String docTempPath = "Soil/Temprature" + String(fireStoreCount);
    content.set("fields/temprature/doubleValue", temp);
    
    Serial.print("Create a document... ");
    if (Firebase.Firestore.createDocument(&fbdo, "iot-devices-1f8c0", "" /* databaseId can be (default) or empty */, docTempPath.c_str(), content.raw()))
    {

      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
    String docHumidityPath = "Soil/Humidity" + String(fireStoreCount);
    content2.set("fields/humidity/integerValue", humidity);
    fireStoreCount++;
    if (Firebase.Firestore.createDocument(&fbdo, "iot-devices-1f8c0", "" /* databaseId can be (default) or empty */, docHumidityPath.c_str(), content2.raw()))
    {

      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}

void sendDataToFirebase(float temp, float humidity)
{
  Serial.printf("\n Real Time: Temprature %0.2f and Humidity %0.2f and int %d ", temp, humidity, humidity);
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    // send temprature data
    if (Firebase.RTDB.setInt(&fbdo, "Soil/temprature", temp))
    {
      Serial.println("\nPASSED");
      // Serial.printf("PATH: %s\n", fbdo.dataPath().c_str());
      // Serial.printf("TYPE: %s\n", fbdo.dataType().c_str());
    }
    else
    {
      Serial.println("FAILED");
      Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
    }
    // send humidity data
    if (Firebase.RTDB.setInt(&fbdo, "Soil/humidity", humidity))
    {
      Serial.println("PASSED");
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
  Serial.print("\n");
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

void setup()
{
  Serial.begin(115200);
  connectWifi();
  delay(1000);
  configDatabase();
  authenticateUser();
  delay(4000);
  dht.begin();
}

void loop()
{
  if (Firebase.isTokenExpired())
  {
    Serial.print("Token Expire");
    Firebase.refreshToken(&config);
    Serial.println("Refresh token");
  }

  SensorData data = getDataFromSensor();
  // sendDataToFirebase(data.temperatureC, data.humidity);
  if (fireStoreCount < 4)
  {
    sendDataToFirestore(data.temperatureC, data.humidity);
  }
  delay(3000);
}
