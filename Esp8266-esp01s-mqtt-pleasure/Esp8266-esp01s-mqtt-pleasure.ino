// Engine PIN connected to GPIO0 (D3) of ESP8266 ESP-01S module
// Thermo PIN to GPIO2 (D4)

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ENGINE_PIN  0
#define THERMO_PIN  2

OneWire oneWire(THERMO_PIN);
DallasTemperature sensors(&oneWire);

const char *topic_pub_state = "tezos-iot/hackathon/device0001/state";
const char *topic_pub_event = "tezos-iot/hackathon/device0001/events";
const char *topic_sub1 = "tezos-iot/hackathon/device0001/ctl";

const char *ssid =  "Tezos";  // Имя вайфай точки доступа
const char *pass =  "hackathon"; // Пароль от точки доступа

const char *mqtt_server = "korotach.com"; // Имя сервера MQTT
const int mqtt_port = 1883; // Порт для подключения к серверу MQTT
const char *mqtt_user = "igor"; // Логи от сервер
const char *mqtt_pass = "igor1315"; // Пароль от сервера
const char *mqtt_device_id = "pleasure_device_0001";

float high_temp = 27.0;
int tm=300;
bool pleasure_detected = false;
String device_status = "Ready";
int g_loop = 0;

// Функция получения данных от сервера

void callback(const MQTT::Publish& pub)
{
  Serial.print(pub.topic());   // выводим в сериал порт название топика
  Serial.print(" => ");
  Serial.println(pub.payload_string()); // выводим в сериал порт значение полученных данных

  char payload[200];
  pub.payload_string().toCharArray(payload, 200);
  
  if(String(pub.topic()) == topic_sub1) // проверяем из нужного ли нам топика пришли данные 
  {
    DynamicJsonBuffer jsonBuffer(200);
    JsonObject& root = jsonBuffer.parseObject(payload);
    if (!root.success()) {
      Serial.println("JSON parsing failed!");
      return;
    } else {
        int pleasure_time = root["pleasure_time"];
        digitalWrite(ENGINE_PIN, HIGH);
        device_status = "RUN";
//        delay(pleasure_time * 1000);
        g_loop = pleasure_time;
//        tm = pleasure_time *333;
//        digitalWrite(ENGINE_PIN, LOW);
    }
  }
}

WiFiClient wclient;      
PubSubClient client(wclient, mqtt_server, mqtt_port);

void setup() {
  Serial.begin(115200);
  pinMode(ENGINE_PIN, OUTPUT);
  digitalWrite(ENGINE_PIN, LOW);
}

void loop() {
  // подключаемся к wi-fi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  // подключаемся к MQTT серверу
  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      Serial.println("Connecting to MQTT server");
      if (client.connect(MQTT::Connect(mqtt_device_id)
                         .set_auth(mqtt_user, mqtt_pass))) {
        Serial.println("Connected to MQTT server");
        client.set_callback(callback);
        client.subscribe(topic_sub1); // подписывааемся по топик с данными для насоса крипто-бармена
      } else {
        Serial.println("Could not connect to MQTT server");   
      }
    }

    if (client.connected()){
      client.loop();
      ReadySend();
    }  
  }
} // конец основного цикла


// Функция отправки в соотв. топик MQTT брокера признака готовности устройства
void ReadySend(){
  if (tm<=0)
  {
//    digitalWrite(ENGINE_PIN, LOW);
    String ready_status = "Ready";
    sensors.requestTemperatures();   // от датчика получаем значение температуры
    float temp = sensors.getTempCByIndex(0);
    client.publish(topic_pub_event, String(temp)); // отправляем в топик для термодатчика значение температуры
    Serial.println(temp);
    if (temp >= high_temp) {
      digitalWrite(ENGINE_PIN, LOW);
      device_status = "Success! ;)))";
    }
    if ((g_loop > 0 ) and (device_status == "RUN"))  {
      g_loop -= 1;
    } else {
 //     g_loop -= 1;
      digitalWrite(ENGINE_PIN, LOW);
      if ((g_loop > 0) and (device_status != "Success! ;)))")) {
        device_status = "Fail :(";
      } else {
        device_status = "Ready";
      }
    }
    client.publish(topic_pub_state, "{\"status\": \"" + device_status + "\", " + "\"temp\": " +String(temp) + "}");
    Serial.println(ready_status);
    tm = 300;  // пауза меду отправками признака готовности около 3 секунд
  }
  tm--; 
  delay(10);  
}
