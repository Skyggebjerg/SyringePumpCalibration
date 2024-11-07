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

uint64_t microLiterPerHour = 16; //delay between runs
uint64_t tempus;
int vandring = 16; // how fast to run motor
int temp_vandring = 700;
int thirdVar;
bool newpress = true; // monitor if button just pressed 
bool broken = false; // pump interrupted or not
int mstatus = 0; // defines which state the system is in
float conversion = 60; // as 1 rpm = 1 µl/min, then we need to convert from ml/h to µl/min and then to rpm
int calculated_hastighed;

signed short int last_value = 0;
signed short int last_btn = 1;


int motor_steps = 13; //Determines the number of steps per revolution for our motor. In reality it is 200steps/rev but is here set to 13 so we cheat the system, so 1 rpm = 1 µl/min
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
    String html = "<html><body style=\"font-size: 24px;\">"; // Increased font size
    html += "<meta name=\"viewport\" content=\"width=390, initial-scale=1\"/>";
    html += "<h1 style=\"font-size: 28px;\">Chemostat feed 01</h1>"; // Increased font size
    html += "<form action=\"/update\" method=\"POST\" style=\"display: flex; flex-direction: column; gap: 20px;\">"; // Flexbox for vertical alignment and gap for spacing
    html += "<label>vandring: <input type=\"text\" name=\"vandring\" value=\"" + String(vandring) + "\" style=\"font-size: 24px; width: 100px;\" required pattern=\"\\d{1,5}\" title=\"Please enter a valid number (up to 5 digits)\"></label>"; // Shorter input field with max 5 digits
    html += "<label>microLiterPerHour: <input type=\"text\" name=\"microLiterPerHour\" value=\"" + String(microLiterPerHour) + "\" style=\"font-size: 24px; width: 100px;\" required pattern=\"\\d{1,5}\" title=\"Please enter a valid number (up to 5 digits)\"></label>"; // Shorter input field with max 5 digits
    html += "<label style=\"margin-top: 24px;\">Run mode: <input type=\"checkbox\" name=\"thirdVar\" value=\"1\" " + String(thirdVar == 1 ? "checked" : "") + "> Loop</label>"; // Increased margin for checkbox
    html += "<input type=\"submit\" value=\"Save\" style=\"font-size: 24px;\">"; // Increased font size
    html += "</form>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleUpdate() {
    if (server.hasArg("vandring") && server.hasArg("microLiterPerHour")) {
        vandring = server.arg("vandring").toInt();
        microLiterPerHour = server.arg("microLiterPerHour").toInt();
        calculated_hastighed = static_cast<int>(ceil(static_cast<float>(microLiterPerHour) / conversion));
        thirdVar = server.hasArg("thirdVar") ? 1 : 0;
        newpress = true; 
        if (thirdVar == 0) {
            //AtomS3.Display.clear();
            //AtomS3.Display.drawString("Off", 5, 0);
            mstatus = 0;
        }
        EEPROM.put(0, vandring);
        EEPROM.put(sizeof(vandring), microLiterPerHour);
        EEPROM.put(sizeof(vandring) + sizeof(microLiterPerHour), thirdVar);
        EEPROM.commit();

        server.sendHeader("Location", "/", true);
        server.send(303);
    } else {
        server.send(400, "text/html", "<html><body><h1>Invalid Input</h1><a href=\"/\">Go Back</a></body></html>");
    }
}

void setup()
{
    Serial.begin(115200);
    EEPROM.begin(512);
    EEPROM.get(0, vandring);
    EEPROM.get(sizeof(vandring), microLiterPerHour);
    EEPROM.get(sizeof(vandring) + sizeof(microLiterPerHour), thirdVar);
    calculated_hastighed = static_cast<int>(ceil(static_cast<float>(microLiterPerHour) / conversion));
    temp_vandring = vandring;
    WiFi.softAP(ssid, password);
    Serial.println("Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP()); //

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
    if (AtomS3.BtnA.wasDoubleClicked()) { // Double click to move piston in
            ss.powerEnable(true);
            ss.setSpeed(1000);
            ss.step(100);
            ss.powerEnable(false);
            broken = false; // reset broken status so aspirate will run a full cycle
        }

    if (AtomS3.BtnA.wasHold()) { // Hold button to start run mode
            mstatus = mstatus +1;
            if(mstatus == 2) mstatus = 0; // go back to base state
            AtomS3.Display.clear();
            AtomS3.Display.drawString(String(mstatus), 10, 100);
            newpress = true;
        }
   
   switch (mstatus) {

         case 0: //wait and do nothing until button is pressed
        {
            if (newpress) {
                AtomS3.Display.clear();
                AtomS3.Display.drawString("Waiting", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(microLiterPerHour), 10, 60);
                AtomS3.Display.drawString(String(calculated_hastighed), 10, 100);
                newpress = false;
            }
            if (millis() - tempus >= microLiterPerHour) // to be set by adjustment (100)
            {
                AtomS3.Display.drawString("Waiting", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(microLiterPerHour), 10, 60);
                tempus = millis();
            }
            break;
        } // end of case 0


        case 1: //run motor
        { 
            if (newpress) {
                AtomS3.Display.clear();
                AtomS3.Display.drawString("Running", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(microLiterPerHour), 10, 60);
                AtomS3.Display.drawString(String(calculated_hastighed), 10, 100);
                newpress = false;
            }
                AtomS3.Display.drawString("Running", 5, 0);
                AtomS3.Display.drawString(String(vandring), 10, 30);
                AtomS3.Display.drawString(String(microLiterPerHour), 10, 60);
                AtomS3.Display.drawString(String(calculated_hastighed), 10, 100);

            while(digitalRead(1) == HIGH) { // Wait for Valve signal OK
                delay(10);
            }  


                // ****** Aspirate ********

            ss.powerEnable(true);
            ss.setSpeed(9000); // 600 x 200/13 = 9230 and then round down to 9000
            if (broken) {
                ss.setSpeed(1000); // only aspirate slowly back if interrupted
                ss.step(-temp_vandring);
                broken = false;
            }
            else {
                ss.step(-700,150,150); //aspirate (1300 steps = 6.5 revs = 100 µl)
            }
            delay(100);
            digitalWrite(2, HIGH); // Tell Valve to change
            delay(100); //extra waiting for valve to finish switching

            while(digitalRead(1) == LOW) { // Wait for Valve signal OK
                delay(10);
            }   


                // ****** Dispense ********      

            delay(100); //extra waiting for valve to finish switching 
            ss.setSpeed(calculated_hastighed); // 
            for (int i = 1; i < 8;) {

                server.handleClient();
                AtomS3.update();
                //temp_vandring = 100 * i;
                if (AtomS3.BtnA.wasPressed()) {
                    mstatus = 0;
                    AtomS3.Display.clear();
                    AtomS3.Display.drawString(String(temp_vandring), 10, 30);
                    //temp_vandring = 100 * i;
                    delay(1000);

                    newpress = true;                    
                    broken = true; 
                    break;
                    //break;
                }
                else {
                    ss.step(100);
                    temp_vandring = 100 * i;
                    //AtomS3.Display.clear();
                    //AtomS3.Display.drawString(String(temp_vandring), 10, 30);
                    i++;
                }
                
            }
            delay(100);
            digitalWrite(2, LOW); // Tell Valve to change back 
            ss.powerEnable(false);
            delay(100); //extra waiting for valve to finish switching
            tempus = millis();
            break;
        } // end of case 1
    }   // end of switch
}