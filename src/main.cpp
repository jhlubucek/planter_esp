#include <Arduino.h>
//#include <stdio.h>
#include "WiFi.h"
#include "PubSubClient.h"
#include "DHT.h"
#include "time.h"
#include "Config.h"
#include "AnalogSensor.h"

// sensors
AnalogSensor lightSensor(LIGHT_SENSOR_PIN, LIGHT_SENSOR_MIN_VALUE, LIGHT_SENSOR_MAX_VALUE, SENSOR_SWITCH_PIN);
AnalogSensor soilMoistureSensor(MOISTURE_SENSOR_PIN, MOISTURE_SENSOR_MIN_VALUE, MOISTURE_SENSOR_MAX_VALUE, SENSOR_SWITCH_PIN);
AnalogSensor waterLevelSensor(WATER_LEVEL_SENSOR_PIN, WATER_LEVEL_SENSOR_MIN_VALUE, WATER_LEVEL_SENSOR_MAX_VALUE, SENSOR_SWITCH_PIN);
DHT dht(DHTPIN, DHTTYPE);

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600; //gtm +1
const int   daylightOffset_sec = 3600; //dayligh saving

char lightSensorTopic[50];
char soilMoistureSensorTopic[50];
char waterLevelSensorTopic[50];
char airTemperatureSensorTopic[50];
char airHumuditySensorTopic[50];
char waterPumpTopic[50];

char waterPumpTopicPayload[50];
int waterPumpTopicPayloadLenght = 0;

int secTillNext;
unsigned long miliSecLast;
unsigned long miliSecNext;

int err = 0; //wifi:1  MQTT:2

WiFiClient espClient;
PubSubClient client(espClient);

//String formattedDate;
//String dayStamp;
//String timeStamp;

void updateSensorData(){

};

void callback(char* topic, byte* payload, unsigned int length) {

  //print message
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
  if (String(topic) == String(waterPumpTopic))
  {
    Serial.println("rovnaji se");
    //max payload lenght 50
    waterPumpTopicPayloadLenght = length > 50 ? 50 : length;
    for (int i = 0; i < waterPumpTopicPayloadLenght; i++) {
      waterPumpTopicPayload[i] = (char)payload[i];  
    }
    Serial.println(waterPumpTopicPayload);
  } else {
   Serial.println("nope"); 
  }
};

void updateLed(){
  if (waterLevelSensor.getLastValue() > 80)
  {
    digitalWrite(WATER_LED_R_PIN, LOW);
    digitalWrite(WATER_LED_G_PIN, HIGH);
    digitalWrite(WATER_LED_B_PIN, LOW);
  }else if (waterLevelSensor.getLastValue() > 20)
  {
    digitalWrite(WATER_LED_R_PIN, LOW);
    digitalWrite(WATER_LED_G_PIN, LOW);
    digitalWrite(WATER_LED_B_PIN, HIGH);
  }else{
    digitalWrite(WATER_LED_R_PIN, HIGH);
    digitalWrite(WATER_LED_G_PIN, LOW);
    digitalWrite(WATER_LED_B_PIN, LOW);
  }
  
  switch (err)
  {
  case 0:
    digitalWrite(ERR_LED_R_PIN, LOW);
    digitalWrite(ERR_LED_G_PIN, HIGH);
    digitalWrite(ERR_LED_B_PIN, LOW);
    break;
  case 2:
    digitalWrite(ERR_LED_R_PIN, HIGH);
    digitalWrite(ERR_LED_G_PIN, LOW);
    digitalWrite(ERR_LED_B_PIN, HIGH);
    break;
  default:
    digitalWrite(ERR_LED_R_PIN, HIGH);
    digitalWrite(ERR_LED_G_PIN, LOW);
    digitalWrite(ERR_LED_B_PIN, LOW);
    break;
  }
  
};

void publishData(){
  client.publish(lightSensorTopic, String(lightSensor.getValue()).c_str());
  client.publish(waterLevelSensorTopic, String(waterLevelSensor.getValue()).c_str());
  client.publish(soilMoistureSensorTopic, String(soilMoistureSensor.getValue()).c_str());
  client.publish(airTemperatureSensorTopic, String(dht.readTemperature()).c_str());
  client.publish(airHumuditySensorTopic, String(dht.readHumidity()).c_str());
  //Serial.print("waterlevel: "); Serial.println(waterLevelSensor.getLastValue());
};

void setup_wifi() {

  // delay(10);
  // We start by connecting to a WiFi network
  //Serial.println();
  //Serial.print("Connecting to ");
  //Serial.println(WIFI_SSID);
  //Serial.println(WiFi.status());
  

  if (WiFi.status() != WL_CONNECTED){  
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWD);

    int counter = 0;
    while (WiFi.status() != WL_CONNECTED && counter < 60) {
      counter ++;
      delay(500);
      //Serial.print(".");
    }
  }

  // update err
  if (WiFi.status() != WL_CONNECTED){
    err += err == 0 || err == 2 ? 1 : 0;
    Serial.println("WiFi failed");
  }else{
    err -= err == 1 || err == 3 ? 1 : 0;
      //Serial.println("");
      //Serial.println("WiFi connected");
      //Serial.println("IP address: ");
      //Serial.println(WiFi.localIP());
  }
  

  randomSeed(micros());

  
}

