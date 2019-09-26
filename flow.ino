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
#include  <ESP8266WiFiMulti.h> 
#include  <ESP8266WebServer.h>   
#include <NTPClient.h>
#include <WiFiUdp.h>


#include  <FS.h>      //  Include the SPIFFS  library 
char ssid1[20]      = "xxxxxxxxxxxxxxxxxxx" ;
char password1[20]  = "xxxxxxxxxxxxxxxxxxx" ;
char ssid2[20]      = "xxxxxxxxxxxxxxxxxxx";
char password2[20]  = "xxxxxxxxxxxxxxxxxxx";
char host[40]  ;
int   hostPort  = 0;
WiFiUDP ntpUDP;
const int   inputPIN  = 4 ; //GPIO 4
int   ticks  = 0 ;
int   previousTicksNoted = -1 ; 
float   totalLiters = 0 ;
int   liter_as_String[10];
unsigned long   timeOfLastPost = 0; 
unsigned long diff ;
unsigned long secondsCounter ;
int value = 0;

ESP8266WiFiMulti  wifiMulti;          //  Create  an  instance  of  the ESP8266WiFiMulti 
ESP8266WebServer  server(80);
NTPClient timeClient(ntpUDP);

typedef struct configData 
{
  String   ssid1;
  String   password1;
  String   ssid2;
  String   password2;
  String   servername ;
  int      portNo;
  String   apiKey;
  String   wifiDeviceName ;  
  
};

