#include <Arduino.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <Bonezegei_DHT11.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "time.h"

String USER_NAME;
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

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 3600;

struct SensorData
{
  float temperatureC;
  int humidity;
  float moisture;
  bool success;
};

// helper functions
String extractNameFromEmail(const String &email)
{
  int atIndex = email.indexOf('@');
  if (atIndex == -1)
  {
    return ""; // Invalid email address
  }

  String name = email.substring(0, atIndex);
  return name;
}

String printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return "";
  }
  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%d%B%Y::%H:%M", &timeinfo);
  return String(timeStr);
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

void sendDataToFirestore(String label, String key, float data, int dataCount)
{
  Serial.printf("Send to firestore => %s %0.2f", label, data); // 18
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 60000 || sendDataPrevMillis == 0))
  {
    FirebaseJson content;
    // /ayush/Slots/Slot 1/Soil/Data/09January2025::11:33:10H1
    // String docTempPath = USER_NAME + "/" + USER_EMAIL + "/Soil/" + label + key.charAt(0) + dataCount;
    // String docTempPath = USER_NAME + "/Slots/Slot1/Soil/" + label + key.charAt(0) + dataCount;
    String docTempPath = USER_NAME + "/Slots/Slot1/Soil/" + label + key.charAt(0) + dataCount + "/" + key;
    content.set("fields/" + key + "/doubleValue", data);



    if (Firebase.Firestore.createDocument(&fbdo, "iot-devices-1f8c0", "" /* databaseId can be (default) or empty */, docTempPath.c_str(), content.raw()))
    {
      Serial.printf(" => Recived to Firestore");
      // Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    }
    else
    {
      Serial.printf(" => Not Recived to Firestore");
      Serial.println(fbdo.errorReason());
    }
  }
}

void sendDataToFirebase(String label, float data)
{
  Serial.printf("Send to RTDB      => %s %0.2f ", label, data); // 13
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0))
  {
    // send temprature data
    if (Firebase.RTDB.setInt(&fbdo, "Soil/" + label, data))
    {
      Serial.println(" => Recived to RTDB");
    }
    else
    {
      Serial.println(" => Not Recived to RTDB");
      Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
    }
  }
  else
  {
    Serial.println("FAILED");
    Serial.printf("REASON: %s\n", fbdo.errorReason().c_str());
  }
}

SensorData getDataFromSensor()
{
  Serial.printf("\n-------------------------------------------------------------");
  dht.begin();
  SensorData data;
  data.success = dht.getData();

  if (data.success)
  {
    data.temperatureC = dht.getTemperature(); //  celsius
    data.humidity = dht.getHumidity();
  }
  else
  {
    data.temperatureC = -1;
    data.humidity = -1;
    Serial.printf("Failed To sense data");
  }
  Serial.printf("\nSensor Read       => Temprature %0.2f and Humidity => %d ", data.temperatureC, data.humidity); // 12
  return data;
}

void setup()
{
  Serial.begin(115200);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  connectWifi();
  delay(1000);
  configDatabase();
  authenticateUser();
  USER_NAME = extractNameFromEmail(USER_EMAIL);
  delay(4000);
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
  String timeSquare = printLocalTime();
  Serial.print(timeSquare);
  if (timeSquare == "")
  {
    Serial.print(timeSquare);
    Serial.printf("Failed to get time\n");
  }
  else
  {
    Serial.printf("\n");
    // Serial.print(timeSquare);
    if (fireStoreCount < 2)
    {
      sendDataToFirebase("temprature", data.temperatureC);
      sendDataToFirebase("humidity", data.humidity);

      sendDataToFirestore(timeSquare, "Temprature", data.temperatureC, fireStoreCount);
      Serial.printf("\n");
      sendDataToFirestore(timeSquare, "Humidity", data.humidity, fireStoreCount);
    }
    fireStoreCount++;
  }
  delay(6000);
}
