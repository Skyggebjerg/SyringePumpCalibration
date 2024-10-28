// Stepmotor calibration for syringe pump in Pegasus

#include <Arduino.h>
#include "M5AtomS3.h"
#include <M5GFX.h>
//#include "Unit_Encoder.h"
//#include <M5Atom.h>
#include "StepperDriver.h"
#include "FastLED.h"

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

const char* ssid = "Chemostat feed 01";
const char* password = "12345678";

WebServer server(80);

uint64_t hastighed = 16; //delay between runs
uint64_t tempus;
int vandring = 16; // how fast to run motor
int thirdVar;
bool newpress = true; // monitor if button just pressed 
int mstatus = 0; // defines which state the system is in

signed short int last_value = 0;
signed short int last_btn = 1;


int motor_steps = 200;
int step_divisition = 32; //32
int en_pin = 5; //AtomS3 pin
int dir_pin = 7;
int step_pin = 6;
unsigned long startTime;
unsigned long elapsedTime;

int step = 0;
int speed = 0;

M5GFX display;
M5Canvas canvas(&display);
//Unit_Encoder sensor;

StepperDriver ss(motor_steps, step_divisition, en_pin, dir_pin, step_pin);

void handleRoot() {
    String html = "<html><body style=\"font-size: 18px;\">";
    html += "<meta name=\"viewport\" content=\"width=390, initial-scale=1\"/>";
    html += "<h1 style=\"font-size: 24px;\">Motor Control Settings</h1>";
    html += "<form action=\"/update\" method=\"POST\">";
    html += "vandring: <input type=\"text\" name=\"vandring\" value=\"" + String(vandring) + "\" style=\"font-size: 18px;\"><br>";
    html += "hastighed: <input type=\"text\" name=\"hastighed\" value=\"" + String(hastighed) + "\" style=\"font-size: 18px;\"><br>";
    html += "ThirdVar: <input type=\"radio\" name=\"thirdVar\" value=\"1\" " + String(thirdVar == 1 ? "checked" : "") + "> On ";
    html += "<input type=\"radio\" name=\"thirdVar\" value=\"0\" " + String(thirdVar == 0 ? "checked" : "") + "> Off<br>";
    html += "<input type=\"submit\" value=\"Save\" style=\"font-size: 18px;\">";
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleUpdate() {
    if (server.hasArg("vandring") && server.hasArg("hastighed") && server.hasArg("thirdVar")) {
        vandring = server.arg("vandring").toInt();
        hastighed = server.arg("hastighed").toInt();
        thirdVar = server.arg("thirdVar").toInt();
            if (thirdVar == 0) {
                AtomS3.Display.clear();
                AtomS3.Display.drawString("Off", 5, 0);
                mstatus = 0;
            }
        EEPROM.put(0, vandring);
        EEPROM.put(sizeof(vandring), hastighed);
        EEPROM.put(sizeof(vandring) + sizeof(hastighed), thirdVar);
        EEPROM.commit();
        
        server.send(200, "text/html", "<html><body><h1>Settings Saved</h1><a href=\"/\">Go Back</a></body></html>");
    } else {
        server.send(400, "text/html", "<html><body><h1>Invalid Input</h1><a href=\"/\">Go Back</a></body></html>");
    }
}

void setup()
{
    Serial.begin(115200);
    EEPROM.begin(512);
    EEPROM.get(0, vandring);
    EEPROM.get(sizeof(vandring), hastighed);
    EEPROM.get(sizeof(vandring) + sizeof(hastighed), thirdVar);
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.on("/update", HTTP_POST, handleUpdate);
    server.begin();
    auto cfg = M5.config();
    AtomS3.begin(cfg);
    //sensor.begin();
    AtomS3.Display.setTextColor(WHITE);
    AtomS3.Display.setTextSize(3);
    AtomS3.Display.clear();
    //AtomS3.Display.print("HEJ");
    delay(100);
    tempus = millis();
  
    //Serial.begin(115200);
    pinMode(1, INPUT_PULLDOWN); // set pin mode to input
    pinMode(2, OUTPUT);
    digitalWrite(26, LOW);
    ss.powerEnable(false);
    ss.setSpeed(0);
    delay(100);

}

void loop()
{
    server.handleClient();

    AtomS3.update();
    //bool btn_status                = sensor.getButtonStatus();
    if (AtomS3.BtnA.wasPressed()) { // Change mode when click ATOMS3 button
        //if (last_btn != btn_status) { // Change mode when click encoder - check if encoder is pressed down or up
        //if (!btn_status) { // Only change mode when encoder go from high to low
            mstatus = mstatus +1;
            if(mstatus == 5) mstatus = 0; // go back to base state
            AtomS3.Display.clear();
            AtomS3.Display.drawString(String(mstatus), 10, 100);
            newpress = true;
        }
        //last_btn = btn_status;
    //}
   
   switch (mstatus) {

         case 0: //wait and do nothing until button is pressed
        {
            if (newpress) {
                AtomS3.Display.drawString("Waiting", 5, 0);
                newpress = false;
            }
            if (millis() - tempus >= hastighed) // to be set by adjustment (100)
            {
                AtomS3.Display.drawString("Waiting", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(hastighed), 10, 60);
                tempus = millis();
            }
            break;
        } // end of case 1


        case 1: //run motor
        { 
            if (newpress) {
                AtomS3.Display.drawString("Running", 5, 0);
                newpress = false;
            }
           // if (millis() - tempus >= hastighed) // to be set by adjustment (100)
           // {
                AtomS3.Display.drawString("Running", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(hastighed), 10, 60);

            while(digitalRead(1) == HIGH) { // Wait for Valve signal OK
                delay(10);
            }     //ss.setSpeed(300);

                // ****** Aspirate ********
            ss.powerEnable(true);
            ss.setSpeed(600); // 60 = 60 revolutions per minute (rpm) = 1 rev per sec (There are 6.5 revs per 100 µl)

            ss.step(-750,100,100); //aspirate (1300 steps = 6.5 revs = 100 µl)
            delay(100);
            digitalWrite(2, HIGH); // Tell Valve to change
            delay(100); //extra waiting for valve to finish switching


            while(digitalRead(1) == LOW) { // Wait for Valve signal OK
                delay(10);
            }   

                // ****** Dispense ********      
            delay(100); //extra waiting for valve to finish switching 
            ss.setSpeed(hastighed); // 16 = fast = 12.7 ml/h
            ss.step(750); //dispense
            delay(100);
            digitalWrite(2, LOW); // Tell Valve to change back 
            ss.powerEnable(false);
            delay(100); //extra waiting for valve to finish switching

            tempus = millis();
            
            break;
        } // end of case 0
    }
}