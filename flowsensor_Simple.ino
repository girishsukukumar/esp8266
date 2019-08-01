#define TCP_MSS whatever
#define LWIP_IPV6 whatever
#define LWIP_FEATURES whatever
#define LWIP_OPEN_SRC whatevers 

#include <Arduino.h> 
#include <EEPROM.h> 
#define USE_SERIAL Serial 
#include <ESP8266WiFi.h> 
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid     = "XXXXX";
const char* password  = "yyyyyyyy";


const char* host      = "mytaxonomylife.com" ;
const int   inputPIN  = 4 ; //GPIO 4
int   ticks  = 0 ;
int   previousTicksNoted = -1 ; 
int   totalLiters = 0 ;
int   liter_as_String[10];
/*
 * -------------------------------------------------------------------------
 * Interrupt Handler for the inputPin, when ever the logic state of this pin
 * goes high this ISR get called 
 * -------------------------------------------------------------------------
 */ 
 
void ICACHE_RAM_ATTR pin_ISR()  
 {        
  ticks++;  
 }
 /* 
  * ------------------------------------------------------------
  * Routine to covert JSON is Ascii format to URL encoded format 
  * ------------------------------------------------------------
  */
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c))
      {
        encodedString+=c;
      } 
      else
      {
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';

        }

        c=(c>>4)&0xf;

        code0=c+'0';

        if (c > 9){

            code0=c - 10 + 'A';

        }

        code2='\0';

        encodedString+='%';

        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;

      }

      yield();

    }

    return encodedString;

    

}

void setup() 
{
  Serial.begin(115200);
  delay(10);
  Serial.print("Starting");
  pinMode(inputPIN, INPUT);

  attachInterrupt(digitalPinToInterrupt(inputPIN), pin_ISR, RISING);
  Serial.print("ISR installed");
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int value = 0;
void PostWithJSON(WiFiClient wifiClient,String URL, float volume)
{
  byte body[200];
  char json[200];
  int len;
  JsonObject tags;
  JsonObject fields;
  StaticJsonDocument<200> jsonDoc;
  String jsonString;

  jsonDoc["measurement"].set("WaterMeasurement");
  tags = jsonDoc.createNestedObject("tags");
  tags["event"].set("periodic");
  tags["SensorId"].set("water-sensor-0001");
  fields = jsonDoc.createNestedObject("fields");  
  fields["Volume"].set(volume);
  //serializeJson(jsonDoc, Serial);
  len = serializeJson(jsonDoc, json);
  jsonString=urlencode(json) ;
  len = jsonString.length();
  //Serial.println(jsonString);
  //Serial.println(body);
  //Serial.print("Requesting URL: ");
  //Serial.println(URL);

  // This will send the request to the server
  wifiClient.print(String("POST ") + URL + " HTTP/1.1\r\n");
  wifiClient.print(String("Host: ") + host + " \r\n" );
  wifiClient.print("Accept: */* \r\n");
  wifiClient.print("User-Agent: PostmanRuntime/7.15.2\r\n");
  wifiClient.print("Cache-Control: no-cache\r\n");
  wifiClient.print(String("Content-Type: application/x-www-form-urlencoded")+"\r\n" );
  //wifiClient.print(String("Accept-Encoding: gzip, deflate\r\n"));
  len = len + strlen("Action=Event&data=");
  wifiClient.print("Content-length: " + String(len) + "\r\n");
  wifiClient.print("Connection: keep-alive\r\n") ;
  wifiClient.print("Cache-Control: no-cache\r\n\r\n");
  wifiClient.print("Action=Event&data=");
  wifiClient.print(jsonString);

  Serial.println(volume);
  unsigned long timeout = millis();
  Serial.println(">>> POST -Sent content length");
  while (wifiClient.available() == 0) 
  {
    if (millis() - timeout > 5000) 
    {
      Serial.println(">>> Client Timeout !");
      wifiClient.stop();
      return;
    }
  }
 
}

void SimplePost(WiFiClient wifiClient,String URL)
{
  //Serial.print("Requesting URL: ");
  //Serial.println(URL);
  
  // This will send the request to the server
  wifiClient.print(String("POST ") + URL + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n");
  wifiClient.print("Connection: keep-alive\r\n\r\n") ;
 
  unsigned long timeout = millis();
  while (wifiClient.available() == 0) 
  {
    if (millis() - timeout > 5000) 
    {
      Serial.println(">>> Client Timeout !");
      wifiClient.stop();
      return;
    }
  }
}

void GetStatusAfterPost(WiFiClient wifiClient)
{
  String line ;
  // Read all the lines of the reply from server and print them to Serial
  while(wifiClient.available())
  {
    line = wifiClient.readStringUntil('\r');
    Serial.print(line);
  }

}
float ConvertTicksToLiters(int Ticks)
{
  // 440 ticks = 1 liter
  // One tick means 2.272 ml 
  float liters ;
  liters = Ticks * 2.272 ;
  return liters ;
}
void loop() 
{
  byte message[100] ;
  String url ;
  WiFiClient client;
  const int httpPort = 80;
  float liters;
  
  //delay(5000); //Wait between two successive calls 
  //++value;

  //Serial.print("connecting to ");
  //Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  Serial.println(ticks);
  Serial.println(previousTicksNoted);
  if (ticks  > 2000)
  {
     if (!client.connect(host, httpPort)) 
     {
       Serial.println("connection failed");
       return;
     }
     liters = ConvertTicksToLiters(ticks);
     ticks = 0 ;
     // We now create a URI for the request
     url = "/iot/speed";

#if 0
     Serial.println(">>> First post !");
     SimplePost(client,url); 
     Serial.println("Reading status");
     GetStatusAfterPost(client);  
     if (!client.connect(host, httpPort)) 
     {
        Serial.println("connection failed");
        return;
     }
     Serial.println("");
     Serial.println("");
     Serial.println("");
#endif  

     Serial.println(">>> Second post !");
     url = "/iot/event";
     PostWithJSON(client,url,liters);
     Serial.println("Reading Status");
     GetStatusAfterPost(client);  
  
     Serial.println();
     Serial.println("closing connection");
  }
}
