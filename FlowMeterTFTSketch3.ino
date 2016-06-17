/*
 * Pin setup
 * 
 * TFT Display
 * GND to Ground
 * Vin to 5 volts
 * CLK to Digital 13
 * MISO to Digital 12
 * MOSI to Digital 11
 * CS to Digital 10
 * D/C to Digital 9
 * Y+ to A2
 * X+ to Digital 8
 * Y- to Digital 7
 * X- to A3
 * 
 * FlowMeter
 * Red to 5 volts
 * Black to Ground
 * Yellow to Digital 2
 */

#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include "Adafruit_HX8357.h"
#include "TouchScreen.h"


// These are the four touchscreen analog pins
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 7   // can be a digital pin
#define XP 8   // can be a digital pin

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 110
#define TS_MINY 80
#define TS_MAXX 900
#define TS_MAXY 940

#define MINPRESSURE 10
#define MAXPRESSURE 1000

// The display uses hardware SPI, plus #9 & #10
#define TFT_RST -1  // dont use a reset pin, tie to arduino RST if you like
#define TFT_DC 9
#define TFT_CS 10

Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


byte sensorInterrupt    = 0;
byte sensorPin          = 2;
byte solenoidPin        = 4;
float calibrationFactor = 34;
volatile byte pulseCount;
unsigned long flowRate;
float flowRateDisplay;
float oldFlowRate;
float flowML;
float flow;
float totalA;
float oldTotalA;
float totalADisplay;
float oldTime;
float setAmount;
float oldSetAmount;
bool Liter = false;
bool valveOpen = false;
bool oldValveOpen = false;
bool onMainScreen = true;
bool manualOverride = false;
float conversionFactor;

//Button integers for lower and higher x and y limits
int button147ClearYLow = 665;
int button147ClearYHigh = 900;
int button2580YLow = 375;
int button2580YHigh = 610;
int button369SetYLow = 85;
int button369SetYHigh = 320;

int button123XLow = 150;
int button123XHigh = 255;
int button456XLow = 305;
int button456XHigh = 395;
int button789XLow = 450;
int button789XHigh = 550;
int buttonClear0SetXLow = 605;
int buttonClear0SetXHigh = 760;

