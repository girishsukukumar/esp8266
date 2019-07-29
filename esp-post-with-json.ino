
#define TCP_MSS whatever
#define LWIP_IPV6 whatever
#define LWIP_FEATURES whatever
#define LWIP_OPEN_SRC whatevers
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
//#include <ArduinoJson.h>
#include <ArduinoJson.h>



const char* ssid     = "XXXXXXXXXXXXXX";
const char* password = "YYYYYYYYYYYYYY";

const char* host = "myplatform.com";

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
void PostWithJSON(WiFiClient wifiClient,String URL)
{
  byte body[200];
  char json[200];
  int len;
  JsonObject tags;
  JsonObject fields;
  StaticJsonDocument<200> jsonDoc;
  String jsonString;

  jsonDoc["measurement"].set("ClimateEvents");
  tags = jsonDoc.createNestedObject("tags");
  tags["event"].set("periodic");
  tags["SensorId"].set("00001");
  fields = jsonDoc.createNestedObject("fields");  
  fields["Temperature"].set(25.1);
  fields["Humidity"].set(60.3);
  serializeJson(jsonDoc, Serial);
  len = serializeJson(jsonDoc, json);
  jsonString=urlencode(json) ;
  len = jsonString.length();
  Serial.println(jsonString);
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
void loop() 
{
  byte message[100] ;
  String url ;
  WiFiClient client;
  const int httpPort = 80;

  
  delay(5000); //Wait between two successive calls 
  ++value;

  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  
  if (!client.connect(host, httpPort)) 
  {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  
  url = "/iot/speed";

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
  
  Serial.println(">>> Second post !");
  url = "/iot/event";
  PostWithJSON(client,url);
  Serial.println("Reading Status");
  GetStatusAfterPost(client);  
  
  Serial.println();
  Serial.println("closing connection");
}
