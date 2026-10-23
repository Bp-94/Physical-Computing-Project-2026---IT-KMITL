#include <WiFiS3.h>
#include <MQTT.h>
#include <Servo.h>

// การตั้งค่า Wi-Fi และ MQTT 
const char  WIFI_SSID[] = ".. ... .. .. .";
const char  WIFI_PASSWORD[] = "0981039972";
const char  MQTT_BROKER_ADRRESS[] = "mqtt-dashboard.com";
const int   MQTT_PORT = 1883; 
const char  MQTT_CLIENT_ID[] = "arduino101Dalmatians"; 
const char  SUBSCRIBE_TOPIC[] = "projectPhyCom4Kon";

// กำหนด PIN
const int SERVO_PIN = 6;
const int RAIN_PIN  = 7;

// กำหนดองศาเปิด/ปืดหลังคา
const int ANGLE_OPEN  = 90;
const int ANGLE_CLOSE = 0;

WiFiClient net;
MQTTClient mqtt(256);
Servo roofServo;

bool commandOpen = false; //เป็นตัวแปรใช้เช็ค เพราะถ้าสั่งเปิดแล้ว หลังคาจะไม่ปิดจนกว่าจะสั่งปิด
bool commandClose = false; //ใช้ในกรณีที่สั่งปิดขณะฝนตก
String lastCommand = "";

int currentAngle = ANGLE_CLOSE;

String weatherStatus = "";
int dryCount = 0; //ใช้เช็คว่าฝนหยุดตกแล้วจริงมั้ย
const int COUNT_TO_DRY = 100; //loop() วนทุก ~50 ms -> 100 รอบประมาณ 5 วิ
String weatherLastSent = "";

void connectToWiFi(){
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED){
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to Wi-Fi!");
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT broker");
  while (!mqtt.connect(MQTT_CLIENT_ID, "", "")){
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to MQTT!");
  mqtt.subscribe(SUBSCRIBE_TOPIC);
}

void setAngle(int angle){
  roofServo.write(angle);
  currentAngle = angle;
  Serial.print("Servo Angle = ");
  Serial.println(currentAngle);
  if (currentAngle == ANGLE_OPEN){
    mqtt.publish(SUBSCRIBE_TOPIC, "ON");
  } else if (currentAngle == ANGLE_CLOSE){
    mqtt.publish(SUBSCRIBE_TOPIC, "OFF");
  }
}

void messageReceived(String &topic, String &payload){
  payload.trim();
  lastCommand = payload;

  if (payload == "STATUS") {
    if (currentAngle == ANGLE_OPEN) {
      mqtt.publish(SUBSCRIBE_TOPIC, "ON");
    } else if (currentAngle == ANGLE_CLOSE) {
      mqtt.publish(SUBSCRIBE_TOPIC, "OFF");
    }
    mqtt.publish(SUBSCRIBE_TOPIC, weatherStatus);
    weatherLastSent = weatherStatus;
    return;
  }

  int angle = payload.toInt();
  if (angle == 0 && payload != "0"){ //เช็คว่าไม่ใช่ตัวอักษร เพราะ ตัวอักษร.toInt() ได้ 0
    return;
  }
  if (angle < ANGLE_CLOSE || angle > ANGLE_OPEN){
    Serial.println("ANGLE OUT OF RANGE!");
    return;
  }
  if (angle == ANGLE_OPEN){
    commandOpen = true;
    commandClose = false;
  } else {
    commandOpen = false;
    if (weatherStatus == "RAIN"){
      commandClose = true;
    }
  }
  setAngle(angle);
}

void checkRainOpenRoof(){
  if (digitalRead(RAIN_PIN) == LOW){ //กรณีฝนตก
    weatherStatus = "RAIN";
    dryCount = 0;
    if (currentAngle != ANGLE_OPEN && !commandClose){
      setAngle(ANGLE_OPEN);
    }
  } else if (weatherStatus == "RAIN"){ //กรณีฝนหยุดตก
      dryCount++;
      if (dryCount >= COUNT_TO_DRY){
        weatherStatus = "DRY";
        dryCount = 0;
        commandClose = false;
        if (currentAngle != ANGLE_CLOSE && !commandOpen){
          setAngle(ANGLE_CLOSE);
        }
      }
  }
  if (weatherStatus != weatherLastSent){
    if (mqtt.publish(SUBSCRIBE_TOPIC, weatherStatus)){
      weatherLastSent = weatherStatus;
    }
  }
}

void setup(){
  Serial.begin(115200);
  pinMode(RAIN_PIN, INPUT);

  roofServo.attach(SERVO_PIN);
  roofServo.write(ANGLE_CLOSE);
  currentAngle = ANGLE_CLOSE;

  if (digitalRead(RAIN_PIN) == LOW){
    weatherStatus = "RAIN";
  } else {
    weatherStatus = "DRY";
  }

  connectToWiFi();
  mqtt.begin(MQTT_BROKER_ADRRESS, MQTT_PORT, net);
  mqtt.onMessage(messageReceived);
  mqtt.setTimeout(2000);
  connectToMQTT();

  mqtt.publish(SUBSCRIBE_TOPIC, "OFF");
}

void loop(){
  checkRainOpenRoof();
  mqtt.loop();
  delay(50);
}
