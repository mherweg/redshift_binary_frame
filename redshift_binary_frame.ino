/*
 ESP8266 MQTT 

this sketch subscibes 2 MQTT topics:

inTopic = "huette/clubraum/000/redshift/actuators/frame

 receive a binary MQTT message and update the whole display
 currently: 8 pixel in one byte  , 8 x 8 = 64 byte per frame(picture)
 later versions: maybe 1 byte per pixel for different level of brightness

inTopic2 = huette/clubraum/000/redshift/actuators/set_pixel

 receive a  command (ascii)
 possible commands:
 0-512 #1  (set one pixel)
 0-512 #0  (clear one pixel)
 text Hello (display Text, max 10 letters)
 
*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>

#include <Adafruit_GFX.h>

#include "nokia5110_chars.h"
#include "conf.h"
                 //war:
#define TAKT D5  // D1
#define HIDE D6  // D2
#define DATA D7  // D3

#define DEBUG
#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTDEC(x)     Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
#endif 


#define BIT_S(var,b) ((var&(1<<b))?1:0)

// clear bit
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

// set bit
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


WiFiClient espClient;
PubSubClient client(espClient);



class MyMatrix : public Adafruit_GFX { 
  };

int16_t width = 64;
int16_t height = 8;

MyMatrix   ada_matrix(width, height); 

    /*  position */
uint8_t cursor_x;
uint8_t cursor_y;



int r,g,b;

byte framebuffer[NUMPIXELS];
String stringPixel, stringFrame, stringIntopic ;

// WIP
String scrolltext = " Hello World!";


void setup() {
 pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
 pinMode(DATA, OUTPUT);
 pinMode(HIDE, OUTPUT);
 pinMode(TAKT, OUTPUT);
 Serial.begin(115200);

  setup_wifi();
  delay(500);
  client.setServer(mqtt_server, 1883);
 
  client.subscribe("inTopic");
  client.subscribe("inTopic2");
  
  client.setCallback(callback);

  stringPixel = String(inTopic2);
  stringFrame = String(inTopic);

  ArduinoOTA.setHostname("redshift"); // give an name to our module
  ArduinoOTA.begin(); // OTA initialization

  
} //setup

void setup_wifi() {
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
     framebuffer[8]=8;
     show();
    delay(500);
    Serial.print(".");
    
  }

  Serial.println("");
  Serial.println("WiFi connected");
  framebuffer[8]=4;
  show();
  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  client.subscribe(inTopic);
  client.setCallback(callback);
  
} //setup_wifi



void set_pixel(int x, int y,int color ){
  byte b_index , mybit;
  b_index = x/8 + y*8;
  mybit = x % 8;
  
  // color= 0 = off, alles andere: on
  if (color > 0)
    sbi(framebuffer[b_index], mybit);
  else
    cbi(framebuffer[b_index], mybit);
  //Serial.println(b_index);
  //Serial.println(mybit);
  //Serial.println(image[b_index]);
  //Serial.println("-----");
} // set_pixel(x,y,col)


// eindimensional, ueber alle Zeilen
void setPixelColor(int j, char r ){

  //nothing todo - it works on all lines
 //int x=j%8;
 //int y = j/8;
 DEBUG_PRINT("setPixelColor:");
 DEBUG_PRINTDEC(j);
 DEBUG_PRINT(" r:");
 DEBUG_PRINTDEC(r);
 DEBUG_PRINTLN("");
 //DEBUG_PRINTDEC(y);
 set_pixel( j, 0 , r );
}


void shiftbyte(byte dat)  // wird von show() benutzt
{
  unsigned char p;
  int i;
  p=0x01;

  for(i=0;i<8;i++)
  {
  if (dat & p)
       digitalWrite(DATA, 1);   
  else
      digitalWrite(DATA, 0);
    
  digitalWrite(TAKT, 0);
  delayMicroseconds(5);
  digitalWrite(TAKT, 1);
  delayMicroseconds(5);
  p<<=1;
  }
 
}


void show()
{
  int line,j;
  int offset=0;
  byte val;  
   digitalWrite(HIDE, 1)  ;   //hide 
  //for (line=0;line<8;line++)
  for (line=8;line>=0;line--)
  {
    //for(j=7;j>=0;j--)
    // 8 bytes on each line
    for(j=0;j<8;j++)
    { 
     //val=framebuffer[offset]-1; //show inverted
     offset=line*8+j;
      val=framebuffer[offset];
     shiftbyte(val);
    }
  }
   digitalWrite(HIDE, 0);    //show
  
  }