void setup()
{
  Serial.begin(38400);
  pinMode(solenoidPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  tft.begin(HX8357D);

  pulseCount    = 0;
  flow          = 0;
  flowRate      = 0;
  totalA        = 0.00;
  totalADisplay = 0.00;
  oldTime       = 0;
  setAmount     = 0.00;

  //Draw the main screen out
  drawMainScreen();

  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}


void loop()
{
  //Set point p when pressing screen
  TSPoint p = ts.getPoint();

  //Here is where we will determine the button locations and actions
  //Main screen Buttons
  if (onMainScreen == true)
  {
    //RESET Button
    if (p.y > 80 && p.y < 320 && p.x > 200 && p.x < 390 && onMainScreen == true)
    {
      resetButton();
    }
  
    //SET Button on Main Screen
    if (p.y > 80 && p.y < 320 && p.x > 455 && p.x < 570 && onMainScreen == true)
    {
      setButtonMainScreen();
    }
  
    //Valve Button
    if (p.y > 670 && p.y < 920 && p.x > 570 && p.x < 900 && onMainScreen == true)
    {
       //intentionall blank
    }
  
    //Blank Button
    if (p.y > 380 && p.y < 615 && p.x > 570 && p.x < 900 && onMainScreen == true)
    {
      AutoManualButton();
    }
  
    // GallonLiter Toggle Button
    if (p.y > 90 && p.y < 320 && p.x > 630 && p.x < 900 && onMainScreen == true)
    {
      gallonLiterToggleButton();
    }
  }
  else //in set screen
  {
    //1
    if (p.y > button147ClearYLow && p.y < button147ClearYHigh && p.x > button123XLow && p.x < button123XHigh)
    {
      setNumberButton(1.00);
    }
    //2
    if (p.y > button2580YLow && p.y < button2580YHigh && p.x > button123XLow && p.x < button123XHigh)
    {
      setNumberButton(2.00);
    }
    //3
    if (p.y >  button369SetYLow && p.y <  button369SetYHigh && p.x > button123XLow && p.x < button123XHigh)
    {
      setNumberButton(3.00);
    }

    //4
    if (p.y > button147ClearYLow && p.y < button147ClearYHigh && p.x > button456XLow && p.x < button456XHigh)
    {
      setNumberButton(4.00);
    }
    //5
    if (p.y > button2580YLow && p.y < button2580YHigh && p.x > button456XLow && p.x < button456XHigh)
    {
      setNumberButton(5.00);
    }
    //6
    if (p.y >  button369SetYLow && p.y <  button369SetYHigh && p.x > button456XLow && p.x < button456XHigh)
    {
      setNumberButton(6.00);
    }

    //7
    if (p.y > button147ClearYLow && p.y < button147ClearYHigh && p.x > button789XLow && p.x < button789XHigh)
    {
      setNumberButton(7.00);
    }
    //8
    if (p.y > button2580YLow && p.y < button2580YHigh && p.x > button789XLow && p.x < button789XHigh)
    {
      setNumberButton(8.00);
    }
    //9
    if (p.y >  button369SetYLow && p.y <  button369SetYHigh && p.x > button789XLow && p.x < button789XHigh)
    {
      setNumberButton(9.00);
    }

    //Clear
    if (p.y > button147ClearYLow && p.y < button147ClearYHigh && p.x > buttonClear0SetXLow && p.x < buttonClear0SetXHigh)
    {
      clearButton();
    }
    //0
    if (p.y > button2580YLow && p.y < button2580YHigh && p.x > buttonClear0SetXLow && p.x < buttonClear0SetXHigh)
    {
      setNumberButton(0.00);
    }
    //Set
    if (p.y > button369SetYLow && p.y < button369SetYHigh && p.x > buttonClear0SetXLow && p.x < buttonClear0SetXHigh)
    {
      setButtonSetScreen();
    }

    
  }

  //Set Conversion Factors
  if (Liter == true)
  {
    conversionFactor = 1;
  }
  else
  {
    conversionFactor = (1 / 3.78541);
  }

  //Start Loop that updates the values every 1 second
  if((millis() - oldTime) > 1000)
  {
    detachInterrupt(sensorInterrupt);

    flowML = pulseCount * (1000/(calibrationFactor*60));
    flowRate = (flowML * (60000 / (millis() - oldTime))) / 1000;
    
    totalA += (flowML / 1000);

    flowRateDisplay = ((flowRate) * conversionFactor);
    totalADisplay = (totalA * conversionFactor);

    //Debug    
    Serial.print("FLow Rate: "); Serial.println(flowRate);    
    Serial.print("FLow Rate is Display: "); Serial.println(flowRateDisplay);
    Serial.print("TotalA : "); Serial.println(totalADisplay);
    Serial.print("setAmount : "); Serial.println(setAmount);
    Serial.print("Pulse Count"); Serial.println(pulseCount);
    Serial.print("How many Seconds "); Serial.println((millis() - oldTime));
    Serial.println("");

    
    
    if ((onMainScreen == true) && (!(oldFlowRate == flowRate)))
    {
      printFlowRate();
    }

    if ((onMainScreen == true) && !(totalA == oldTotalA))
    {
      printTotalA();
    }

    toggleValve();
      
    pulseCount = 0;

    oldTime = millis();

    oldValveOpen = valveOpen;

    oldFlowRate = flowRate;

    oldTotalA = totalA;

    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    
  }
  
}

void pulseCounter()
{
  pulseCount++;
}

void drawMainScreen()
{
  onMainScreen = true;

  //Set background color to BLACK
  tft.fillScreen(HX8357_BLACK);
  //Set orientation to Horizon
  tft.setRotation(3);
  //Print Static Text
  printFlowRate();
  printTotalA();
  printSetAmountMainScreen();
  //Set default text size and color
  tft.setTextSize(4);
  tft.setTextColor(HX8357_WHITE);

  //Draw Static boxes and text
  tft.drawRoundRect(5, 5, 470, 55, 10,HX8357_RED); 
  tft.drawRoundRect(5, 65, 340, 55, 10,HX8357_RED); 
    tft.fillRoundRect(355, 65, 120, 55, 10,HX8357_RED); 
    tft.drawRoundRect(355, 65, 120, 55, 10,HX8357_WHITE); 
  tft.drawRoundRect(5, 125, 340, 55, 10,HX8357_RED); 
    tft.fillRoundRect(355, 125, 120, 55, 10,HX8357_GREEN); 
    tft.drawRoundRect(355, 125, 120, 55, 10,HX8357_BLUE); 
 
  tft.setTextSize(3);
  tft.setCursor(15, 22);
  tft.print("Flow Rate: ");
  tft.setCursor(15, 82);
  tft.print("Total: ");
  tft.setCursor(15, 142);
  tft.print("Set To: ");
  tft.setCursor(372, 82);
  tft.print("RESET");
  tft.setCursor(372, 142);
  tft.setTextColor(HX8357_BLUE);
  tft.print(" SET");
  tft.setTextColor(HX8357_WHITE);

  
  if (Liter == false)
  {
    tft.setCursor(340, 22);
    tft.print("Gal/Min");
    tft.setCursor(280, 82);
    tft.print("Gal");
    tft.setCursor(280, 142);
    tft.print("Gal");
  }
  else
  {
    tft.setCursor(340, 22);
    tft.print("Lit/Min");
    tft.setCursor(280, 82);
    tft.print("Lit");
    tft.setCursor(280, 142);
    tft.print("Lit");
  }

  //Print Dynamic Buttons
  printValveButton();
  printGalLitButton();
  printAutoManualButton();

}

void drawSetScreen()
{
   onMainScreen = false;
   //Set background color to BLACK
  tft.fillScreen(HX8357_BLACK);
  //Set orientation to Horizon
  tft.setRotation(3);
  //Set default text size and color
  tft.setTextSize(4);
  tft.setTextColor(HX8357_WHITE);

  //Draw out Keypad
  tft.fillRoundRect(5,   5, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(165, 5, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(325, 5, 150, 55, 10, HX8357_RED);

  tft.fillRoundRect(5,   65, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(165, 65, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(325, 65, 150, 55, 10, HX8357_RED);

  tft.fillRoundRect(5,   125, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(165, 125, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(325, 125, 150, 55, 10, HX8357_RED);

  tft.fillRoundRect(5,   185, 150, 55, 10, HX8357_BLUE);
  tft.fillRoundRect(165, 185, 150, 55, 10, HX8357_RED);
  tft.fillRoundRect(325, 185, 150, 55, 10, HX8357_GREEN);

  tft.drawRoundRect(5, 245, 470, 70, 10, HX8357_GREEN);

  tft.setCursor(70,  19);
  tft.print("1");
  tft.setCursor(230, 19);
  tft.print("2");
  tft.setCursor(390, 19);
  tft.print("3");

  tft.setCursor(70,  79);
  tft.print("4");
  tft.setCursor(230, 79);
  tft.print("5");
  tft.setCursor(390, 79);
  tft.print("6");

  tft.setCursor(70,  139);
  tft.print("7");
  tft.setCursor(230, 139);
  tft.print("8");
  tft.setCursor(390, 139);
  tft.print("9");

  tft.setCursor(20,  199);
  tft.print("Clear");
  tft.setCursor(230, 199);
  tft.print("0");
  tft.setCursor(365, 199);
  tft.setTextColor(HX8357_BLUE);
  tft.print("Set");

  tft.setCursor(15,265);
  if (Liter == false)
  {
    tft.print("Gallons");
  }
  else
  {
    tft.print("Liters");
  }

}

void printValveButton()
{
  if (onMainScreen == true && (!(oldValveOpen == valveOpen)))
  {
    tft.setTextSize(4);
    
    if (valveOpen == true)
    {
      tft.fillRoundRect(5,   185, 150, 130, 10,HX8357_GREEN); 
      tft.drawRoundRect(5,   185, 150, 130, 10,HX8357_RED);
      tft.setTextColor(HX8357_RED); 
      tft.setCursor(23, 215);
      tft.print("Valve");
      tft.setCursor(10, 260);
      tft.print(" OPEN ");
    }
    else
    {
      tft.fillRoundRect(5,   185, 150, 130, 10,HX8357_RED); 
      tft.drawRoundRect(5,   185, 150, 130, 10,HX8357_GREEN); 
      tft.setTextColor(HX8357_GREEN); 
      tft.setCursor(23, 215);
      tft.print("Valve");
      tft.setCursor(10, 260);
      tft.print("CLOSED");
    }
  }

  oldValveOpen = valveOpen;

}

void printGalLitButton()
{
  tft.fillRoundRect(325, 185, 150, 130, 10,HX8357_WHITE); 
  tft.drawRoundRect(325, 185, 150, 130, 10,HX8357_RED);
  tft.setTextSize(3);
  tft.setTextColor(HX8357_RED);   

  if (Liter == true)
  {
    tft.setCursor(350, 240);
    tft.print("Liters");
  }
  else
  {
    tft.setCursor(340, 240);
    tft.print("Gallons");
  }
  
}

void printAutoManualButton()
{
  tft.fillRoundRect(165, 185, 150, 130, 10,HX8357_BLUE); 
  tft.drawRoundRect(165, 185, 150, 130, 10,HX8357_WHITE); 

  tft.setTextSize(4);
  tft.setTextColor(HX8357_WHITE);

  if (manualOverride == true)
  {
    tft.setCursor(172, 240);
    tft.print("MANUAL");
  }
  else
  {
    tft.setCursor(193, 240);
    tft.print("Auto");
  }
  
  
}

void blankOutFlowRate()
{
    tft.fillRect(190, 8, 148, 46, HX8357_BLACK);
}

void blankOutTotalA()
{
    tft.fillRect(120, 68, 148, 46, HX8357_BLACK);
}

void blankOutSetScreenSetAmount()
{
    tft.fillRect(190, 250, 270, 60, HX8357_BLACK);
}

void blankOutMainScreenSetAmount()
{
    tft.fillRect(135, 128, 140, 46, HX8357_BLACK);
}

void openValve()
{
  digitalWrite(solenoidPin, HIGH);
  valveOpen = true;
  printValveButton();
}

void closeValve()
{
  digitalWrite(solenoidPin, LOW);
  valveOpen = false;
  printValveButton();
}

void resetButton()
{
  totalA = 0;
  printTotalA();
  setAmount = 0;
  printSetAmountMainScreen();
  closeValve();
}

void setButtonMainScreen()
{
  closeValve();
  totalA = 0;
  setAmount = 0;
  drawSetScreen();
}

void setButtonSetScreen()
{
  drawMainScreen();
  if (setAmount > totalA)
  {
    openValve();
  }

}

void gallonLiterToggleButton()
{
  if (Liter == true)
  {
    Liter = false;
    setAmount = (setAmount * 0.264172);
  }
  else
  {
    Liter = true;
    setAmount = (setAmount * 3.78541);
  }
  //We have to change oldValveOpen to have the printValveButton create the button
  oldValveOpen = !oldValveOpen;
  
  //We want to draw the whole screen as a lot of the text will change
  drawMainScreen();

}

void AutoManualButton()
{
  if (valveOpen == false)
  {
    openValve();
    manualOverride = true;
  }
  else
  {
    closeValve();
    manualOverride = false;
  }
  printAutoManualButton();
}

void toggleValve()
{
  if ((onMainScreen == true) && (setAmount >= totalADisplay) && (setAmount > 0) && (manualOverride == false))
  {
    openValve();
  }
  else if (manualOverride == true)
  {
    openValve();
  }
  else if (manualOverride == false)
  {
    closeValve();
  }
}

void printFlowRate()
{
  blankOutFlowRate();
  tft.setTextSize(3);
  tft.setTextColor(HX8357_MAGENTA); 
  tft.setCursor(220, 22);
  tft.print(flowRateDisplay);
}

void printTotalA()
{
  blankOutTotalA();
  tft.setTextSize(3);
  tft.setTextColor(HX8357_MAGENTA); 
  tft.setCursor(150, 82);
  tft.print(totalADisplay);
}

void printSetAmountMainScreen()
{
  blankOutMainScreenSetAmount();
  tft.setTextSize(3);
  tft.setTextColor(HX8357_MAGENTA); 
  tft.setCursor(150, 142);
  tft.print(setAmount);
}

void printSetAmountSetScreen()
{
  blankOutSetScreenSetAmount();
  tft.setTextSize(4);
  tft.setTextColor(HX8357_MAGENTA); 
  tft.setCursor(220, 265);
  tft.print(setAmount);
}

void setNumberButton(float i)
{
    float newSetAmount = (setAmount * 10);
    newSetAmount += (i/100);

    if (newSetAmount < 1000)
    {
      setAmount = newSetAmount;
    }

  printSetAmountSetScreen();
}

void clearButton()
{
  setAmount = 0.00;
  printSetAmountSetScreen();
}

