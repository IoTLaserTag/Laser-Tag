#include <stdio.h>
#include <string.h>
#include <Adafruit_CircuitPlayground.h>

#if !defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS)
  #error "Infrared support is only for the Circuit Playground Express, it doesn't work with the Classic version"
#endif

#define TINY_GSM_MODEM_ESP8266

#define SerialMon Serial
#define SerialAT Serial1

#define MQTT_USERNAME "PutYourOwnMQTTUserName"
#define MQTT_PASSWORD "PutYourOwnMQTTPassword"

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "PutYourOwnSSIDName";
const char wifiPass[] = "PutYourOwnWifiPassword";

// MQTT details
const char MQTT_CLIENTID[] = MQTT_USERNAME __DATE__ __TIME__;

const char* broker = "io.adafruit.com";

uint32_t lastReconnectAttemptTimer = 0;
uint32_t lastReconnectAttemptLive = 0;
uint32_t lastReconnectAttemptOutOfLife = 0;

char mqttMsgLive[50];
String liveRateStr;
char mqttMsgOutOfLife[50];
String outOfLifeStr;
int OutOfLifeCounter=0;
String minutesStr;
char mqttMsgTimerSeconds[50];
char mqttMsgTimerMinutes[50];
String secondsStr;
long lastPublishedLive = 0;
long lastPublishedTimer = 0;
long lastPublishedOutOfLife = 0;

const char laserTagTopicLive[] = MQTT_USERNAME "/feeds/players.laser-tag-player-1";
const char laserTagTopicGameTimer[] = MQTT_USERNAME "/feeds/game-time.game-time";
const char laserTagTopicOutOfLife[] = MQTT_USERNAME "/feeds/death-count.death-count-1";

#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define HITVALUE 3772802968
#define NUMOFLEDS 10
#define MAXCOLORVALUE 255
#define MINCOLORVALUE 0

// Vars of voice commands
const uint8_t spUH[]            PROGMEM = {0x63,0x2A,0xAC,0x2B,0x8D,0xF7,0xEC,0xF1,0xB6,0xB7,0xDD,0xDD,0xEC,0xC7,0x5A,0x58,0x55,0x39,0xF5,0x9E,0x6B,0x3D,0xD3,0x59,0xB8,0x67,0x39,0xEE,0x8A,0x77,0x7A,0xAB,0x54,0x6F,0xC7,0x4C,0xF6,0x91,0xCF,0xFF,0x03};
const uint8_t spREADY[]         PROGMEM = {0x6A,0xB4,0xD9,0x25,0x4A,0xE5,0xDB,0xD9,0x8D,0xB1,0xB2,0x45,0x9A,0xF6,0xD8,0x9F,0xAE,0x26,0xD7,0x30,0xED,0x72,0xDA,0x9E,0xCD,0x9C,0x6D,0xC9,0x6D,0x76,0xED,0xFA,0xE1,0x93,0x8D,0xAD,0x51,0x1F,0xC7,0xD8,0x13,0x8B,0x5A,0x3F,0x99,0x4B,0x39,0x7A,0x13,0xE2,0xE8,0x3B,0xF5,0xCA,0x77,0x7E,0xC2,0xDB,0x2B,0x8A,0xC7,0xD6,0xFA,0x7F};
const uint8_t spGO[]            PROGMEM = {0x06,0x08,0xDA,0x75,0xB5,0x8D,0x87,0x4B,0x4B,0xBA,0x5B,0xDD,0xE2,0xE4,0x49,0x4E,0xA6,0x73,0xBE,0x9B,0xEF,0x62,0x37,0xBB,0x9B,0x4B,0xDB,0x82,0x1A,0x5F,0xC1,0x7C,0x79,0xF7,0xA7,0xBF,0xFE,0x1F};
const uint8_t spOFF[]           PROGMEM = {0x6B,0x4A,0xE2,0xBA,0x8D,0xBC,0xED,0x66,0xD7,0xBB,0x9E,0xC3,0x98,0x93,0xB9,0x18,0xB2,0xDE,0x7D,0x73,0x67,0x88,0xDD,0xC5,0xF6,0x59,0x15,0x55,0x44,0x56,0x71,0x6B,0x06,0x74,0x53,0xA6,0x01,0x0D,0x68,0x80,0x03,0x1C,0xF8,0x7F};
const uint8_t spHIGH[]          PROGMEM = {0x04,0xC8,0x7E,0x9C,0x02,0x12,0xD0,0x80,0x06,0x56,0x96,0x7D,0x67,0x4B,0x2C,0xB9,0xC5,0x6D,0x6E,0x7D,0xEB,0xDB,0xDC,0xEE,0x8C,0x4D,0x8F,0x65,0xF1,0xE6,0xBD,0xEE,0x6D,0xEC,0xCD,0x97,0x74,0xE8,0xEA,0x79,0xCE,0xAB,0x5C,0x23,0x06,0x69,0xC4,0xA3,0x7C,0xC7,0xC7,0xBF,0xFF,0x0F};

