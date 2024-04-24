#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <array>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebSrv.h>

using namespace std;

//WiFi Credentials
const char* ssid = "SSID";
const char* password = "PASSWORD";

const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 3.0rem;}
    p {font-size: 3.0rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 25px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>Wordclock</h2>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}
</script>
<form action="/get">
  input1: <input type="text" name="input1">
  <input type="submit" value="Submit">
</form>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var){
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER"){
    String buttons = "";
    buttons += "<h4>Rainbow</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"1\" " + outputState(1) + "><span class=\"slider\"></span></label>";
    //buttons += "<h4>Output - GPIO 4</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    //buttons += "<h4>Output - GPIO 2</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider\"></span></label>";
    return buttons;
  }
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}

//Pin und Anzahl Pixel & UTC Offset
#define PIN            2
#define NUMPIXELS      114

// Winterzeit 3600 Sommerzeit 7200
const long utcOffsetInSeconds = 7200;

//Wifi verbinden
WiFiClientSecure client;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

int delayval = 1000;

int sensorValue;

int color_on[] = {0, 25, 0};
int color_off[] = {0, 0, 0};

bool state = false;
bool h = false;
int H2 = 0;

int Esist[] = {5, 6, 7, 9, 10};
int Uhr[] = {107, 108, 109};
int vor[] = {39, 40, 41};
int nach[] = {35, 36, 37, 38};
int halb[] = {54, 53, 52, 51};
int m_fuenf[] = {0, 1, 2, 3};
int m_zehn[] = {14, 13, 12, 11};
int m_zwanzig[] {21, 20, 19, 18, 17, 16, 15};
int m_viertel[] = {28, 27, 26, 25, 24, 23, 22};
int dot_min[] = {112, 113, 111, 110};

int h_eins[] = {60, 59, 58, 57};
int h_zwei[] = {58, 57, 56, 55};
int h_drei[] = {75, 74, 73, 72};
int h_vier[] = {84, 85, 86, 87};
int h_fuenf[] = {69, 68, 67, 66};
int h_sechs[] = {100, 101, 102, 103, 104};
int h_sieben[] = {65, 64, 63, 62, 61, 60};
int h_acht[] = {94, 95, 96, 97};
int h_neun[] = {80, 81, 82, 83};
int h_zehn[] = {90, 91, 92, 93};
int h_elf[] = {77, 78, 79};
int h_zwoelf[] = {49, 48, 47, 46, 45};

int input1 = 222; 

const int numReadings = 10;

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int inputPin = A0;


