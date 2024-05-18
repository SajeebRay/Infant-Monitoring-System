#include <WiFi.h>       // standard library
#include <WebServer.h>  // standard library
#include "SuperMon.h"   // .h file that stores your html page code
#include <DHT.h>

// as you just need to refresh the browser as opposed to reconnection to the web server
#define USE_INTRANET
// replace this with your homes intranet connect parameters
#define LOCAL_SSID "Welcome"
#define LOCAL_PASS "pust.ac.bd"
// once  you are read to go live these settings are what you client will connect to
#define AP_SSID "TestWebSite"
#define AP_PASS "023456789"

// start your defines for pins for sensors, outputs etc.
#define PIN_TOY 26 // connected to nothing but an example of a digital write from the web page
#define PIN_FAN 27    // pin 27 and is a PWM signal to control a fan speed
#define PIN_FAN 2     //On board LED symbol of fan
#define PIN_A1 35     // some analog input sensor
#define dhtPin 34 // dht11 sensor input

DHT dht(dhtPin,DHT22);
// variables to store measure data and sensor states
float TempA0 = 0, HumA1 = 0;
float VoltsA0 = 0, VoltsA1 = 0;
int FanSpeed = 0;
bool Fan = false, BabyToy = false;
uint32_t SensorUpdate = 0;
int FanRPM = 0;
// the XML array size needs to be bigger that your maximum expected size. 2048 is way too big for this example
char XML[2048];
// just some buffer holder for char operations
char buf[32];
// variable for the IP reported when you connect to your homes intranet (during debug mode)
IPAddress Actual_IP;
// definitions of your desired intranet created by the ESP32
IPAddress PageIP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress ip;
// gotta create a server
WebServer server(80);

void setup() 
{
  Serial.begin(9600);

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);

  // turn off  fan, previous name led
  Fan = false;
  digitalWrite(PIN_FAN, Fan);
  // if your web page or XML are large, you may not get a call back from the web page
  // and the ESP will think something has locked up and reboot the ESP
  // not sure I like this feature, actually I kinda hate it
  // disable watch dog timer 0
  disableCore0WDT();
  // just an update to progress
  Serial.println("starting server");
  // if you have this #define USE_INTRANET,  you will connect to your home intranet, again makes debugging easier
#ifdef USE_INTRANET
  WiFi.begin(LOCAL_SSID, LOCAL_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  Actual_IP = WiFi.localIP();
#endif
  // if you don't have #define USE_INTRANET, here's where you will creat and access point
  // an intranet with no internet connection. But Clients can connect to your intranet and see
  // the web page you are about to serve up
#ifndef USE_INTRANET
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(100);
  WiFi.softAPConfig(PageIP, gateway, subnet);
  delay(100);
  Actual_IP = WiFi.softAPIP();
  Serial.print("IP address: "); Serial.println(Actual_IP);
#endif

  printWifiStatus();
  // these calls will handle data coming back from your web page
  // this one is a page request, upon ESP getting / string the web page will be sent
  server.on("/", SendWebsite);
  // upon esp getting /XML string, ESP will build and send the XML, this is how we refresh
  // just parts of the web page
  server.on("/xml", SendXML);
  // this process will be documented in the SuperMon.h web page code
  server.on("/BUTTON_0", ProcessButton_0);
  server.on("/BUTTON_1", ProcessButton_1);
  // finally begin the server
  server.begin();
}

void loop() 
{
  // analog input can be from temperature sensors, light sensors, digital pin sensors, etc.
  if ((millis() - SensorUpdate) >= 50) 
  {
    //Serial.println("Reading Sensors");
    SensorUpdate = millis();
    TempA0 = readDHTTemperature();
    HumA1 = readDHTHumidity();
    Serial.println(TempA0);
    Serial.println(HumA1);
  }

  // call this handleClient repeatidly--otherwise the web page
  // will not get instructions to do something
  server.handleClient();
}
float readDHTTemperature()
{
  float t = dht.readTemperature();
  if (isnan(t)) {    
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    Serial.println(t);
    return t;
  }
}
float readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return -1;
  }
  else {
    Serial.println(h);
    return h;
  }
}
void ProcessButton_0() 
{
  Serial.println("Button 0 press");
  Fan = !Fan;
  digitalWrite(PIN_FAN, Fan);
  Serial.print("Button 0 "); 
  Serial.println(Fan);
  server.send(200, "text/plain", ""); //Send web page
}
// same notion for processing button_1
void ProcessButton_1()
 {
  Serial.println("Button 1 press");
  BabyToy = !BabyToy;
  digitalWrite(PIN_TOY, BabyToy);
  Serial.print("Button 1 ");
  Serial.println(BabyToy);

  server.send(200, "text/plain", ""); //Send web page
}
// code to send the main web page
// PAGE_MAIN is a large char defined in SuperMon.h
void SendWebsite()
{
  Serial.println("sending web page");
  server.send(200, "text/html", PAGE_MAIN);
}
void SendXML() 
{
  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");
  // send TempA0
  sprintf(buf, "<B0>%d.%d</B0>\n", (int) (TempA0), abs((int) (TempA0 * 10)  - ((int) (TempA0) * 10)));
  strcat(XML, buf);
  // send bits1
  sprintf(buf, "<B1>%d.%d</B1>\n", (int) (HumA1), abs((int) (HumA1 * 10)  - ((int) (HumA1) * 10)));
  strcat(XML, buf);
  // show Fan status
  if (Fan) {
    strcat(XML, "<FAN>1</FAN>\n");
  }
  else {
    strcat(XML, "<FAN>0</FAN>\n");
  }
  if (BabyToy) {
    strcat(XML, "<TOY>1</TOY>\n");
  }
  else {
    strcat(XML, "<TOY>0</TOY>\n");
  }
  strcat(XML, "</Data>\n");
  // actually print it to the serial monitor and use some text editor to get the size
  Serial.println(XML);
  // you may have to play with this value, big pages need more porcessing time, and hence
  // a longer timeout that 200 ms
  server.send(200, "text/xml", XML);
}

void printWifiStatus() 
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(ip);
}

