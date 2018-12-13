
// E1.31 Receiver and pixel controller by Chris Rees (crees@bearrivernet.net) for the Teensy 3.2 This code was modified from 
// Andrew Huxtable base code. (andrew@hux.net.au)
// This code may be freely distributed and used as you see fit for non-profit
// purposes and as long as the original author is credited and it remains open
// source
//
// Please configure your Lighting product to use Unicast to the IP the device is given from your DHCP server
// Multicast is not currently supported at the moment


// You will need the, Teensy utility installed with the added Ethernet One Socket and FastLed Libraries from:
// [url]https://www.pjrc.com/teensy/teensyduino.html[/url]
// [url]https://github.com/mrrees/MegaPixel-One-Socket-Ethernet[/url]
// [url]https://github.com/FastLED/FastLED/releases[/url]
//
// Please note the one socket library may throw errors in compiling. The error was in realation to the chip speed detection and using the 
// correct SPI speed.  
//
// The Teensy with the Octows2811 and FastLED will allow up to 5440 Pixels (32 Universes) to run.  One thing to note is if you desire
// high frame rates your pixel count must not exceed over 680 Pixels per Octo Pin.  The reason why is the ammount of time to write out to 
// these LED's takes time to shift from one LED to the next.  The more LED's per SPI or Octo Pin the more time it takes and the less frame 
// rate you will acheive.  In the Pixel Controller Wolrd 680 per SPI port is the desired balance.   For the Teensy this is a perfect balance
// any more pixels and memory starts to become an issue.  Those who whish to push more pixels per port can do so but must sacrifice the
// fastLED and or Octows2811 libraries to free up buffer space..  but your on your own and we welcome you to share your improved methods 
// with the community!


// in the code structure there is some serial feedback.  Please note enableing serial feedback will interrupt with the pixel performance
// and will slow it down.  use only for debugging and omit during production run.

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>
#include <EEPROM.h>


//*********************************************************************************


// enter desired universe and subnet  (sACN first universe is 1)
#define DMX_SUBNET 0
//#define DMX_UNIVERSE 1 //**Start** universe


// Set a different MAC address for each controller IMPORTANT!!!! you can change the last value but make sure its HEX!...
byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x21 };

//DEFINES for at Compile time.
//Leave this alone.  At current a full e1.31 frame is 636 bytes..
 #define ETHERNET_BUFFER 636 //540 is artnet leave at 636 for e1.31
 #define NUM_LEDS_PER_STRIP 680 //170
 #define NUM_STRIPS 8


///GLOBALS
 int unsigned DMX_UNIVERSE = 1; //**Start** universe 1, 9, 17, 25, 33, 41
 int unsigned UNIVERSE_COUNT = 32; //How Many Universes 8, 8, 8, 4, 8, 8
 int unsigned UNIVERSE_LAST = 32; // List the last universe typically its sequencially from start but does not have to. 8, 16, 24, 28, 32, 40
 int unsigned CHANNEL_COUNT = 510; //max channels per dmx packet
 byte unsigned LEDS_PER_UNIVERSE = 170; // Max RGB pixels 

//
 int unsigned NUM_LEDS  = UNIVERSE_COUNT * LEDS_PER_UNIVERSE; // with current fastLED and OctoWs2811 libraries buffers... do not go higher than this - Runs out of SRAM

///



//***********************************************************
// BEGIN  Dont Modify unless you know what your doing below
//***********************************************************

// Define the array of leds

CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5


unsigned char packetBuffer[ETHERNET_BUFFER];
int c = 0;
float fps = 0;
unsigned long currentMillis = 0;
unsigned long previousMillis = 0;

EthernetUDP Udp;