void setpixel( byte* payload, unsigned int length){
  int splitat;
  String s = "";
  for(int i = 0; i < length; i++) {
    
        s += (char)payload[i];
    //  Serial.println((char)payload[i]);
  
    
     if (isWhitespace((char)payload[i])) {
        splitat = i;
        i = length;
     } //if
      
   } //for
  
   String Numberstring = "";
   String Colorstring = "";
   for(int i = 0; i < splitat; i++) {
       Numberstring += (char) payload[i];   
   }

   for (int i = (splitat + 1); i < length; i++) {
      Colorstring += (char) payload[i];
   }
 
//Colorstring contains something like = "#00ff00" or "00ff00"
// you need to convert it to 0,255,0 and insert it to the two calls below
// Am ende Newline.
   int intcolors=(int) strtol(&Colorstring[1],NULL,16);
   int r = intcolors >> 16;
   int g = intcolors >> 8 & 0xFF;
   int b = intcolors & 0xFF;

  
  Serial.print("Number:" + Numberstring + " Color:" + Colorstring + " intcolors:");
  //Serial.println(intcolors);

 if(Numberstring == "text") {
    //scrolltext = Colorstring;
    //DEBUG_PRINT("scrolltext:");
    //DEBUG_PRINTLN(scrolltext);
    //scrolltext(); 
    cursor_x=0;
    cursor_y=0;
    nokia_lcd_write_string(Colorstring.c_str() ,1);
    show(); 
  }
  
  if(Numberstring.toInt() == 255) {
    for(int i = 0; i < NUMPIXELS; i++) {
      if (r == 0)
        framebuffer[i]=0;
      else
        framebuffer[i]=1;
      
    }
    show(); 
  }
  else {
    setPixelColor(Numberstring.toInt(), r ); 
    show(); // This sends the updated pixel color to the hardware.
  }
}  //setpixel



// ein Frame das binär via MQTT empfangen wurde
// wird in den Frambuffer geschrieben und 1x show() aufgerufen
// 
void writeframe(byte* payload, unsigned int length){
   //Serial.print("TOPIC=frame");

  // 1 pixel = 1 bit , 1 byte = 8 pixel
  //write bytes to framebuffer
  for (int i = 0; i < (NUMPIXELS) ; i++) {
    //Serial.print((char)payload[i]);
    //Serial.print(" ");
    framebuffer[i] = payload[i];
  }


  // wenn 1 pixel = 1 byte dann das hier
   //write frame to LEDs
  //  for(int i = 0; i < NUMPIXELS ; i++) {
      //Serial.print(" ");
      //Serial.print(j);
      //Serial.print(":");
      //Serial.print(r);
      
   //   r = payload[i];
   //   setPixelColor(i, r ); 
   //  }
  Serial.println();
  show();  
} //writeframe

void callback(char* topic, byte* payload, unsigned int length) {

  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");
  //Serial.print("length:");
  //Serial.print(   length );

  stringIntopic = String(topic);
  if (stringIntopic == stringPixel ){
    //Serial.print("TOPIC=setpixel");
    setpixel(payload,length);
  }
  
  if (stringIntopic == stringFrame ){
    writeframe(payload,length);
  }
} //callback

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
      client.subscribe(inTopic2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
} //reconnect



void nokia_lcd_write_char(char code, uint8_t scale)
{
  register uint8_t x, y;

  for (x = 0; x < 5*scale; x++)
    for (y = 0; y < 7*scale; y++)
      if (pgm_read_byte(&CHARSET[code-32][x/scale]) & (1 << y/scale))
        set_pixel(cursor_x + x, cursor_y + y, 1);
      else
        set_pixel(cursor_x + x, cursor_y + y, 0);

  cursor_x += 5*scale + 1;
  if (cursor_x >= 64) {
    cursor_x = 0;
    //cursor_y += 7*scale + 1;
    cursor_y = 0;
  }
  if (cursor_y >= 10) {
    cursor_x = 0;
    cursor_y = 0;
  }
}

void nokia_lcd_write_string(const char *str, uint8_t scale)
{
  while(*str)
    nokia_lcd_write_char(*str++, scale);
}

void nokia_lcd_set_cursor(uint8_t x, uint8_t y)
{
  cursor_x = x;
  cursor_y = y;
}





void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ArduinoOTA.handle(); 
  
    delay(10);
    // show();
}  //loop
