// Stepmotor calibration for syringe pump in Pegasus

#include <Arduino.h>
#include "M5AtomS3.h"
#include <M5GFX.h>
#include "Unit_Encoder.h"
//#include <M5Atom.h>
#include "StepperDriver.h"
#include "FastLED.h"

uint64_t forsink = 16; //delay between runs
uint64_t tempus;
int ontime = 16; // how fast to run motor
bool newpress = true; // monitor if button just pressed 
int mstatus = 0; // defines which state the system is in

signed short int last_value = 0;
signed short int last_btn = 1;


int motor_steps = 200;
int step_divisition = 32; //32
int en_pin = 22;
int dir_pin = 23;
int step_pin = 19;
unsigned long startTime;
unsigned long elapsedTime;

int step = 0;
int speed = 0;

M5GFX display;
M5Canvas canvas(&display);
Unit_Encoder sensor;

StepperDriver ss(motor_steps, step_divisition, en_pin, dir_pin, step_pin);

void setup()
{
  //M5.begin(true, false, true);
  Wire.begin(2,1);
  auto cfg = M5.config();
  AtomS3.begin(cfg);
  sensor.begin();
  AtomS3.Display.setTextColor(WHITE);
  AtomS3.Display.setTextSize(3);
  AtomS3.Display.clear();
    //AtomS3.Display.print("HEJ");
  delay(2000);
  tempus = millis();
  
  //Serial.begin(115200);
  //pinMode(32, INPUT_PULLDOWN); // set pin mode to input
  //pinMode(26, OUTPUT);
  //digitalWrite(26, LOW);
  ss.powerEnable(false);
  ss.setSpeed(0);
  delay(1600);

}

void loop()
{
    AtomS3.update();
    bool btn_status                = sensor.getButtonStatus();
    //if (AtomS3.BtnA.wasPressed()) { // Change mode when click ATOMS3 button
    if (last_btn != btn_status) { // Change mode when click encoder - check if encoder is pressed down or up
        if (!btn_status) { // Only change mode when encoder go from high to low
            mstatus = mstatus +1;
            if(mstatus == 5) mstatus = 0; // go back to base state
            AtomS3.Display.clear();
            AtomS3.Display.drawString(String(mstatus), 10, 100);
            newpress = true;
        }
        last_btn = btn_status;
    }
   
   switch (mstatus) {

        case 0: //run motor
        { 
            if (newpress) {
                AtomS3.Display.drawString("Running", 5, 0);
                newpress = false;
            }
           // if (millis() - tempus >= forsink) // to be set by adjustment (100)
           // {
                AtomS3.Display.drawString("Running", 5, 0);
                AtomS3.Display.drawString(String(ontime), 10, 30);
                AtomS3.Display.drawString(String(forsink), 10, 60);

      ss.powerEnable(true);
      ss.setSpeed(600); // 60 = 60 revolutions per minute (rpm) = 1 rev per sec (There are 6.5 revs per 100 µl)
      //Serial.println("AspirateStart:-750");
      ss.step(-750,100,100); //aspirate (1300 steps = 6.5 revs = 100 µl)
      //Serial.println("AspirateEnd:-750");
      delay(100);
      //digitalWrite(26, HIGH); // Tell Valve to change
      
      //while(digitalRead(32) == LOW) { // Wait for Valve signal OK
       // delay(10);
//}     //ss.setSpeed(300);
delay(1000); //simulate waiting for valve

      //ss.setSpeed(5); // 5 revs/min = 300 revs/h = 46.15 full syringe strokes (300/6.5) = 4615 µl per hour (46 x 100µl) 
      //ss.setSpeed(1); // 1 revs/min = 60 revs/h = 9.23 full syringe strokes (60/6.5) = 923 µl per hour (9.23 x 100µl)
      
      //ss.setSpeed(300); // 300 = superfast = purge and ethanol cleaning
      ss.setSpeed(16); // 16 = fast = 12.7 ml/h
      //ss.setSpeed(8); // 8 = slow = 6,4 ml/h
      
      //ss.setSpeed(3); // 3 revs/min = 2.77 ml/h (3 x 923 µl)   
      //Serial.println("DispenseStart:750");
      ss.step(750); //dispense
      //Serial.println("DispenseEnd:750");
      delay(100);
      //digitalWrite(26, LOW); // Tell Valve to change back 
      ss.powerEnable(false);


                //driver.setDriverDirection(HBRIDGE_FORWARD); // Set peristaltic pump in forward to take out BR content
                //driver.setDriverDirection(HBRIDGE_BACKWARD)
                //driver.setDriverSpeed8Bits(127); //Run pump in half speed
                //delay(ontime); // to be set by adjustment (30)
                //driver.setDriverDirection(HBRIDGE_STOP);
                //driver.setDriverSpeed8Bits(0);  //Stop pump
                tempus = millis();
            
            break;
        } // end of case 0
   
   
   }
         
        // M5.update();
}