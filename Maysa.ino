#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include <TimeLib.h>
#include <Servo.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASS"

#define API_KEY "APIKEY"
#define DATABASE_URL "https://feeder-dde1d-default-rtdb.europe-west1.firebasedatabase.app"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int timestamp;
String feedNow;
bool oldMode;
bool mode;
int oldTime1;
int oldTime2;
int oldTime3;
int oldTime4;
int oldInterval;
int oldFeedTime;
int prevAngle;

Servo servo;

void feed() {
  int feedPortion;
  int amount;
  timestamp = timeClient.getEpochTime();  
  Firebase.RTDB.getInt(&fbdo, "main/amount", &amount);
  if (amount >= 60 && amount <= 180) {
    feedPortion = amount / 60;
    for (int i = feedPortion; i >= 1; i--) {
      int servoAngle = servo.read();
      if (servoAngle < 5) {
        prevAngle = 0;
        servo.write(60);
        delay(1000);
      } else if (servoAngle > 55 && servoAngle < 65) {
        if (prevAngle == 0) {
          prevAngle = 60;
          servo.write(120);
          delay(1000);
        } else {
          prevAngle = 60;
          servo.write(0);
          delay(1000);
        }
      } else if (servoAngle > 115 && servoAngle < 125) {
        if (prevAngle == 60) {
          prevAngle = 120;
          servo.write(180);
          delay(1000);
        } else {
          prevAngle = 120;
          servo.write(60);
          delay(1000);
        }
      } else if (servoAngle > 175) {
        prevAngle = 180;
        servo.write(120);
        delay(1000);
      }
    }
    Firebase.RTDB.setInt(&fbdo, "main/feedTime", timestamp);    
  }  
}

void intervalFeed() {
  int interval;
  int feedTime;
  timestamp = timeClient.getEpochTime();
  Firebase.RTDB.getInt(&fbdo, "interval/interval", &interval);
  Firebase.RTDB.getInt(&fbdo, "main/feedTime", &feedTime);
  if (interval == oldInterval && oldFeedTime == feedTime) {
    if (timestamp >= feedTime + interval) {
      feed();      
    }
  } else {
    Firebase.RTDB.getInt(&fbdo, "interval/interval", &interval);
    Firebase.RTDB.getInt(&fbdo, "main/feedTime", &feedTime);
    oldInterval = interval;
    oldFeedTime = feedTime;
    if (timestamp >= feedTime + interval) {
      feed();      
    }
  }
}

void feedNowFun() {
  feed();
  Firebase.RTDB.setString(&fbdo, "main/feedNow", "dontFeed");  
}

void scheduledFeed() {
  timestamp = timeClient.getEpochTime();
  int numberOfFeed;
  int time1;
  int time2;
  int time3;
  int time4;
  int feedTime;
  Firebase.RTDB.getInt(&fbdo, "scheduled/numberOfFeed", &numberOfFeed);
  Firebase.RTDB.getInt(&fbdo, "scheduled/time1", &time1);
  Firebase.RTDB.getInt(&fbdo, "scheduled/time2", &time2);
  Firebase.RTDB.getInt(&fbdo, "scheduled/time3", &time3);
  Firebase.RTDB.getInt(&fbdo, "scheduled/time4", &time4);
  Firebase.RTDB.getInt(&fbdo, "main/feedTime", &feedTime);
  if (numberOfFeed >= 1 && numberOfFeed <= 4 && time1 == oldTime1 && time2 == oldTime2 && time3 == oldTime3 && time4 == oldTime4) {
    if (numberOfFeed >= 1 && hour(timestamp) == hour(time1) && minute(timestamp) >= minute(time1) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 2 && hour(timestamp) == hour(time2) && minute(timestamp) >= minute(time2) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 3 && hour(timestamp) == hour(time3) && minute(timestamp) >= minute(time3) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 4 && hour(timestamp) == hour(time4) && minute(timestamp) >= minute(time4) && timestamp >= feedTime + 3600) {
      feed();      
    }
  } else {
    Firebase.RTDB.getInt(&fbdo, "scheduled/numberOfFeed", &numberOfFeed);
    Firebase.RTDB.getInt(&fbdo, "scheduled/time1", &time1);
    Firebase.RTDB.getInt(&fbdo, "scheduled/time2", &time2);
    Firebase.RTDB.getInt(&fbdo, "scheduled/time3", &time3);
    Firebase.RTDB.getInt(&fbdo, "scheduled/time4", &time4);
    oldTime1 = time1;
    oldTime2 = time2;
    oldTime3 = time3;
    oldTime4 = time4;
    if (numberOfFeed >= 1 && hour(timestamp) == hour(time1) && minute(timestamp) >= minute(time1) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 2 && hour(timestamp) == hour(time2) && minute(timestamp) >= minute(time2) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 3 && hour(timestamp) == hour(time3) && minute(timestamp) >= minute(time3) && timestamp >= feedTime + 3600) {
      feed();      
    } else if (numberOfFeed >= 4 && hour(timestamp) == hour(time4) && minute(timestamp) >= minute(time4) && timestamp >= feedTime + 3600) {
      feed();      
    }
  }
}

void setup() {
  fbdo.setBSSLBufferSize(2048, 2048);
  fbdo.setResponseSize(2048);  
  servo.attach(D1, 500, 2450);
  servo.write(0);  
  Serial.begin(9600);
  timeClient.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    
    timeClient.update();
    Firebase.RTDB.getString(&fbdo, "main/feedNow", &feedNow);
    if (feedNow == "feed") {
      feedNowFun();
    } else {
      Firebase.RTDB.getBool(&fbdo, "main/mode", &mode);
      if (oldMode == mode) {
        if (mode == true) {
          intervalFeed();
        } else {
          scheduledFeed();
        }
      } else {
        Firebase.RTDB.getBool(&fbdo, "main/mode", &mode);
        oldMode = mode;
        if (mode == true) {
          intervalFeed();
        } else {
          scheduledFeed();
        }
      }
    }
  }
}