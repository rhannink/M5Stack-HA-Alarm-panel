


// M5Stack Home Assistant RFID MQTT alarm panel
// Author: Remco Hannink
// Version: 0.3
// Date: 31-04-2020

#include "FS.h"
#include "SPIFFS.h"
#include "M5Stack.h"
#include <M5ez.h>
#include <ezTime.h>
#include <Wire.h>
#include "MFRC522_I2C.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Tone32.h>
#include <pitches.h>

// set MQTT parameters to overcome constant reconnection

#define MQTT_MAX_PACKET_SIZE 512
#define MQTT_KEEPALIVE 60

// The parameters below should be adapted to your own situation

const char ssid[] = "WiFi SSID";
const char pass[] = "WiFi password";
const char* mqtt_host = "mqtt.hostname (FQDN)";
const int mqtt_port = 8883;
const char* mqtt_user = "MQTT-userID";
const char* mqtt_pass = "MQTT-password";
const char* mqtt_clientId = "M5stack-alarm1";
const char* mqtt_state_topic = "home/alarm";
const char* mqtt_command_topic = "home/alarm/set";
const char* mqtt_state_full = "armed_away";
const char* mqtt_state_part = "armed_home";
const char* mqtt_state_off = "disarmed";
const char* mqtt_state_pending = "pending";
const char* mqtt_command_full = "ARM_AWAY";
const char* mqtt_command_part = "ARM_HOME";
const char* mqtt_command_off = "DISARM";
const char* display_state_full ="Fully Armed";
const char* display_state_part ="Partially Armed";
const char* display_state_off ="Switched Off";
const char* display_state_pending ="Pending";
const char* display_present_card = "Please present Card!";
const char* button_a_text = "Off";
const char* button_b_text = "Home";
const char* button_c_text = "Away";

// Adding the LetsEncrypt CA to verify the MQTT server certificate

const char* ca_cert = \ 
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure m5stackClient;
PubSubClient client(m5stackClient);
long lastMsg = 0;
char msg[50];
char buttonString[50];
String alarmStatus ="";
byte readCard[4];

//Array with valid RFID UID's
byte uidArray[3][4]={
  {0x11,0x22,0x33,0x44},
  {0x12,0x22,0x33,0x45},
  {0x13,0x22,0x33,0x46},  
};

//Array with accompanying user names 
String nameArray[3]={
  "Card1","Card2","Card3"
};

int beepVolume=1;
int screenTimeout = 10000; // screen timeout in mSec
int screenStatus = 1;

MFRC522 mfrc522(0x28); 

