/* ESP8266 + MQTT Irrigation Controller
 * Controls 8 zone relay via external MQTT commands
 * Allows for 2 areas, each with 8 zones.  Defined in const as Frontyard and Backyard
 * Only one zone in either area is allowed to be on at once
 * Author: Ricky Hervey
 */

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
 
// Update these with values suitable for your network.
 
const char* ssid = "{SSID}";
const char* password = "{password}"; 
const char* mqtt_server = "{IP_ADRESS}"; // server or URL of MQTT broker
const char* mqtt_serverName = "";
const char* mqtt_serverPass = "";
const char* espName = "House Irrigation Controller";

String clientName = "Irrigation-"; // just a name used to talk to MQTT broker
String macName;
char* zoneTopic = "irrigation/zone/active";
char* statusTopic = "irrigation/zone/+/status/";
char message_buff[100];

int relayPin[] ={16, 5, 4, 0, 2, 14};
int zonesAmount = sizeof(relayPin)/sizeof(relayPin[0]);

const char*  publishTopic; 
unsigned long statusPublishPeriod = 60000; //ms, set to 1 minute for testing
unsigned long statusLastTransmit = 0;
unsigned long statusLastZoneActivation = 0;

unsigned long maxinterval = 1200000; //(ms) - 30 minutes max sprinkler on time
unsigned long resetPeriod = 86400000; // 1 day - this is the period after which we restart the CPU

WiFiClient wifiClient;
PubSubClient clientMQTT(wifiClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//Setup WiFi Connection
void setup_wifi() {
 
  WiFiManager wifiManager;
  wifiManager.setTimeout(30);
  
  if(!wifiManager.autoConnect("AutoConnectAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 
  
  Serial.println("Connected.");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//End of setup Wifi Connection


//Turn Off all zones function
void turnOffAllZones() {
  Serial.println("Turn off all zones -> ");
  for (int i = 0; i < zonesAmount; ++i) {
    Serial.print("    Turn off all zones:");
    Serial.println(i+1);
    pinMode(relayPin[i], OUTPUT);
    digitalWrite(relayPin[i], HIGH);
    statusLastZoneActivation = 0;
    }
}

void turnOnZone(int p) {
  delay(500);
  char charValue = p+'0';
  Serial.printf("Turn on zone:%c", charValue);
  digitalWrite(p, LOW); 
}

//Read mqqt topic payload
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
 
  // Switch on the LED if an 1 was received as first character
  if (String(topic) == String(zoneTopic)) {
    if ((char)payload[0] == '1') {
      turnOffAllZones();
      turnOnZone(16);
      statusLastZoneActivation = millis();
    } 
    else if((char)payload[0] == '2') {
      turnOffAllZones();
      turnOnZone(5);
      statusLastZoneActivation = millis();
    } 
    else if((char)payload[0] == '3') {
      turnOffAllZones();
      turnOnZone(4);
      statusLastZoneActivation = millis();
    } 
    else if((char)payload[0] == '4') {
      turnOffAllZones();
      turnOnZone(0);
      statusLastZoneActivation = millis();
    } 
    else if((char)payload[0] == '5') {
      turnOffAllZones();
      turnOnZone(2);
      statusLastZoneActivation = millis();
    }
    else if((char)payload[0] == '6') {
      turnOffAllZones();
      turnOnZone(14);
      statusLastZoneActivation = millis();
    }
    else {
      turnOffAllZones();
    }
  }
}

//Reconnect if deisconnected
  void reconnect() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    clientName += String(random(0xffff), HEX);
    // Attempt to connect
    if (clientMQTT.connect(clientName.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      clientMQTT.publish("outTopic", "hello world");
      // ... and resubscribe
      clientMQTT.subscribe(zoneTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} 

//Setup finction
void setup() {
  pinMode(16, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  
  turnOffAllZones();
  
  Serial.begin(115200);
  setup_wifi();
  clientMQTT.setServer(mqtt_server, 1883);
  clientMQTT.setCallback(callback);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}

//Main loop 
void loop() {
   ArduinoOTA.handle();
  if (!clientMQTT.connected()) {
    reconnect();
  }
  clientMQTT.loop();

  // reset after a day to avoid memory leaks 
  if(millis()>resetPeriod){
    Serial.println("Reastarting after 24hours uptime...");
    ESP.restart();
  }

  //Turn off all zones after max runtime interval exceed
  long runTime = millis() - statusLastZoneActivation;
  if (runTime > maxinterval && statusLastZoneActivation != 0) {
    Serial.println("Runtime limit exceed, turn off all zones");
    //clientMQTT.publish("outTopic", 0);
    turnOffAllZones();
  }
  
  long now = millis();

  if (now - lastMsg > statusPublishPeriod) {
    lastMsg = now;
    ++value;
    snprintf (msg, 50, "hello world #%ld", value);
    Serial.println("Alive");
    clientMQTT.publish("irrigation/controller/status", "alive");
  }
}