void setup() {

 //byte ipadd[4] ;   //Variable to store data read from EEPROM.
  //unsigned int eeAddress = 39; //EEPROM address to start reading from
//EEPROM.get( eeAddress, ipadd );

//*******************************************************
// ethernet interface ip address
//IPAddress ip(ipadd[0], ipadd[1], ipadd[2], ipadd[3]);  //IP address of ethernet shield
IPAddress ip(192, 168, 2, 21);  //IP address of ethernet shield

//IPAddress mip(239,255,0,1); //multicast IP
//*******************************************************

// E1.31 is UDP.  One socket library will only allow one protocol to be defined.  




//WIZNET RESET AND INITIALIZE
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);   // reset the WIZ820io
    delay(10);
    digitalWrite(9, HIGH);   // reset the WIZ820io
    
//SD CARD INITIALIZE
    //pinMode(10, OUTPUT); // For SD Card Stuff
    //digitalWrite(10, HIGH);  // de-select WIZ820io
    //pinMode(4, OUTPUT); //SD Card Stuff
    //digitalWrite(4, HIGH);   // de-select the SD Card
    
//Serial Port Stuff
    Serial.begin(115200);
    delay(10);

  
  // Using different LEDs or colour order? Change here...
  // ********************************************************
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(50); //value should be 0-255  Very Bright after 100 default is 50 to save on current and eyes!
  // ********************************************************

  //pins 3,4,22 are to the RGB Status LED
 
  // ********************************************************  
  //Ethernet.init(1); //-> 1 Socket with 16k RX/TX buffer for ethernet3
  
  Ethernet.begin(mac,ip);

//int success = Udp.beginMulticast(mip, 5568);
//Serial.print( "begin, success: " );
//Serial.println( success );

 Udp.begin(5568);

//DEFINE AND Turn Framing LED OFF
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
//DEFINE AND TURN STATUS LED ON  
  pinMode(3, OUTPUT);

  digitalWrite(3, LOW);
  delay(9000);
  //Turn Status LED OFF
  digitalWrite(3, HIGH);
  // ******************************************************** 
    Serial.print("server is at ");
    Serial.println(Ethernet.localIP());
    //Serial.println(F_BUS);

  //Once the Ethernet is initialised, run a test on the LEDs
  //initTest();
}



static inline void fps2(const int seconds){
  // Create static variables so that the code and variables can
  // all be declared inside a function
  static unsigned long lastMillis;
  static unsigned long frameCount;
  static unsigned int framesPerSecond;
  
  // It is best if we declare millis() only once
  unsigned long now = millis();
  frameCount ++;
  if (now - lastMillis >= seconds * 1000) {
    framesPerSecond = frameCount / seconds;
    
    Serial.print("FPS @ ");
    Serial.println(framesPerSecond);
    frameCount = 0;
    lastMillis = now;
  }

}

static inline void pixelrefresh(const int syncrefresh){
  // Create static variables so that the code and variables can
  // all be declared inside a function 
  static unsigned long frametimestart;
  static unsigned long frametimeend;
  static unsigned long frametimechk;
  static unsigned long frameonce;
  unsigned long now = micros();
 

  //start frame time
  frametimestart = now;
  
  //Serial.println(frametimechk)
   //If we have framed no need to frame again update time to most recent
   if  (syncrefresh == 1){
   frametimeend = frametimestart; 
   frameonce = 1;
   }
   
//If we havent framed this will increment via time and at some point will be true, 
//if so we need to frame to clear out any buffer and the hold off untill 
//we receive our next valid dmx packet. We use the pixel protocol to get a general rule of timing to compare to.

frametimechk = frametimestart - frametimeend;
 // num leds time 30us + 300us reset to simulate the time it would take to write out pixels. 
 //this should help us not loop to fast and risk premature framing and jeopordize ethernet buffer
 if  (frametimechk >= (NUM_LEDS * 30) + 300){
  frametimeend = frametimestart;


    if (frameonce == 1){
    LEDS.show();
    Serial.println ("Partial framing detected");
    frameonce = 0;  
  }
  
 }
 
}