struct configData ConfigData ;

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
    for (int i =0; i < str.length(); i++)
    {
      c=str.charAt(i);
      if (c == ' ')
      {
        encodedString+= '+';
      } 
      else if (isalnum(c))
      {
        encodedString+=c;
      } 
      else
      {
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9)
        {
            code1=(c & 0xf) - 10 + 'A';
        }

        c=(c>>4)&0xf;

        code0=c+'0';

        if (c > 9)
        {
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
void DisplayForm()
{
   File  file ;
   size_t  sent;
 
      if (SPIFFS.exists("/form.html"))  
      { 
        file =SPIFFS.open("/form.html",  "r");
        sent =server.streamFile(file, "text/html");  
        file.close();
      }
 
}

void ReadVolumeFromSPIFFS(char *v, char *d, char *t)
{
  String volume;
  String rdate;
  String rtime ; 
  File   file;
  file =SPIFFS.open("/totalvolume.dat",  "r");
  volume = file.readStringUntil('\n');     
  Serial.println("Volume read from File");
  Serial.println(volume);
 // rdate  = file.readStringUntil('\n');     
 // rtime  = file.readStringUntil('\n');     
 
  file.close();
  
}

void DisplayConfig()
{
   char html[2000];
   char devicename[20];
   char apikey[16] ;
   char volume[10];
   char rtime[10];
   char rdate[10] ;
   char lHost[40];
   ReadVolumeFromSPIFFS(volume, rtime, rtime);
   
   ConfigData.wifiDeviceName.toCharArray(devicename, 20);
   ConfigData.servername.toCharArray(lHost, 40);
   ConfigData.apiKey.toCharArray(apikey, 16);   
   sprintf(html, "<HTML>, <H1> Device Configuration <\/H1> <body style=\"background-color:powderblue;\"> <TABLE border=\"1\">\
                  <TR> <TD> SSID1:</TD> <TD> %s </TD> <\/TR>\
                  <TR> <TD> SSID2: </TD> <TD> %s </TD> <\/TR>\
                  <TR> <TD> Platform Server: </TD> <TD> %s </TD> <\/TR>\
                  <TR> <TD> Port No: </TD> <TD> %d </TD> <\/TR>\
                  <TR> <TD> Device Key: </TD> <TD> %s </TD> <\/TR>\
                  <TR> <TD> Device Name: </TD> <TD> %s </TD> <\/TR> \
                  <TR> <TD> Connected Wifi: </TD> <TD> %s </TD> </TR>\
                  <TR> <TD> Device IP address: </TD> <TD> %s </TD> </TR> \
                  <TR> <TD> Volume Consumed today: </TD> <TD> %f </TD> </TR> </TABLE>\
                  <form  action=\"\/displayForm\" method=\"POST\">\
                   <button type=\"submit\">  Edit Config  <\/button></form> </body> <\/HTML>",
                   ssid1,
                   ssid2,
                   lHost,
                   ConfigData.portNo,
                   apikey,
                   devicename,
                   WiFi.SSID().c_str(),
                   WiFi.localIP().toString().c_str(),
                   totalLiters);
                
    server.send(404,  "text/html", html);                    
}
void  handleRoot()  
{
    File file;
    size_t sent;
    if (SPIFFS.exists("/index.html"))  
    {
       file =SPIFFS.open("/index.html",  "r");
       sent =server.streamFile(file, "text/html");  
       file.close();
    }
}

void  handleNotFound()
{   
  server.send(404,  "text/html", "<HTML> <H1>404: Handler Not found </H1></HTML>");    
}

 
void handleLogin()
{
  File  file ;
  size_t  sent;
    if( ! server.hasArg("username") ||  
        ! server.hasArg("password") ||  
          server.arg("username")  ==  NULL  ||  
          server.arg("password")  ==  NULL)
    { 
      //  If  the POST  request doesn't have  username  and password  data        
        server.send(400,  "text/html", "<HTML> <H1> 400: Invalid Request</H1></HTML>");                 
      //  The request is  invalid,  so  send  HTTP  status  400      
      return;   
    }   
    if(server.arg("username") ==  "root"  &&  server.arg("password")  ==  "password123")  
    { 
      //  If  both  the username  and the password  are correct       
      // server.send(200,  "text/html",  "<h1>Welcome, " + server.arg("username")  + "!</h1><p>Login successful</p>");   
      DisplayConfig();
  
    } 
    else  
    {                                                                                                                                    
       //  Username  and password  don't match       
       server.send(401,  "text/html", "<HTML> <H1> 401: Unauthorized </H1> </HTML>");   
    } 
    
}
void WriteConfigValuesToSPIFFS()
{
  File file;
  file = SPIFFS.open("/config.dat", "w");                                     
  file.println(ConfigData.ssid1);
  file.println(ConfigData.password1);
  file.println(ConfigData.ssid2);
  file.println(ConfigData.password2);
  file.println(ConfigData.servername);     
  file.println(ConfigData.portNo);
  file.println(ConfigData.apiKey);
  file.println(ConfigData.wifiDeviceName);
  file.close();
}
void WriteVolumeToSPIFFS()
{
  File file;
  int splitT ;
  String formattedDate;
  String dayStamp;
  timeClient.update();
  Serial.println("NTP TIME");
  formattedDate = timeClient.getFormattedDate(); 
  splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);

  Serial.println(timeClient.getFormattedTime());
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  file = SPIFFS.open("/totalvolume.dat", "w");
  file.println(totalLiters);
  file.println(dayStamp);
  file.println(timeClient.getFormattedTime());
  file.close();
}
void handleSubmit()
{
  
   Serial.println("handleSubmit:Collecting Data from parameters");
   
   if (server.hasArg("ssid1") && server.hasArg("password1") &&
      (server.hasArg("ssid1") != NULL) &&
      (server.hasArg("password1")!= NULL))
   {
     //Update SSID 1 and password1
     ConfigData.ssid1 = server.arg("ssid1") ;
     ConfigData.password1 = server.arg("password1") ;
     
   }
   if (server.hasArg("ssid2") && server.hasArg("password2") &&
      (server.hasArg("ssid2") != NULL) &&
      (server.hasArg("password2")!= NULL))
   
   {
     ConfigData.ssid2 = server.arg("ssid2") ;
     ConfigData.password2 = server.arg("password2") ;
 
   }
   if ((server.hasArg("servername")) && (server.arg("servername") != NULL))
   {
     ConfigData.servername = server.arg("servername") ;
   }
   if ((server.hasArg("serverPort")) && (server.arg("serverPort") != NULL))
   {
       String val ;
       val = server.arg("serverPort");
       ConfigData.portNo = val.toInt();
   }
   if ((server.hasArg("apikey"))&& (server.arg("apikey") != NULL))
   {
       ConfigData.apiKey = server.arg("apikey") ;      
   }
   if ((server.hasArg("wifihostname")) && (server.arg("wifihostname") != NULL))
   {
       ConfigData.wifiDeviceName = server.arg("wifihostname") ;
   }
   
   PrintConfigValues();

   WriteConfigValuesToSPIFFS();
   
   server.send(401,  "text/html", "<HTML> <H1>  Values will be updated After next reboot </H1> </HTML>");
}

void PrintConfigValues()
{
      Serial.println("---------Print Values--------------------------");
      Serial.println(ConfigData.ssid1);
      Serial.println(ConfigData.password1);
      Serial.println(ConfigData.ssid2);
      Serial.println(ConfigData.password2);
      Serial.println(ConfigData.servername);     
      Serial.println(ConfigData.portNo);
      Serial.println(ConfigData.apiKey);
      Serial.println(ConfigData.wifiDeviceName);
      Serial.println("--------------End---------------------");
}
void ReadConfigValuesFromSPIFFS()
{
  int count ;
  SPIFFS.begin();  
  Serial.println("SPIFFS begin"); 
  if  (SPIFFS.exists("/config.dat")) 
  {
      File  file ;
      String str ;
      char buf[30];
      Serial.println("Reading /config.dat ....");
      file = SPIFFS.open("/config.dat", "r");                                     
      Serial.println("config.opened");
      count = 1 ;
      while(file.available() && (count < 10))
      {
        str = file.readStringUntil('\n');     
        Serial.println(str);
        switch(count)
        {
           case 1 : {  ConfigData.ssid1 = str ; break ;}
           case 2 : {  ConfigData.password1 = str  ; break ;}
           case 3 : {  ConfigData.ssid2  = str ; break ;}
           case 4 : {  ConfigData.password2 = str  ; break ;}
           case 5 : { 
                       int len,i; // Only for debugging 
                       ConfigData.servername = str ; 
                       len = ConfigData.servername.length();
                       ConfigData.servername.toCharArray(host, len);                       
                       break ;
                    } 
           case 6 : {  ConfigData.portNo   = str.toInt() ; break ;}
           case 7 : {  ConfigData.apiKey  = str ;  break ;}
           case 8 : {  ConfigData.wifiDeviceName = str ; break ;}
           default : { break; }
        }
        count++;
      }
      file.close();  
      Serial.println("Finished Reading SPIFFS !!!");
      PrintConfigValues();
  }
  else
  {
    // Populate with default values
  }  
}

void setup() 
{

  int          ServerMode ;
  String       str;
  unsigned int len;

  Serial.begin(115200);
  delay(1000);
  Serial.print("Starting");
  pinMode(inputPIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(inputPIN), pin_ISR, RISING);
  Serial.print("ISR installed");
  // We start by connecting to a WiFi network
  delay(6000);
  ReadConfigValuesFromSPIFFS();
  
  WiFi.hostname(ConfigData.wifiDeviceName); 
  //Read Server Mode from GPIO status for now this is hard coded
  ServerMode = 1 ; // Server mode is set to TRUE
  
  
  
  len = ConfigData.ssid1.length();
  
  ConfigData.ssid1.toCharArray(ssid1,len ); 

  len = ConfigData.password1.length();
  ConfigData.password1.toCharArray(password1,len ); 


  len = ConfigData.ssid2.length();
  ConfigData.ssid2.toCharArray(ssid2,len ); 

  len = ConfigData.password2.length();
  ConfigData.password2.toCharArray(password2,len ); 
 
  Serial.println(ssid1);
  Serial.println(password1);
  
  Serial.println(ssid2);
  Serial.println(password2);
  
  wifiMulti.addAP(ssid1, password1);   
  wifiMulti.addAP(ssid2, password2);    
  
  while  (wifiMulti.run()  !=  WL_CONNECTED) 
  { 
    //  Wait  for the Wi-Fi to  connect:  scan  for Wi-Fi networks, and connect to  the strongest of  the networks  above       
    delay(1000);        
    Serial.print('*');    
  }   
  delay(5000);
  Serial.println('\n');   
  Serial.print("Connected to  ");   
  Serial.println(WiFi.SSID());         
  Serial.print("IP  address:\t");   
  Serial.println(WiFi.localIP()); 
  WiFi.softAPdisconnect (true);   //Disable the Access point mode.
 
  if (ServerMode == 1)
  {
      // Install the handlers the HTTP Server (Equivalent to flask)
      server.on("/",  HTTP_GET,   handleRoot);    //  Call  the 'handleRoot'
      server.on("/login",  HTTP_POST,  handleLogin);
      server.on("/showconf", HTTP_POST, DisplayConfig);
      server.on("/conf",  handleSubmit);
      server.on("/displayForm",DisplayForm);
      server.onNotFound(handleNotFound); // For error handling unknown operation
      server.begin(); // Start the server
      Serial.println("HTTP  server  started");
  }
  
  timeClient.begin();
  timeClient.setTimeOffset(19800); /* GMT + 5:30 hours */
}


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
 
  Serial.println("PostWithJSON");
  Serial.println(host);
  // This will send the request to the server
  wifiClient.print(String("POST ") + URL + " HTTP/1.1\r\n");
  wifiClient.print(String("Host: ") + host + " \r\n" );
  wifiClient.print("Accept: */* \r\n");
  wifiClient.print("User-Agent: PostmanRuntime/7.15.2\r\n");
  wifiClient.print("Cache-Control: no-cache\r\n");
  wifiClient.print(String("Content-Type: application/x-www-form-urlencoded")+"\r\n" );
  //wifiClient.print(String("Accept-Encoding: gzip, deflate\r\n"));
  len = len + strlen("Action=Event&APIKEY=a197bfcc29cd&data=");
  wifiClient.print("Content-length: " + String(len) + "\r\n");
  wifiClient.print("Connection: keep-alive\r\n") ;
  wifiClient.print("Cache-Control: no-cache\r\n\r\n");
  wifiClient.print("Action=Event&APIKEY=a197bfcc29cd&data=");
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
  // One tick means 2.272 milli liters 
  float liters =0.0;
  if (Ticks == 0)
      return liters;
  // Volume in milli liters - Ticks X 2.272
  liters = (Ticks * 2.272)/1000 ;
  return liters ;
}