void setup(){
    #include <themes/default.h>
  
    M5.begin(true, false, true, true);

    // turn off the M5stack amplifier to overcome I2C interference with speaker. 
    // For modification see: http://community.m5stack.com/topic/367/mod-to-programmatically-disable-speaker-whine-hiss
    
    pinMode(5,OUTPUT);
    digitalWrite(5, LOW);

    // Setup Wifi
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Mount SPIFFS for icons and pictures
    if(!SPIFFS.begin(true)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // initialize ez5stack screen and buttons
    ez.begin();
    ez.canvas.clear();
    M5.Lcd.fillScreen(TFT_WHITE);
    strcpy(buttonString,button_a_text);
    strcat(buttonString, " # ");
    strcat(buttonString,button_b_text);
    strcat(buttonString, " # ");
    strcat(buttonString,button_c_text);
    ez.buttons.show(buttonString);

    // Initialize RFID reader
    mfrc522.PCD_Init();             // Init MFRC522
    
    m5stackClient.setCACert(ca_cert);
    client.setServer(mqtt_host, mqtt_port);
    client.setCallback(callback);
    reconnect();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicText= "";
  M5.Lcd.setBrightness(50);
  screenTimeout=10000;
  screenStatus = 1;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    
    topicText +=((char)payload[i]);
  }
  Serial.println();
  
  // Set M5Stack screen according to incoming state message
  if (topicText == mqtt_state_part){
    beep(400);
    M5.Lcd.drawJpgFile(SPIFFS, "/orange.jpg", 80, 40);
    ez.header.show(display_state_part);
    alarmStatus = mqtt_state_part;
  }
    if (topicText == mqtt_state_off){
    beep(50);
    delay(100);
    beep(50);
    delay (100);
    beep(50);
    M5.Lcd.drawJpgFile(SPIFFS, "/blue.jpg", 80, 40);
    ez.header.show(display_state_off);
    alarmStatus = mqtt_state_off;
  }
    if (topicText == mqtt_state_full){
    beep(400);
    M5.Lcd.drawJpgFile(SPIFFS, "/red.jpg", 80, 40);
    ez.header.show(display_state_full);
    alarmStatus = mqtt_state_full;
  }
    if (topicText == mqtt_state_pending){
    beep(100);
    delay(100);
    M5.Lcd.drawJpgFile(SPIFFS, "/yellow.jpg", 80, 40);
    ez.header.show(display_state_pending);
    alarmStatus = mqtt_state_pending;
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
   if (client.connect(mqtt_clientId,mqtt_user,mqtt_pass)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(mqtt_command_topic, "Connected");
      // ... and resubscribe
      client.subscribe(mqtt_state_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  M5.update();
  screenTimeout = screenTimeout-200;

  // If screenTimeout is zero then reset timer
  
  if (screenTimeout == 0){
    screenTimeout=10000;
    
    // if screenTimeout is zero and screenStatus is on, the dim the screen
    
    if (screenStatus == 1){
  
      // if timer is zero and screen is on, dim the screen graduately in 50 steps ans set screen Status to 0
      
      for (int i = 50; i >= 0; i--) {
        M5.Lcd.setBrightness(i);
        delay(20);
      }
      screenStatus = 0;
    }
  }
  
  // If alarm status equals pending, then make sure screen stays on
  
  if (alarmStatus == mqtt_state_pending){
    M5.Lcd.setBrightness(50);
    screenStatus = 1;
    screenTimeout = 10000;
  }

  // If button A is pressed and status is not already off and screen is already on, the enable alarm off and ask for RFID token 
    
  if(M5.BtnA.wasPressed() && (alarmStatus != mqtt_state_off) && (screenStatus == 1))  {
    beep(100);
    ez.header.show(display_present_card);
    Serial.println(display_present_card);
    screenTimeout = 10000;
    screenStatus = 1;
  }

  // If button B is pressed and status is not already home and screen is on, then enable home away mode
  
  if(M5.BtnB.wasPressed() && (alarmStatus != mqtt_state_part) && (screenStatus == 1)) {
    beep(100);
    M5.Lcd.drawJpgFile(SPIFFS, "/orange.jpg", 80, 40);
    ez.header.show(display_state_part);
    client.publish(mqtt_command_topic, mqtt_command_part);
    Serial.println(mqtt_command_part);
    screenTimeout = 10000;
  }

  // If button C is pressed and status is not already away and screen is on, then enable away mode
    
  if(M5.BtnC.wasPressed() && (alarmStatus != mqtt_state_full) && (screenStatus == 1)) {
    beep(100);
    M5.Lcd.drawJpgFile(SPIFFS, "/red.jpg", 80, 40);
    ez.header.show(display_state_full);
    client.publish(mqtt_command_topic, mqtt_command_full);
    Serial.println(mqtt_command_full);
    screenTimeout = 10000;
  }

  // If any button was pressed while screen is off, the just turn screen on and do nothing else
  
  if((M5.BtnA.wasPressed() || M5.BtnB.wasPressed() || M5.BtnC.wasPressed()) && (screenStatus == 0)){
    M5.Lcd.setBrightness(50);
    screenTimeout=10000;
    screenStatus = 1;
    delay(200);
  }
  
  // Look for new cards, and select one if present
  
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(200);
    return;
  }
  
  // Now a card is read. The UID is in mfrc522.uid.
  // sound beep and turn on screen;

  M5.Lcd.setBrightness(50);
  screenTimeout = 10000;
  screenStatus = 1;
  beep(100);  

  // Only act if alarm state != off, else, ignore card
    
  if (alarmStatus != mqtt_state_off) {

    Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      readCard[i] = mfrc522.uid.uidByte[i];
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    } 
    Serial.println();
    client.publish(mqtt_state_topic, "RFID card detected");

    // check if RFID UID is valid one and find username
    
    for(int i=0; i<3; i++){
      Serial.print("Testing Match with checkArray ");
      Serial.println(i);
      if((memcmp(readCard,uidArray[i],4))==0){
        // UID is valid, disarm the alarm and break
        Serial.print("Arrays Match, name is:");
        Serial.println(nameArray[i]);
        M5.Lcd.drawJpgFile(SPIFFS, "/blue.jpg", 80, 40);
        ez.header.show(nameArray[i]);
        client.publish(mqtt_command_topic, mqtt_command_off);
        break;
      }
    }  
  }
 
}

uint8_t music[1000];
void tone_volume(uint16_t frequency, uint32_t duration) {
  float interval=0.001257 * float(frequency);
  float phase=0;
  for (int i=0;i<1000;i++) {
    music[i]=127+126 * sin(phase);
    phase+=interval;
  }
  music[999]=0;
  int remains=duration;
  for (int i=0;i<duration;i+=200) {
    if (remains<200) {
      music[remains * 999/200]=0;
    }
    M5.Speaker.playMusic(music,5000);
    remains-=200;
  }
}

void beep(int leng) {
  digitalWrite(5, HIGH);
  delay(150);
  M5.Speaker.setVolume(1);
  M5.Speaker.update();
  tone_volume(1000, leng);
  digitalWrite(5, LOW);
}
  
  
