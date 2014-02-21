/*
 * @Author:  Ankush Agrawal, Joshua Anatalio, Elle Nguyen-Khoa, Christopher Graves, Albert Xu
 * Description: This code reads input from an arduino micrphone and performs an FFT analysis.
 *              Then, it plots the output onto an LED strip in vaying colors. The lights are split into
 *              five groups based in frequency and the brightnes is determined by volume.
 *              The software can be thought of as an audio spectrum analyzer.
 *
 *References:
 *
 */

#include <Adafruit_NeoPixel.h>
#define PIN 6 
//clipping indicator variables 
boolean clipping = 0;
int maxBirghtness = 255;
int maxColor = 255;
int minBrightness = 125;
//data storage variables 
byte newData = 0; 
byte prevData = 0; 
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally 
int timer[10];//storage for timing of events 
int slope[10];//storage for slope of events 
unsigned int totalTimer;//used to calculate period 
unsigned int period;//storage for period of wave 
byte index = 0;//current storage index 
float frequency;//storage for frequency calculations 
int maxSlope = 0;//used to calculate max slope as trigger point int newSlope;
//storage for incoming slope data 
int refreshCount;  
int color =0;
int loopCount = 0; // counts iterations through loop method
byte noMatch = 0;//counts how many non­matches you've received to reset variables if it's been too long 
byte slopeTol = 3;//slope tolerance­ adjust this if you need
int timerTol = 10;//timer tolerance­ adjust this if you need
//variables for amp detection 
unsigned int ampTimer = 0; 
byte maxAmp = 0; 
byte checkMaxAmp; 
byte ampThreshold = 30;//raise if you have a very noisy signal

// Parameter 1 = number of pixels in strip 
// Parameter 2 = pin number (most are valid) 
// Parameter 3 = pixel type flags, add together as needed: 
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs) 
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers) 
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products) 
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2) 
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

void setup()
{  
  strip.begin();
  strip.show(); 
  Serial.begin(9600);

  cli();//diable interrupts
  //set up continuous sampling of analog pin 0 at 38.5kHz
  //clear ADCSRA and ADCSRB registers  
  ADCSRA = 0;  ADCSRB = 0;
  ADMUX |= (1 << REFS0); //set reference voltage  
  ADMUX |= (1 << ADLAR); 
  //left align the ADC value­ so we can read highest 8 bits from ADCH register only
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler­ 16mHz/32=500kHz  
  ADCSRA |= (1 << ADATE); //enabble auto trigger  
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete  
  ADCSRA |= (1 << ADEN); //enable ADC  
  ADCSRA |= (1 << ADSC); //start ADC measurements
  sei();//enable interrupts
}//end setup()


ISR(ADC_vect) 
{
  prevData = newData;//store previous value 
  newData = ADCH;//get value from A0  
  if (prevData < 127 && newData >=127)
  {
    //if increasing and crossing midpoint    
    newSlope = newData ­ prevData;
    //calculate slope    
    if (abs(newSlope­maxSlope)<slopeTol)
    {
      //if slopes are ==      
      //record new data and reset time      
      slope[index] = newSlope;      
      timer[index] = time;      
      time = 0;      
      if (index == 0)
      {
        //new max slope just reset        
        noMatch = 0;        
        index++;//increment index      
      }      
      else if (abs(timer[0]­timer[index])<timerTol && abs(slope[0]­newSlope)<slopeTol)
      {
        //if timer duration and slopes match        
        //sum timer values        
        totalTimer = 0;        
        for (byte i=0;i<index;i++)
        {          
          totalTimer+=timer[i];        
        }        
        period = totalTimer;//set period        
        //reset new zero index values to compare with        
        timer[0] = timer[index];        
        slope[0] = slope[index];        
        index = 1;//set index to 1                
        noMatch = 0;      
      }      
      else
      {
        //crossing midpoint but not match        
        index++;//increment index        
        if (index > 9)
        {
          reset();
        }      
      }    
    }
    else if (newSlope>maxSlope)
    {
      //if new slope is much larger than max slope      
      maxSlope = newSlope;      
      time = 0;//reset clock      
      noMatch = 0;      
      index = 0;//reset index    
    }   
    else
    {
      //slope not steep enough      
      noMatch++;
      //increment no match counter      
      if (noMatch>9)
      {
        reset();
      }
    }
  }
  if (newData == 0 || newData == 1023)
  {
    //if clipping      
    clipping = 1;//currently clipping  
  }
  time++;//increment timer at rate of 38.5kHz
  ampTimer++;//increment amplitude timer  
  if (abs(127­ADCH)>maxAmp)
  {
    //change maxAmp to new maximum amplitude
    maxAmp = abs(127­ADCH);
  }
  if (ampTimer==1000)
  {
    //reset ampTimer
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;//reset maxAmp
  }
}//end ISR