// LaserTag vars
bool isFirstTimer = true;
bool isBoardOn = false;
int ledRedColor = 0;
int ledGreenColor = 0;
long lastMS = 0;
long timerMS = 0;
int liveRate = 10;

int redLiveColorLed[] = {153,0,0,0,0};
int greenLiveColorLed[] = {255,102,51,0,0};
int blueLiveColorLed[] = {255,204,102,204,51};

// Entry Point
void setup() {
  CircuitPlayground.begin();
  mqttSet();
  Serial.begin(9600);  
  CircuitPlayground.irReceiver.enableIRIn(); // Start the receiver
  Serial.println("Ready to receive IR signals");
  isFirstTimer = true;
}

// Main Loop
void loop() {
  timerUpdate(); // Update the game timer
  if(checkIfOn()){
    // Update data on game dashboard
    writeLiveDataToMQTT();
    writeOutOfLifeDataToMQTT();

    // Check if player got hit and he still have life
    if (CircuitPlayground.irReceiver.getResults() && liveRate > 0) {
      CircuitPlayground.irDecoder.decode();           // Decode it
      if(isMatchValue(CircuitPlayground.irDecoder.valueResults())){
        Serial.println(CircuitPlayground.irDecoder.valueResults(),HEX);
        // Create hit effect
        hitEffect();
      }
    }

    //Restart receiver    
    CircuitPlayground.irReceiver.enableIRIn();
  }
}

// Connect to Wifi and set mqtt settings
void mqttSet(){
  SerialMon.begin(115200);
  delay(1000);
  SerialAT.begin(115200);
  SerialMon.println("Initializing Modem");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print(F("Setting SSID & password."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println("Failed");
    while(true);
  }
  SerialMon.println("Success");

  SerialMon.print("Waiting for network.");
  if (!modem.waitForNetwork()) {
    SerialMon.println("Failed");
    while(true);
  }
  SerialMon.println("Success");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }
  
  mqtt.setServer(broker, 1883);    
}

// Connect to mqtt server
boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Check if you created a succeful connection
  boolean status = mqtt.connect(MQTT_CLIENTID, MQTT_USERNAME, MQTT_PASSWORD);
  if (status == false) {
    SerialMon.println("Failed");
    return false;
  }
  
  SerialMon.println("Success");
  return mqtt.connected();
}

// Write the amount of life the player as left to the mqtt dashboard
void writeLiveDataToMQTT(){
  if (!mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttemptLive > 10000L) {
      lastReconnectAttemptLive = t;
      if (mqttConnect()) {
        lastReconnectAttemptLive = 0;
      }
    }
    delay(100);
    return;
  }
  mqtt.loop();

  // Update the mqtt on minimum 10000ms interval, to prevent of adafruit dashboard from blocking.
  if (millis() - lastPublishedLive > 10000) {
    Serial.println("Publishing LiveRate");
    // Update player life dashboard
    liveRateStr = String((liveRate/2) * 20);
    liveRateStr.toCharArray(mqttMsgLive, liveRateStr.length() + 1);
    Serial.println(mqttMsgLive);
    mqtt.publish(laserTagTopicLive, mqttMsgLive);
    lastPublishedLive = millis();
  }
}

// Write the out of life  data to the mqtt dashboard
void writeOutOfLifeDataToMQTT(){
  if (!mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttemptOutOfLife > 10000L) {
      lastReconnectAttemptOutOfLife = t;
      if (mqttConnect()) {
        lastReconnectAttemptOutOfLife = 0;
      }
    }
    delay(100);
    return;
  }
  mqtt.loop();

  // Update the mqtt on minimum 10000ms interval, to prevent of adafruit dashboard from blocking.
  if (millis() - lastPublishedOutOfLife > 10000) {
    Serial.println("Publishing OutOfLife");
    // Update player out of life dashboard
    outOfLifeStr = String(OutOfLifeCounter);
    outOfLifeStr.toCharArray(mqttMsgOutOfLife, outOfLifeStr.length() + 1);
    Serial.println(mqttMsgOutOfLife);
    mqtt.publish(laserTagTopicOutOfLife, mqttMsgOutOfLife);
    lastPublishedOutOfLife = millis();
  }
}