void sacnDMXReceived(unsigned char* pbuff, int count) {
  static unsigned long uniloopcount;
  if (count > CHANNEL_COUNT) count = CHANNEL_COUNT;
  byte b = pbuff[113]; //DMX Subnet
  if ( b == DMX_SUBNET) {
    b = pbuff[114];  //DMX Universe
    byte s = pbuff[111]; //sequence
    static unsigned long ls; // Last Sequence
    if (s > ls){
    uniloopcount = 0; 
    ls = s;
    }
   //turn framing LED OFF
   digitalWrite(4, HIGH);
    //Serial.print("UNI ");
    //Serial.println(count );
    //Serial.println(s);
    if ( b >= DMX_UNIVERSE && b <= UNIVERSE_LAST) {
        //Serial.println(b );
      if ( pbuff[125] == 0 ) {  //start code must be 0   
      int ledNumber = (b - DMX_UNIVERSE) * LEDS_PER_UNIVERSE;
       // sACN packets come in seperate RGB but we have to set each led's RGB value together
       // this 'reads ahead' for all 3 colours before moving to the next led.
       //Serial.println("*");
       for (int i = 126;i < 126+count;i = i + 3){
          byte charValueR = pbuff[i];
          byte charValueG = pbuff[i+1];
          byte charValueB = pbuff[i+2];
          leds[ledNumber] = CRGB(charValueR,charValueG,charValueB); //RBG GRB
          //Serial.println(ledNumber);
          ledNumber++;
        }




        

        
        }

       

 
      
          
 
      
      }


    
  }

         uniloopcount ++;
         //Serial.print("UNILOOP");
         //Serial.println(uniloopcount);

        //if (b == UNIVERSE_LAST){
        if (uniloopcount >= UNIVERSE_COUNT){ 
        //Turn Framing LED ON
        digitalWrite(4, LOW);
        LEDS.show();
        pixelrefresh(1);
        uniloopcount = 0;
        //Frames Per Second Function fps(every_seconds)
        fps2(5);
        }

}


int checkACNHeaders(unsigned char* messagein, int messagelength) {
  //Do some VERY basic checks to see if it's an E1.31 packet.
  //Bytes 4 to 12 of an E1.31 Packet contain "ACN-E1.17"
  //Only checking for the A and the 7 in the right places as well as 0x10 as the header.
  //Technically this is outside of spec and could cause problems but its enough checks for us
  //to determine if the packet should be tossed or used.
  //This improves the speed of packet processing as well as reducing the memory overhead.
  //On an Isolated network this should never be a problem....
  if ( messagein[1] == 0x10 && messagein[4] == 0x41 && messagein[12] == 0x37) {   
      int addresscount = (byte) messagein[123] * 256 + (byte) messagein[124]; // number of values plus start code
      return addresscount -1; //Return how many values are in the packet.
    }
  return 0;
}



void initTest() //runs at board boot to make sure pixels are working
{
  LEDS.showColor(CRGB(255, 0, 0)); //turn all pixels on red
   delay(1000);
   LEDS.showColor(CRGB(0, 255, 0)); //turn all pixels on green
   delay(1000);
   LEDS.showColor(CRGB(0, 0, 255)); //turn all pixels on blue
   delay(1000);
   LEDS.showColor(CRGB(0, 0, 0)); //turn all pixels off
}

void loop() {
   //Process packets
   int packetSize = Udp.parsePacket(); //Read UDP packet count
  
   if(packetSize){
    //Serial.println(packetSize);
    Udp.read(packetBuffer,ETHERNET_BUFFER); //read UDP packet
    
    
    int count = checkACNHeaders(packetBuffer, packetSize);
    if (count) {
      
     //Serial.print("packet size first ");
     //Serial.println(packetSize);

     
     

    
     sacnDMXReceived(packetBuffer, count); //process data function
     
    
     
    
    }  



  }

pixelrefresh(0);


}