void reset()
{
  //clear out some variables  
  index = 0;//reset index  
  noMatch = 0;//reset match couner   
  maxSlope = 0;//reset slope 
}//end reset()

void loop()
{ 
  if(clipping)
  { 
    digitalWrite(13, HIGH);
  }   
  strip.show();
  if (checkMaxAmp>ampThreshold)
  {    
    frequency = 38462/float(period);
    //calculate frequency timer rate/period    
    Serial.println(maxSlope);    
    //print results    
    Serial.print(frequency);
    Serial.println(" hz");
  }   
  int maxSlopeThreshold = maxSlope;  
  if(maxSlopeThreshold > maxBrightness)    
    maxSlopeThreshold = maxBrightness;
  if(maxSlopeThreshold < minBrightness)
    maxSlopeThreshold = maxSlopeThreshold*2.5;
  int freqRange = 100;
  if(loopCount%100 == 0)
  {   
    color+=5;
  }   
  loopCount++;
  if(frequency < freqRange)
  {    
    //middle six lights
    for(int i = 12; i < 18; i++)
    {     
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & maxColor));
    }    
    strip.setBrightness(maxSlopeThreshold);
    strip.show();
  }  
  else if(frequency < freqRange*2)
  {    
    //lights 9-12 and 18-21
    for(int i = 9; i < 12; i++)
    {     
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & maxColor));
      strip.setPixelColor(i+9, Wheel((((i+9) * 256 / strip.numPixels()) + color) & maxColor));  
    }
    //set the brighness and turn them on
    strip.setBrightness(maxSlopeThreshold);    
    strip.show();
  }
  else if(frequency < freqRange*3)
  {    
    for(int i = 6; i < 9; i++)    
    {     
      //lights 6-9 and 21-24
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & maxColor));     
      strip.setPixelColor(i+15, Wheel((((i+15) * 256 / strip.numPixels()) + color) & maxColor));    
    }
    strip.setBrightness(maxSlopeThreshold);
    strip.show();  
  }
  else if(frequency < freqRange*4)
  {    
    //lights 3-6 and 24-27
    for(int i = 3; i < 6; i++)
    {     
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & maxColor));     
      strip.setPixelColor(i+21, Wheel((((i+21) * 256 / strip.numPixels()) + color) & maxColor));    
    }
    //sets the brightness and displays the lights
    strip.setBrightness(maxSlopeThreshold);
    strip.show();  
  }    
  else if(frequency < freqRange*5) 
  {  
    //lights 0-3 and 27- 30
    for(int i = 0; i < 3; i++) 
    {   
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & maxColor));    
      strip.setPixelColor(i+27, Wheel((((i+27) * 256 / strip.numPixels()) + color) & maxColor)); 
    }
    //sets brightness and diaplays lights
    strip.setBrightness(maxSlopeThreshold);
    strip.show();  
  } 
  else
  {  
    //default display on middle lights
    for(int i = 12; i < 18; i++)
    {  
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + color) & 255));   
    } 
    strip.setBrightness(maxSlopeThreshold);   
    strip.show();
  }   
}//end loop()

uint32_t Wheel(byte WheelPos)
{  
  //sets color appropriateley
  if(WheelPos < 85)
  { 
    return strip.Color(WheelPos * 3, 255 ­ WheelPos * 3, 0);
  }
  else if(WheelPos < 170) 
  {  
    WheelPos ­= 85;
    return strip.Color(255 ­ WheelPos * 3, 0, WheelPos * 3); 
  } 
  else
  {  
    WheelPos ­= 170; 
    return strip.Color(0, WheelPos * 3, 255 ­ WheelPos * 3);  
  } 
}//end wheel()

void colorWipe(uint32_t c, uint8_t wait)
{  
  //wipes the color clean
  for(uint16_t i=0; i<strip.numPixels(); i++) 
  { 
    strip.setPixelColor(i, c);
    strip.show();
  } 
}//end colorWipe()