// Write the game time to the mqtt dashboard
void writeTimerDataToMQTT(int secondsTimer, int minutesTimer){
  if (!mqtt.connected()) {
    uint32_t t = millis();
    if (t - lastReconnectAttemptTimer > 10000L) {
      lastReconnectAttemptTimer = t;
      if (mqttConnect()) {
        lastReconnectAttemptTimer = 0;
      }
    }
    delay(100);
    return;
  }
  mqtt.loop();

  // Update the mqtt on minimum 10000ms interval, to prevent of adafruit dashboard from blocking.
  if (millis() - lastPublishedTimer > 10000) {
    Serial.println("Publishing Time");
    char mqttMsgTimer [100] = "";
   
    minutesStr = String(minutesTimer);
    secondsStr = String(secondsTimer);

    minutesStr.toCharArray(mqttMsgTimerMinutes, minutesStr.length() + 1);
    secondsStr.toCharArray(mqttMsgTimerSeconds, secondsStr.length() + 1);

    // Create leading 0 on game clock if needed
    if(secondsStr.length() == 1){
      mqttMsgTimerSeconds[0]='\0';
      strcat(mqttMsgTimerSeconds, "0");
      strcat(mqttMsgTimerSeconds, secondsStr.c_str());
    }

    if(minutesStr.length() == 1){
      mqttMsgTimerMinutes[0] = '\0';
      strcat(mqttMsgTimerMinutes, "0");
      strcat(mqttMsgTimerMinutes, minutesStr.c_str());
    }

    // Update game time dashboard
    strcat(mqttMsgTimer,mqttMsgTimerMinutes);
    strcat(mqttMsgTimer, ":");
    strcat(mqttMsgTimer, mqttMsgTimerSeconds);
    Serial.println(mqttMsgTimer);
    mqtt.publish(laserTagTopicGameTimer, mqttMsgTimer);
    
    lastPublishedTimer = millis();
  }
}

// Update game timer
void timerUpdate(){
   
  long currentUpdate = 0;
  long lastMSAsSeconds = 0;

      // Check if it's first timer execution   
      if(!isFirstTimer){
       currentUpdate = millis() - lastMS;      
       timerMS = timerMS + currentUpdate;
       lastMS = millis();
       lastMSAsSeconds = timerMS / 1000;
      }
      else{                 
          lastMS = millis();
          timerMS = 0;
          isFirstTimer = false;
     }
  
  
  int secondsTimer = lastMSAsSeconds % 60;
  int minutesTimer = lastMSAsSeconds / 60;
  
  //Write timer to mqtt dashboard
  writeTimerDataToMQTT(secondsTimer, minutesTimer);
}

// Check if player got IR hit from enemy IR value.
bool isMatchValue(uint64_t currentValue){
  return currentValue == HITVALUE;
}

// Create hit effect.
void hitEffect(){ 
  hitSoundEffect();
  hitLedEffect();
}

// Create hit effect sound, update the liveRate var and outOfLife var if needed.
void hitSoundEffect(){
    CircuitPlayground.speaker.say(spHIGH);
    liveRate -= 2;
    if(liveRate == 0){
      OutOfLifeCounter++;
    }
}

// Method make a sound of Ready-Go when the game begining. 
void onSpeaker(){    
    CircuitPlayground.speaker.say(spREADY);  
    CircuitPlayground.speaker.say(spGO);        
}

// Method make a sound of Off when turn off.
void offSpeaker(){  
    CircuitPlayground.speaker.say(spOFF);     
}

// Method make leds effect on hit
void hitLedEffect(){
  
    for (int i = 0; i < NUMOFLEDS; i++)
    {
        delay(200);
        CircuitPlayground.setPixelColor(i, 255,0 , 0);
    }
    
    // Turn off leds
    CircuitPlayground.clearPixels();
    
    // Set live gauge
    liveGaugeLed();
}

// Create live gauge
void liveGaugeLed(){
  
  for (int i = 0; i < liveRate; i=i+2)
    {
        delay(200);
        CircuitPlayground.setPixelColor(i, redLiveColorLed[i/2], greenLiveColorLed[i/2], blueLiveColorLed[i/2]);
        CircuitPlayground.setPixelColor(i+1, redLiveColorLed[i/2], greenLiveColorLed[i/2], blueLiveColorLed[i/2]);
    }  
}

// Check if user turn on the circuit playground express.
bool checkIfOn()
{
    // Check if circuit playground express mode was changed
    if (isBoardOn != CircuitPlayground.slideSwitch())
    {
        isBoardOn = CircuitPlayground.slideSwitch();

        // Execute turn on settings
        if (isBoardOn)
        {
            ledOnOffSwitch(true);
            liveRate = 10;
            onSpeaker();
            liveGaugeLed();
        }
        else
        {
            // Execute turn on settings
            offSpeaker();
            ledOnOffSwitch(false);
            liveRate = 0;                               
        }
    }
}

// The method create the On/Off led effect
void ledOnOffSwitch(bool isOn)
{
    if (!isOn)
    {
        ledRedColor = MAXCOLORVALUE;
        ledGreenColor = MINCOLORVALUE;
    }
    else
    {
        ledRedColor = MINCOLORVALUE;
        ledGreenColor = MAXCOLORVALUE;
        
    }

    for (int i = 0; i < NUMOFLEDS; i++)
    {
        delay(200);
        CircuitPlayground.setPixelColor(i, ledRedColor, ledGreenColor, 0);
    }   

    // Turn of leds
    CircuitPlayground.clearPixels();
    
    if(isOn){
      // Set live gauge
      liveGaugeLed();
    }
}