/*
void updateNTPTime()
{
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();    // Get Unix epoch time from the NTP server
  TimeString = timeClient.getFormattedTime() ;
}
*/
void loop() 
{
  byte message[100] ;
  String url ;
  WiFiClient client;
  
  float liters;
  unsigned long currentTimeinMillis ;


  if (ticks != 0)
  {
    if ((ticks % 250) == 0)
      Serial.println(ticks);
  }
  if (ticks  > 2000)
  {
      
      
     if (!client.connect(host, ConfigData.portNo)) 
     {
       Serial.println("regular: connection failed");
       return;
     }
     
     // We now create a URI for the request
     url = "/iot/speed"; 

     Serial.println(">>> Second post !");
     
     liters = ConvertTicksToLiters(ticks);
     ticks = 0 ;
     url = "/iot/event";
     timeOfLastPost = millis();
     PostWithJSON(client,url,liters);
     totalLiters = totalLiters+ liters ;
     WriteVolumeToSPIFFS();
     Serial.println("Reading Status");
     GetStatusAfterPost(client);  
  
     Serial.println();
     Serial.println("closing connection");
  }
  else
  {
      // This condition is to avoid no data for a long time/
      // We make post oof ZERO liters or 0.0001 liters every
      // 5 minutes as a keep alive message
      unsigned long now;
      now = millis() ;
      diff =  now - timeOfLastPost ;
      //Serial.print("diff: ");
      //Serial.print(diff);
      if (( diff / 1000) > 300)
      {
        Serial.println("Keep alive");
        
        if (!client.connect(host , ConfigData.portNo)) 
        {
          Serial.println("keep alive :connection failed");
          return;
        }
        liters = ConvertTicksToLiters(ticks);
        if (liters == 0.0)
           liters = 0.0001;
        ticks = 0 ;
        url = "/iot/event";
        timeOfLastPost = millis();
        PostWithJSON(client,url,liters);
        Serial.println("Reading Status");
        GetStatusAfterPost(client);  
        Serial.println();
        Serial.println("closing connection");
        secondsCounter = 0 ;
      }
  }
  server.handleClient(); // Handle requests from client
 }