void setup() {
  Serial.begin(9600);
  pixels.begin();

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  //Connecting to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println("Connected!");
  Serial.println("IP address:");
  Serial.println(WiFi.localIP());  

  rainbow(1);
  pixels.clear();

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    if (inputMessage1 == "1"){
      int rnb = inputMessage1.toInt();
      rainbow(rnb);
      pixels.clear();
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}

void loop() {
  timeClient.update();  

   // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(inputPin);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  // send it to the computer as ASCII digits
  //Serial.println(average);

  sensorValue = analogRead(A0); // read analog input pin 0
  //int col = sensorValue;
  int col2 = average/10*10;


  if (col2 >= 255){
    col2 =255;
  }
  if (col2 < 10){
    col2 =10;
  }

  color_on[0]=col2;
  color_on[1]=col2;
  color_on[2]=col2;

    Serial.println(col2);

  F_test(Esist, (sizeof(Esist)/4)-1, true);

  float Minute = timeClient.getMinutes();
    int Min2=Minute;
    int last = Min2 % 10;
    if (last >= 5){
      last = last-5;
    }
    if (last == 0){
      state = false; 
      F_test(dot_min, (sizeof(dot_min)/4)-1 , (state));
    } else if (last > 0){
      state = true;
      F_test(dot_min, (last)-1,(state));
    }

    if ((Minute >= 0) && (Minute < 5)){
      state = false;
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
    } else if ((Minute >= 5) && (Minute < 10)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      state = true;
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
    } else if ((Minute >= 10) && (Minute < 15)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      state = true;
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
    } else if ((Minute >= 15) && (Minute < 20)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      state = true;
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
    } else if ((Minute >= 20) && (Minute < 25)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      state = true;
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
    } else if ((Minute >= 25) && (Minute < 30)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
    } else if ((Minute >= 30) && (Minute <= 35)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(halb,(sizeof(halb)/4)-1, (state));
    } else if ((Minute >= 35) && (Minute < 40)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
      state = true;
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
    } else if ((Minute >= 40) && (Minute < 45)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
    } else if ((Minute >= 45) && (Minute < 50)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
    } else if ((Minute >= 50) && (Minute < 55)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
    } else if ((Minute >= 55) && (Minute <= 59)){
      state = false;
      F_test(Uhr,(sizeof(Uhr)/4)-1, (state));
      F_test(m_zehn,(sizeof(m_zehn)/4)-1, (state));
      F_test(m_viertel,(sizeof(m_viertel)/4)-1, (state));
      F_test(m_zwanzig,(sizeof(m_zwanzig)/4)-1, (state));
      F_test(halb,(sizeof(halb)/4)-1, (state));
      F_test(nach,(sizeof(nach)/4)-1, (state));
      state = true;
      F_test(m_fuenf,(sizeof(m_fuenf)/4)-1, (state));
      F_test(vor,(sizeof(vor)/4)-1, (state));
    }

    int Hour = timeClient.getHours();
    H2=Hour;
    
    if (H2 >= 12){
      H2 = H2-12;
    } 
    if (Minute < 25){
      h=false;
    } else {
      h=true;
    }
    if (h) {
       H2 = H2+1;
    }

    Serial.println(H2);

    if (H2 < 1) {
      state = false;
      F_test(h_elf,(sizeof(h_elf)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_zwoelf,(sizeof(h_zwoelf)/4)-1, (state));
    }
    if (H2 == 1) {
      state = false;
      F_test(h_zwoelf,(sizeof(h_zwoelf)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_eins,(sizeof(h_eins)/4)-1, (state));
    }
    if (H2 == 2) {
      state = false;
      F_test(h_eins,(sizeof(h_eins)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_zwei,(sizeof(h_zwei)/4)-1, (state));
    }
    if (H2 == 3) {
      state = false;
      F_test(h_zwei,(sizeof(h_zwei)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_drei,(sizeof(h_drei)/4)-1, (state));
    }
    if (H2 == 4) {
      state = false;
      F_test(h_drei,(sizeof(h_drei)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_vier,(sizeof(h_vier)/4)-1, (state));
    }
    if (H2 == 5) {
      state = false;
      F_test(h_vier,(sizeof(h_vier)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_fuenf,(sizeof(h_fuenf)/4)-1, (state));
    }
    if (H2 == 6) {
      state = false;
      F_test(h_fuenf,(sizeof(h_fuenf)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_sechs,(sizeof(h_sechs)/4)-1, (state));
    }
    if (H2 == 7) {
      state = false;
      F_test(h_sechs,(sizeof(h_sechs)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_sieben,(sizeof(h_sieben)/4)-1, (state));
    }
    if (H2 == 8) {
      state = false;
      F_test(h_sieben,(sizeof(h_sieben)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      state = true;
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
    }
    if (H2 == 9){
      state = false;
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
    } 
    if (H2 == 10) {
      state = false;
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_zehn,(sizeof(h_zehn)/4)-1, (state));
    }
    if (H2 == 11) {
      state = false;
      F_test(h_zehn,(sizeof(h_zehn)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_elf,(sizeof(h_elf)/4)-1, (state));
    }
    if (H2 == 12) {
      state = false;
      F_test(h_elf,(sizeof(h_elf)/4)-1, (state));
      F_test(h_neun,(sizeof(h_neun)/4)-1, (state));
      F_test(h_acht,(sizeof(h_acht)/4)-1, (state));
      state = true;
      F_test(h_zwoelf,(sizeof(h_zwoelf)/4)-1, (state));
    }

    pixels.show();
    delay(delayval);
}

void F_test(int *test1, int b, int state){
  if (!state){
    for (int i = 0; i <= b; i++) {
      pixels.setPixelColor(test1[i], pixels.Color(color_off[0],color_off[1],color_off[2]));
    }
  } else if (state) {
      for (int i = 0; i <= b; i++) {
        pixels.setPixelColor(test1[i], pixels.Color(color_on[0],color_on[1],color_on[2]));
      }
    }
}

void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<pixels.numPixels(); i++) { 
      int pixelHue = firstPixelHue + (i * 65536L / pixels.numPixels());
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show();
    delay(wait);
  }
}
