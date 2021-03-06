
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <ModbusTCPSlave.h>
#include <Ticker.h>
#include <SimpleTimer.h>
#include <Wire.h>
#include <PID_v1.h>
#include <Servo.h>
#include <modbus.h>

#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
SSD1306Wire  display(0x3c, 5, 4);
SSD1306Wire  display2(0x3d, 5, 4);
 // Initialize the OLED display using Wire library

#define BLYNK_PRINT Serial    
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
#define LED_PIN     3
#define NUM_LEDS    36
#define BRIGHTNESS  100
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define LED_PIN2     4
#define NUM_LEDS2    36
#define BRIGHTNESS2  100
#define LED_TYPE2    WS2811
#define COLOR_ORDER2 GRB
CRGB leds2[NUM_LEDS2];


// Power
#define BH1750_POWER_DOWN 0x00  // No active state
#define BH1750_POWER_ON 0x01  // Waiting for measurement command
#define BH1750_RESET 0x07  // Reset data register value - not accepted in POWER_DOWN mode

// Measurement Mode
#define CONTINUOUS_HIGH_RES_MODE 0x10  // Measurement at 1 lux resolution. Measurement time is approx 120ms
#define CONTINUOUS_HIGH_RES_MODE_2 0x11  // Measurement at 0.5 lux resolution. Measurement time is approx 120ms
#define CONTINUOUS_LOW_RES_MODE 0x13  // Measurement at 4 lux resolution. Measurement time is approx 16ms
#define ONE_TIME_HIGH_RES_MODE 0x20  // Measurement at 1 lux resolution. Measurement time is approx 120ms
#define ONE_TIME_HIGH_RES_MODE_2 0x21  // Measurement at 0.5 lux resolution. Measurement time is approx 120ms
#define ONE_TIME_LOW_RES_MODE 0x23  // Measurement at 4 lux resolution. Measurement time is approx 16ms

// I2C Address
#define BH1750_1_ADDRESS 0x23  // Sensor 1 connected to GND
#define BH1750_2_ADDRESS 0x5C  // Sensor 2 connected to VCC


// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "kevincaat";
char pass[] = "12345678";


CRGBPalette16 currentPalette;
double sp = 70 ;
double sp2 = 20 ;
double sum;
double xsum;
int man1 = 0;
int man2 = 0;
double Outx = 100;
double Outx2 = 100;
double Outy = 100;
double Outy2 = 100;
double Out ;
double Out2;
double Outs ;
double Outs2;
double Outc ;
double Outc2;
String color = "Blue";



// Definition of Variable
int16_t RawData;
int16_t SensorValue[2];

double Setpoint , Input, Output;
double Setpoint2 , Input2, Output2;
PID myPID(&Input, &Output, &Setpoint ,1.2,0.9,0, DIRECT);
PID myPID2(&Input2, &Output2, &Setpoint2 ,1.2,0.9,0, DIRECT);
Servo servo;
Servo servo2;
Ticker Parada;
ModbusTCPSlave Mb;
SimpleTimer timer;

void setup() {
    Serial.begin(115200);
    Mb.begin(ssid,pass);
    Wire.begin(D1,D2);
    delay( 3000 ); // power-up safety delay
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
    FastLED.addLeds<LED_TYPE2, LED_PIN2, COLOR_ORDER>(leds2, NUM_LEDS2).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS2 );    
    
    timer.setInterval(13L, LED());
    timer.setInterval(500L, sensor());
    timer.setInterval(1131L, oled());
    timer.setInterval(8137L, v1());
    myPID.SetMode(AUTOMATIC);
    myPID2.SetMode(AUTOMATIC);
    display.init();// Initialising the UI will init the display too.
    display2.init();
    display.setI2cAutoInit(true);
    display2.setI2cAutoInit(true);

                        
    display.flipScreenVertically();
    display2.flipScreenVertically();
    servo.attach(14);
    servo2.attach(12);    
   delay(100);
}

sp = MBHoldingRegister[0];
sp2 = MBHoldingRegister[1];
man1 = MBHoldingRegister[2];
man2 = MBHoldingRegister[3];
Outs = MBHoldingRegister[4];
Outs2 = MBHoldingRegister[5];


