#define DHTPIN 6  
#include <SoftwareSerial.h>
#include <stdlib.h>
#define DHTTYPE DHT11   
#include "DHT.h"
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <SFE_BMP180.h>

#define RAIN_GAUGE_PIN 2
#define RAIN_GAUGE_INT 0
#define RAIN_FACTOR 0.2794
#include <math.h> 
int VaneValue;
int Direction;// translated 0 - 360 direction 
int CalDirection;// converted value with offset applied 
int LastValue; 
char n;
#define WindSensorPin 3//red wire on 3 and yellow on ground 

#define Offset 0; 
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter(0x5C);
SFE_BMP180 pressure;
volatile unsigned long Rotations; 
volatile unsigned long ContactBounceTime; 
float WindSpeed;


#define ALTITUDE 1655.0 

// replace with your channel's thingspeak API key
String apiKey = "KJU8XOPI6HVS9LMV";

// connect 10 to TX of Serial USB
// connect 11 to RX of serial USB
SoftwareSerial ser(10, 11); // RX, TX

// this runs once
void setup() {     
  LastValue = 1; 
             
   pinMode(RAIN_GAUGE_PIN,INPUT);
  digitalWrite(RAIN_GAUGE_PIN,HIGH);  // Turn on the internal Pull Up Resistor
  
  attachInterrupt(RAIN_GAUGE_INT,rainGageClick,FALLING);
  interrupts();

  pinMode(WindSensorPin, INPUT); 
attachInterrupt(digitalPinToInterrupt(WindSensorPin), rotation, FALLING);

  Serial.begin(9600); 
  // enable software serial
   lightMeter.begin(BH1750_CONTINUOUS_HIGH_RES_MODE);
 

  if (pressure.begin())
    Serial.println("BMP180 init success");
  else
  {
    // Oops, something went wrong, this is usually a connection problem,
    // see the comments at the top of this sketch for the proper connections.

    Serial.println("BMP180 init fail\n\n");
    while(1); // Pause forever.
  }
  ser.begin(9600);
  
  // reset ESP8266
  ser.println("AT+RST");
   dht.begin();
}

volatile unsigned long rain_count=0;
volatile unsigned long rain_last=0;
 
double getUnitRain()
{
 
  unsigned long reading=rain_count;
  rain_count=0;
  double unit_rain=reading*RAIN_FACTOR;
   
 
  return unit_rain;
}
 
void rainGageClick()
{
    long thisTime=micros()-rain_last;
    rain_last=micros();
    if(thisTime>500)
    {
      rain_count++;
    }
    
}
void rotation () { 

if ((millis() - ContactBounceTime) > 15 ) {  
Rotations++; 
ContactBounceTime = millis(); 
} 
}


// Converts compass direction to heading 
void getHeading(int direction) { 
if(direction < 22) 
Serial.println("N"); 
else if (direction < 67) 
Serial.println("NE"); 
else if (direction < 112) 
Serial.println("E"); 
else if (direction < 157) 
Serial.println("SE"); 
else if (direction < 212) 
Serial.println("S"); 
else if (direction < 247) 
Serial.println("SW"); 
else if (direction < 292) 
Serial.println("W"); 
else if (direction < 337) 
Serial.println("NW"); 
else 
Serial.println("N"); 
}

// the loop 
void loop() {
  uint16_t lux = lightMeter.readLightLevel();
   int sensorValue = analogRead(A0);

   double rain;
  rain = getUnitRain();
  
  
   float t = dht.readTemperature();
float h = dht.readHumidity();

 char status;
  double T,P,p0,a,z;

  
  // convert to string
  char buf[16];
  String strTemp = dtostrf(t, 4, 1, buf);
  String strHum = dtostrf(h, 4, 1, buf);
  String strRain = dtostrf(rain, 4, 1, buf);
  Serial.println(strHum);
  Serial.println(strTemp);
  Serial.println(strRain);
  Serial.println(sensorValue);
   Serial.println();
  Serial.print("provided altitude: ");
  Serial.print(ALTITUDE,0);
  Serial.println(" meters, ");

   Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
 

  status = pressure.startTemperature();
  if (status != 0)
  {
    
    delay(status);

    
    status = pressure.getTemperature(T);
    if (status != 0)
    {
      
      Serial.print("temperature: ");
      Serial.print(T,2);
      Serial.println(" deg C, ");
     
      status = pressure.startPressure(3);
      if (status != 0)
      {
        
        delay(status);

       

        status = pressure.getPressure(P,T);
        if (status != 0)
        {
          
          Serial.print("absolute pressure: ");
         
          Serial.print(P*0.0295333727,2);
          Serial.println(" inHg");
          z=P*0.0295333727;

         

          p0 = pressure.sealevel(P,ALTITUDE); // we're at 1655 meters (Boulder, CO)
          Serial.print("relative (sea-level) pressure: ");
        
          Serial.print(p0*0.0295333727,2);
          Serial.println(" inHg");

        
        }
        else Serial.println("error retrieving pressure measurement\n");
      }
      else Serial.println("error starting pressure measurement\n");
    }
    else Serial.println("error retrieving temperature measurement\n");
  }
  else Serial.println("error starting temperature measurement\n");
  
  
  // TCP connection
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "139.59.61.213"; // api.thingspeak.com
  cmd += "\",4040";
  ser.println(cmd);
   
  if(ser.find("Error")){
    Serial.println("AT+CIPSTART error");
    return;
  }
   String getStr = "";
  getStr += String(strTemp);
  getStr +=",";
  getStr += String(strHum);
    getStr +=",";
  getStr += String(rain);
    getStr +=",";
  getStr += String(sensorValue);
    getStr +=",";
  getStr += String(ALTITUDE);
    getStr +=",";
  getStr += String(lux);
    getStr +=",";
  getStr += String(z);
    getStr +=",";
   getStr +="5c:cf:7f:f1:6f:49";
   
   getStr += "\r\n\r\n";
 
  // send data length
  cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  ser.println(cmd);

  if(ser.find(">")){
    ser.print(getStr);
  }
  else{
    ser.println("AT+CIPCLOSE");
    // alert user
    Serial.println("AT+CIPCLOSE");
  }
  
  
 

  
    
 
  delay(16000);  
}
