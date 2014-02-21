/*
 * @Author:  Ankush Agrawal, Joshua Anatalio, Elle Nguyen-Khoa, Christopher Graves, Albert Xu
 * Description: This code reads input from an arduino micrphone and performs an FFT analysis.
 *              Then, it plots the output onto an 8x8 LED matrix. The x-axis refers to a range of
 *              frequencies, and the y-axis refers to the loudness.
 *              The software can be thought of as an audio spectrum analyzer.
 * Specific note about our implementation of the loop  - You will notice that there is never a single
 * row of lights on at a time. This is because we implemented what we call a friend algorithm. The
 * code checks to see the length of the height of adjacent columns and adjusts the height of its column
 * accordingly. The reason for this is that the desired output turns out more visually appealing.
 *
 *References: We set up up the hardware and some of the FFT analysis using the following instructable tutorial:
 *               http://www.instructables.com/id/Arduino-Audio-Input/ 
 */

//clipping indicator variables
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//sstorage for timing of events
int slope[10];//storage for slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//storage for period of wave
byte index = 0;//current storage index
float frequency;//storage for frequency calculations
int maxSlope = 0;//used to calculate max slope as trigger point
int newSlope;//storage for incoming slope data
int refreshCount;

//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 3;//slope tolerance- adjust this if you need
int timerTol = 10;//timer tolerance- adjust this if you need
int freqRange = 600;//Variable used to control the size of each freqeuency band


//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

// 2-dimensional array of row pin numbers:
const int row[8] = {
  2,7,19,5,13,18,12,16};

// 2-dimensional array of column pin numbers:
const int col[8] = {
  6,11,10,3,17,4,8,9};

// 2-dimensional array of pixels:
int pixels[8][8];           

// cursor position:
int x = 5;
int y = 5;


void setup()
{

  Serial.begin(9600);

  cli();//diable interrupts

  //set up continuous sampling of analog pin 0 at 38.5kHz

  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements

  sei();//enable interrupts

  // initialize the I/O pins as outputs
  // iterate over the pins:
  for (int thisPin = 0; thisPin < 8; thisPin++)
  {
    // initialize the output pins:
    pinMode(col[thisPin], OUTPUT); 
    pinMode(row[thisPin], OUTPUT);  
    digitalWrite(col[thisPin], HIGH);
  }
}

ISR(ADC_vect) 
{//when new ADC value ready

  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol)
    {
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0)
      {
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol)
      {  //if timer duration and slopes match
        //sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++)
          totalTimer+=timer[i];
        period = totalTimer;//set period
        //reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope)
    {//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else
    {//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9)
        reset();
    }
  }

  if (newData == 0 || newData == 1023){
    clipping = 1;
  }

  time++;//increment timer at rate of 38.5kHz

  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp)
    maxAmp = abs(127-ADCH);
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }

}

void reset(){//clean out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


void checkClipping()
{//manage clipping indicator LED
  if (clipping)
    clipping = 0;
}


void loop(){
  /*Serial.println("Analog Read: " + analogRead(A0));
   Serial.println("print me");
   */
  checkClipping();
  resetPixels();
  if (checkMaxAmp>ampThreshold){
    frequency = 38462/float(period);//calculate frequency timer rate/period
    Serial.println(maxSlope);
    //print results
    Serial.print(frequency);
    Serial.println(" hz");
  }
  // read input:
  //readSensors();
  int maxSlopeThreshold = maxSlope/3;
  if(maxSlopeThreshold > 8)
    maxSlopeThreshold = 8;

  if(frequency < freqRange*.75)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[0][i] = LOW; 
      pixels[1][i/2] = LOW;
    } 
  } 

  else if(frequency < freqRange*1.5)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[0][i/2] = LOW; 
      pixels[1][i] = LOW;
      pixels[2][i/2-1] = LOW;
    } 

  }
  else if(frequency < freqRange*3)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[1][i/2-1] = LOW; 
      pixels[2][i] = LOW;
      pixels[3][i/2] = LOW;
    } 

  }
  else if(frequency < freqRange*3.75)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[2][i/2] = LOW; 
      pixels[3][i] = LOW;
      pixels[4][i/2-1] = LOW;
    } 

  }
  else if(frequency < freqRange*5)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[3][i/2+1] = LOW; 
      pixels[4][i] = LOW;
      pixels[5][i/2] = LOW;
    }
  }
  else if(frequency < freqRange*6)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[4][i/2-1] = LOW; 
      pixels[5][i] = LOW;
      pixels[6][i/2] = LOW;
    }
  }
  else if(frequency < freqRange*7)
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[5][i/2] = LOW; 
      pixels[6][i] = LOW;
      pixels[7][i/2+1] = LOW;
    }
  }
  else 
  {
    for(int i = 0; i < maxSlopeThreshold; i++)
    {
      pixels[6][i/2] = LOW;
      pixels[7][i] = LOW;
    }
  }

  refreshCount++;
  // draw the screen:

  delay(1);
  refreshScreen();
}

void resetPixels() {
  for(int r = 0; r < 8; r++)
    for(int c = 0; c < 8; c++)
      pixels[r][c] = HIGH;
}  

void refreshScreen() {

  // iterate over the rows (anodes):
  for (int thisRow = 0; thisRow < 8; thisRow++) {
    // take the row pin (anode) high:
    digitalWrite(row[thisRow], HIGH);
    // iterate over the cols (cathodes):
    for (int thisCol = 0; thisCol < 7; thisCol++) 
    {
      // get the state of the current pixel;
      int thisPixel = pixels[thisRow][thisCol];
      // when the row is HIGH and the col is LOW,
      // the LED where they meet turns on:
      digitalWrite(col[thisCol], thisPixel);
      // turn the pixel off:
      if (thisPixel == LOW)
        digitalWrite(col[thisCol], HIGH);
    }
    // take the row pin low to turn off the whole row:
    digitalWrite(row[thisRow], LOW);
    for(int mcol = 7; mcol < 8; mcol++)
      digitalWrite(col[mcol], LOW);
    for(int mcol = 0; mcol < 1; mcol++)
      digitalWrite(col[mcol], LOW);     
  }
}



