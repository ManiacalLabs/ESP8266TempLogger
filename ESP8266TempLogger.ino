//ESP8266TempLogger
//By Dan of ManiacalLabs.com
//Based on this Instructable by yanc:
//http://www.instructables.com/id/Arduino-Wifi-Temperature-Logger/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>

//period between posts, set at 60 seconds
#define DELAY_PERIOD 60000

//pin for the DS18B20 temperature module
#define TEMPERATURE_PIN 4

// Used to hold the ESP8266 in reset until ready to transmit dataa  
#define WIFI_ENABLE_PIN 13

//data.sparkfun.com keys
#define PUBLIC_KEY "YOUR_PUBLIC_KEY_HERE"
#define PRIVATE_KEY "YOUR_PRIVATE_KEY_HERE"

#define DEBUG 0

OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);

// use http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html 
// to find the address of your DS18B20, it will be different then this
DeviceAddress thermometer = {0x28, 0x2A, 0x0E, 0xE8, 0x05, 0x00, 0x00, 0xA5 };

char serialbuffer[100];//serial buffer for request command
long nextTime;//next time in millis that the temperature read will fire

SoftwareSerial mySerial(3, 2); // rx, tx

void setup()
{
    mySerial.begin(9600);//connection to ESP8266
    Serial.begin(9600); //serial debug

    if(DEBUG) {
      while(!Serial);
    }

    pinMode(WIFI_ENABLE_PIN, OUTPUT);
    digitalWrite(WIFI_ENABLE_PIN, HIGH);
    delay(5000);//delay
    
    Serial.println("Begin...");
    
    sensors.begin();
    
    Serial.println("Done Sensors...");
    
    Serial.print("Module Test...");
    mySerial.println("AT\r");
    delay(1000);
    if(mySerial.find("OK")){
      Serial.println("OK");
    }

    nextTime = millis();//reset the timer
}

void loop()
{
    //output everything from ESP8266 to the Arduino Micro Serial output
    while (mySerial.available() > 0) {
      Serial.write(mySerial.read());
    }

    //to send commands to the ESP8266 from serial monitor (ex. check IP addr)
    if (Serial.available() > 0) {
       //read from serial monitor until terminating character
       int len = Serial.readBytesUntil('\n', serialbuffer, sizeof(serialbuffer));

       //trim buffer to length of the actual message
       String message = String(serialbuffer).substring(0,len-1);
       
       Serial.println("message: " + message);

       //check to see if the incoming serial message is an AT command
       if(message.substring(0,2)=="AT"){
         //make command request
         Serial.println("COMMAND REQUEST");
         mySerial.println(message); 
       }//if not AT command, ignore
    }

    //wait for timer to expire
    if(nextTime<millis()){
      Serial.print("timer reset: ");
      Serial.println(nextTime);

      //reset timer
      nextTime = millis() + DELAY_PERIOD;
      
      //get temp
      sensors.requestTemperatures();
      
      //Enable the module and allow it to connect to the network
      digitalWrite(WIFI_ENABLE_PIN, HIGH);
      delay(5000);//delay 
      
      //send temp and delay to allow successful transmit
      SendTempData(sensors.getTempC(thermometer));
      delay(5000);//delay
      
      //hold module in reset to save power until next transmit
      digitalWrite(WIFI_ENABLE_PIN, LOW);
    }
}


//web request needs to be sent without the http for now, https still needs some working
void SendTempData(float temperature){
 char temp[10];

 Serial.print("temp: ");     
 Serial.println(temperature);

 dtostrf(temperature,1,2,temp);

 //create start command
 String startcommand = "AT+CIPSTART=\"TCP\",\"data.sparkfun.com\", 80";
 
 mySerial.println(startcommand);
 Serial.println(startcommand);
 
 //test for a start error
 if(mySerial.find("Error")){
   Serial.println("error on start");
   return;
 }
 
 //create the request command
 String sendcommand = "GET /input/"; 
 sendcommand.concat(PUBLIC_KEY);
 sendcommand.concat("?private_key=");
 sendcommand.concat(PRIVATE_KEY);
 sendcommand.concat("&temp=");
 sendcommand.concat(String(temp));
 sendcommand.concat("\r\n");
 //sendcommand.concat(" HTTP/1.0\r\n\r\n");
 
 Serial.println(sendcommand);
 
 
 //send to ESP8266
 mySerial.print("AT+CIPSEND=");
 mySerial.println(sendcommand.length());
 
 //display command in serial monitor
 Serial.print("AT+CIPSEND=");
 Serial.println(sendcommand.length());
 
 if(mySerial.find(">"))
 {
   Serial.println(">");
 }else
 {
   mySerial.println("AT+CIPCLOSE");
   Serial.println("connect timeout");
   delay(1000);
   return;
 }
 
 //Serial.print("Debug:");
 //Serial.print(sendcommand);
 mySerial.print(sendcommand);
 delay(1000);
 mySerial.println("AT+CIPCLOSE");
}