void drawlux()
{
  int x=0;
  int y=0;

  int lux = SensorValue[0] ;
  

  display.setFont(ArialMT_Plain_24);
  String hum = String(lux) + " PSIG";
  display.drawString(0 + x, 15 + y, hum);
  int humWidth = display.getStringWidth(hum);
}

void drawlux2()
{
  int x=0;
  int y=0;

  int lux = SensorValue[1] ;
  

  display2.setFont(ArialMT_Plain_24);
  String hum = String(lux) + " PSIG";
  display2.drawString(0 + x, 15 + y, hum);
  int humWidth = display2.getStringWidth(hum);
}


void CheckConnection(){    // check every 11s if connected to Blynk server
  if(!WiFi.connected()){
    Serial.println("Not connected to WiFi"); 
    WiFi.connect();  // try to connect to server with default timeout
  }
  else{
    Serial.println("Connected to Wifi");     
  }
}

void init_BH1750(int ADDRESS, int MODE){
  //BH1750 Initializing & Reset
  Wire.beginTransmission(ADDRESS);
  Wire.write(MODE);  // PWR_MGMT_1 register
  Wire.endTransmission(true);
}

void RawData_BH1750(int ADDRESS){
  Wire.beginTransmission(ADDRESS);
  Wire.requestFrom(ADDRESS,2,true);  // request a total of 2 registers
  RawData = Wire.read() << 8 | Wire.read();  // Read Raw Data of BH1750
  Wire.endTransmission(true);
}
void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = xsum;
    
    for( int i = 1; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness );
        colorIndex += 3;
    }
}
void FillLEDsFromPaletteColors2( uint8_t colorIndex)
{
    uint8_t brightness = xsum;
    
    for( int i = 1; i < NUM_LEDS2; i++) {
        leds2[i] = ColorFromPalette( currentPalette, colorIndex, brightness );
        colorIndex += 3;
    }
}


void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 5, CRGB::Black);
    // and set every fourth one to white.
    if(SensorValue[0]>=100){
      currentPalette[0] = CRGB::Red;
      currentPalette[4] = CRGB::Red;
      currentPalette[8] = CRGB::Red;
      currentPalette[12] = CRGB::Red;
    
      }else{currentPalette[0] = CRGB::Blue; 
           currentPalette[4] = CRGB::Blue;
           currentPalette[8] = CRGB::Blue;
           currentPalette[12] = CRGB::Blue;
    
      
      }
    
}

void LED()
{
    leds[0].g =xsum; 
    leds2[0].g =xsum;
    static uint8_t startIndex = 1;
    startIndex = startIndex +1; /* motion speed */
    FillLEDsFromPaletteColors( startIndex);
    FillLEDsFromPaletteColors2( startIndex);
    FastLED.show();
    myPID.Compute();
    myPID2.Compute();
    Outx = map(Outs,0,100,0,255);
    Outx2 = map(Outs2,0,100,0,255);
    if (man1 == 1 )
    {
      Outy = Outx;
      
    }else{Outy = Output;}
    if (man2 == 1 )
    {
      Outy2 = Outx2;
      
    }else{Outy2 = Output2;}
     sum = Outy + Outy2;
    xsum = constrain(sum,0,255);
    SetupBlackAndWhiteStripedPalette();
}
void sensor ()
{
  init_BH1750(BH1750_1_ADDRESS, CONTINUOUS_HIGH_RES_MODE);
    RawData_BH1750(BH1750_1_ADDRESS);
    SensorValue[0] = RawData / 120;  
    init_BH1750(BH1750_2_ADDRESS, CONTINUOUS_HIGH_RES_MODE);
    RawData_BH1750(BH1750_2_ADDRESS);
    SensorValue[1] = abs ((RawData / 24)-17);
    Input = SensorValue[0];
    Input2 = SensorValue[1];
    Setpoint = sp;
    Setpoint2 = sp2;
    Out=map(Outy,0,255,0,100);
    Out2=map(Outy2,0,255,0,100);
    servo.write(Outy);
    servo2.write(Outy2);
}

void oled (){
    display.clear();
    drawlux();
    display.display();
    display2.clear();
    drawlux2();
    display2.display();  
  }


void v1()
{
  
  }

void loop()
{
    
  
  Mb.Run();
  delay(10);

  Parada.attach_ms(25,valor);
  timer.run();
      
}