void reconnect() {
  
  int counter = 0;
  if (!client.connected())
  {
    do {
      counter ++;
      //Serial.print("Attempting MQTT connection...");
      String clientId = "ESP8266Client-";
      clientId += String(random(0xffff), HEX);
      // Attempt to connect
      if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWD)) {
        Serial.println("connected MQTT");
        // Once connected, publish an announcement...
        //client.publish("outTopic", "hello world");
        // ... and resubscribe
        client.subscribe("inTopic");
        client.subscribe(waterPumpTopic);
      } else {
        //Serial.print("failed, rc=");
        //Serial.print(client.state());
        //Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
      }
    } while (!client.connected() && counter < 3);
  }

  
  

  if (!client.connected()){
    err += err == 0 || err == 1 ? 2 : 0;
  }else{
    err -= err == 2 || err == 3 ? 2 : 0;
  }
      
}

void runPump(){
  if (waterPumpTopicPayloadLenght == 1 && waterPumpTopicPayload[0] == '1')
  {
    digitalWrite(WATER_PUMP_PIN, HIGH);
    delay(WATER_PUMP_TIME);
    digitalWrite(WATER_PUMP_PIN, LOW);
    client.publish(waterPumpTopic,"2",true);
  }
  
}

void loopClient(){
  //Serial.println("looping client");
  unsigned long starttime = millis();
  while ((millis() - starttime) <=5000) // do this loop for up to 1000mS
  {
    client.loop();
  }
  Serial.println("done");
}


void setup() {
  Serial.begin(9600);
  Serial.println("test");
  sprintf(lightSensorTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"lightSensor");
  sprintf(waterLevelSensorTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"waterLevelSensor");
  sprintf(soilMoistureSensorTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"soilMoistureSensor");
  sprintf(airHumuditySensorTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"airHumiditySensor");
  sprintf(airTemperatureSensorTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"airTemperatureSensor");
  sprintf(waterPumpTopic,"%s/%s/%s", USER_ID, PLANTER_NAME,"waterPump");
  // setup inputs and outputs
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(WATER_LED_R_PIN, OUTPUT);
  pinMode(WATER_LED_G_PIN, OUTPUT);
  pinMode(WATER_LED_B_PIN, OUTPUT);
  pinMode(ERR_LED_R_PIN, OUTPUT);
  pinMode(ERR_LED_G_PIN, OUTPUT);
  pinMode(ERR_LED_B_PIN, OUTPUT);
  pinMode(SENSOR_SWITCH_PIN, OUTPUT);
  digitalWrite(SENSOR_SWITCH_PIN,LOW);

  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(WATER_LEVEL_SENSOR_PIN, INPUT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);

  dht.begin();

  setup_wifi();
  client.setServer(MQTT_SERVER, 1883);
  client.setCallback(callback);
  reconnect();
  client.subscribe(waterPumpTopic);
  loopClient();
  Serial.println(lightSensorTopic);
  Serial.println(waterLevelSensorTopic);
  Serial.println(soilMoistureSensorTopic);
  Serial.println(airTemperatureSensorTopic);
  Serial.println(airHumuditySensorTopic);
  Serial.println(waterPumpTopic);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  miliSecLast = miliSecNext = micros();
}

int NextWakeup(){
  
  return 5;

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    //Serial.println("Failed to obtain time");
    return 60*5;
  }
  
  char t[3];
  strftime(t,3, "%M", &timeinfo);
  int total = atoi( t );
  while (total>=0)
  {
    total -=5;
  }
  total *= -60;
  
  strftime(t,3, "%S", &timeinfo);
  total -= atoi( t );
  return total;
}

void disconnect(){
  client.disconnect();
  WiFi.disconnect(true);
}

//void goToSleep(){
//  Serial.flush(); 
//  esp_sleep_enable_timer_wakeup(1000000 * 5);
//  gpio_deep_sleep_hold_en();
//	gpio_hold_en((gpio_num_t)WATER_LED_R_PIN);
//  gpio_hold_en((gpio_num_t)WATER_LED_G_PIN);
//  gpio_hold_en((gpio_num_t)WATER_LED_B_PIN);
//  gpio_hold_en((gpio_num_t)ERR_LED_R_PIN);
//  gpio_hold_en((gpio_num_t)ERR_LED_G_PIN);
//  gpio_hold_en((gpio_num_t)ERR_LED_B_PIN);
//  esp_deep_sleep_start();
//}

bool nextPublish(){
    //Serial.println("time");
  unsigned long now = micros();
  //Serial.println(miliSecLast);
  //Serial.println(miliSecNext);
  //Serial.println(now);
  if (miliSecLast > miliSecNext)
  {
    return (now < miliSecLast && now >= miliSecNext);
  }else{
    return (now < miliSecLast || now >= miliSecNext);
  }
}

void loop() {
  //Serial.println("loop");  
  setup_wifi();
  reconnect();
  client.loop();
  if (!err)
  {
    if (nextPublish()){
      publishData();
      miliSecLast = micros();
      Serial.print("xxx:  ");
      Serial.println(miliSecLast);
      Serial.println(miliSecNext);
      miliSecNext = miliSecLast + (NextWakeup() * 1000000);
      
    }
    runPump();
  }
  updateLed();

  
  //disconnect();

  //goToSleep();
  //delay(secToWake*1000);  
};

