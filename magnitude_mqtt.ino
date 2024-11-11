#include <ESP8266WiFi.h>
#include <MPU9250_WE.h>
#include <Wire.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

// Define MPU9250 I2C address
#define MPU9250_ADDR 0x68

// Adafruit IO MQTT Server details
#define AIO_SERVER "io.adafruit.com"
#define AIO_SERVERPORT 1883

#define AIO_USERNAME "REI_owo"
#define AIO_KEY "aio_iQLK515MNSiHMdXNIGPcDlw6zzjR"

// Wi-Fi credentials
const char* ssid="Chan_2.4G";
const char* password="0125717025";

// Create MPU9250 object
MPU9250_WE myMPU9250 = MPU9250_WE(MPU9250_ADDR);
WiFiClient client;

// Create Adafruit MQTT client
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Create MQTT Publish instances for different magnitudes
Adafruit_MQTT_Publish graph_high = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/magnitude.strong");
Adafruit_MQTT_Publish graph_medium = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/magnitude.moderate");
Adafruit_MQTT_Publish graph_low = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/magnitude.light");

// Constants for calculation
float a0 = 0.00583; // Fixed A0 in meter that measured at the epicenter
float a; // a that will measured using the accelerator at current location
float delta = 0.03; // The distance between the epicenter and current location (8cm converted to km) 
float magnitude; // The Richter's magnitude that will be calculated using the variables above
long BR = 115200;

// Function prototype for MQTT connection
void MQTT_connect();

void setup() {
	Serial.begin(BR);
	while(!Serial){}
	
	Wire.begin();
  // Initialize MPU9250
	if(!myMPU9250.init()){
		Serial.println("MPU9250 does not respond");
	}
	else{
    Serial.println("MPU9250 is connected");
	}
	
	// Calibrate MPU9250 (xMin,xMax,yMin,yMax,zMin,zMax)
	myMPU9250.setAccOffsets(-14240.0, 18220.0, -17280.0, 15590.0, -20930.0, 12080.0);

  // Connect to Wi-Fi
	WiFi.begin(ssid, password);
	while(WiFi.status() != WL_CONNECTED) {
		delay (500);
		Serial.print(".");
	}

  // Connect to MQTT
  MQTT_connect();
}

void loop() {
  // Get the amplitude
	xyzFloat accRaw = myMPU9250.getAccRawValues();
	// Use absolute function to retrieve positive value
	a = abs(accRaw.z);
	// Use Richter's magnitude formula to calculate the magnitude, a is converted from micrometer to meter
	magnitude = log10(a*pow(10,-6)) - (log10(a0*delta));

  Serial.print("\nSending magnitude: ");
  Serial.println(magnitude);

  delay(200);

  // Store magnitude into array so that to handle the adafruit could not handle the data so quickly
  char richterString[10];
  dtostrf(magnitude, 4, 2, richterString);

//send magnitude value to the feeds accordingly
if(magnitude>=3){
  if(!graph_high.publish(richterString)){
    Serial.println("Send failed.");
  }
  else{
    Serial.println("Strong Sent");
  }
}
else if(magnitude>=2){
  if(!graph_medium.publish(richterString)){
    Serial.println("Send failed.");
  }
    else{
    Serial.println("Moderate Sent");
  }
}
else{
  if(!graph_low.publish(richterString)){
    Serial.println("Send failed.");
  }
    else{
    Serial.println("Light Sent");
    
  }
}

if (magnitude >= 10) {
    magnitude = 0; // Reset the value.
  }

//if not delay the data will go too fast until client cannot handle
delay(5000);

if (!mqtt.ping()) mqtt.disconnect();

}
void MQTT_connect(){
  int8_t ret;

// Stop if already connected.
if(mqtt.connected())
return;

Serial.print("Connecting to MQTT... ");

uint8_t retries = 3;
while((ret = mqtt.connect()) != 0){ // connect will return 0 for connected
Serial.println(mqtt.connectErrorString(ret));

mqtt.disconnect();
delay(5000); // wait 5 seconds
retries--;
if(retries == 0)
while (1); // basically die and wait for WDT to reset me
}
Serial.println("MQTT Connected!");
}
