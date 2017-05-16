/*
  Pump control sketch. Turns pump on for 15 minutes per hour. Pump comes on at 7 am and goes off at 7 pm
*/

#include "..\wire\wire.h"
#include <SPI.h>
#include <RTClib.h>
#include <RTC_DS3234.h>
#include <Time.h>

// Create an RTC instance, using the chip select pin it's connected to
RTC_DS3234 RTC(8);

// Pump
int relay = 2;    // Relay pin
int state = LOW;  // Initial relay state - off

// HC-SR04 Ultrasonic device pin settings
int trig = 6; // attach pin 6 to Trig
int echo = 7; //attach pin 7 to Echo

int idle = 2700;    // 45 minutes
int run = 900;      // 15 minutes

//#define TEST
#define ENABLE_PING

unsigned long TogglePumpTime;
unsigned long CurrentUnixTime;

// Start time:  07:00
int sHour = 7;
int sMin = 00;

// Off time:  19:00
int eHour = 19;
int eMin = 00;

// Time variables
time_t turn_on;
time_t turn_off;

void setup () 
{
    Serial.begin(9600);
  
    Serial.println("");
    Serial.println("Initializing....");
  
    pinMode(relay, OUTPUT);
    digitalWrite(relay, state);

#ifdef TEST
    idle = 30;
    run = 30;
    sHour = 7;
    eHour = 19;
    Serial.println("Running in test mode");
#endif

    SPI.begin();
    SPI.setDataMode(SPI_MODE3);
    RTC.begin();

    // Notify if the RTC isn't running
    if (! RTC.isrunning()) 
    {
        Serial.println("RTC is NOT running");
        ;
    }
  
    // Initialize values
    TogglePumpTime = 0;
    CurrentUnixTime = 0;

    // Get time from RTC
    DateTime CurrentTime = RTC.now();
    DateTime compiled = DateTime(__DATE__, __TIME__);

    if (CurrentTime.unixtime() < compiled.unixtime()) 
    {
        Serial.println("RTC is older than compile time! Updating");
        RTC.adjust(DateTime(__DATE__, __TIME__));    
    }

    // Use the RTC time to set the start time
    setTime(sHour, sMin, 00, 0, 0, 0);
    turn_on = now();

    // Use RTC time to set the end time
    setTime(eHour, eMin, 00, 0, 0, 0);
    turn_off = now();
  
    // Reset system time to now
    setTime(CurrentTime.hour(), CurrentTime.minute(), CurrentTime.second(), CurrentTime.day(), CurrentTime.month(), CurrentTime.year());  
  
    Serial.println("Setup complete.");
    printTime();  
}

void loop()
{

    DateTime CurrentTime = RTC.now();

    // Reset system time to now
    setTime(CurrentTime.hour(), CurrentTime.minute(), CurrentTime.second(), 0, 0, 0);    
    time_t timenow = now();

    // Toggle the pump on for 15 minutes only if the current time is within our daily turn on time value
    if (timenow >= turn_on && timenow < turn_off) 
    {
#ifdef    ENABLE_PING
        long maxdepth = 24;  // depth of water in inches
        long depth = GetDepth(maxdepth);
        if (depth > maxdepth)
        {
            // Turn the pump off if it's on 
            if (state == HIGH)
            {
                Serial.println("Exceeded maxdepth or below minimum range");
                TurnPumpOff();
            }
            delay(1000);
            return; // skip rest of processing
        }
#endif
        CurrentUnixTime = CurrentTime.unixtime();
        if (CurrentUnixTime > TogglePumpTime) 
        {
            printTime();                                            // For debugging purposes
            if (state == LOW) 
            {                             // Turn on
                TogglePumpTime = CurrentUnixTime + run;
                TurnPumpOn();
            } 
            else 
            {                                                    // Turn off
                TogglePumpTime = CurrentUnixTime + idle;
                TurnPumpOff();
            }
        }
    } 
    else if (state == HIGH) // pump is currently on
    {
         TurnPumpOff();
    }
    else
    {
        TogglePumpTime = 0;                     // reset for tomorrow morning
    }

    delay(1000);

}

void TurnPumpOn()
{
     state = HIGH;
     digitalWrite(relay, state);             // Toggle relay
     Serial.println("Turning pump on.");

}

void TurnPumpOff()
{
     state = LOW;
     digitalWrite(relay, state);             // Toggle relay
     Serial.println("Turning pump off.");
}

void printTime() 
{
    const int len = 32;
    static char buf[len];

    DateTime now = RTC.now();

    Serial.println(now.toString(buf,len));
}

long GetDepth(long maxdepth)
{
    // establish variables for duration of the ping,
    // and the distance result in inches and centimeters:
    long duration, inches, cm;

    // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
    pinMode(trig, OUTPUT);
    digitalWrite(trig, LOW);
    delayMicroseconds(2);
    digitalWrite(trig, HIGH);
    delayMicroseconds(5);
    digitalWrite(trig, LOW);

    // The same pin is used to read the signal from the PING))): a HIGH
    // pulse whose duration is the time (in microseconds) from the sending
    // of the ping to the reception of its echo off of an object.
    pinMode(echo, INPUT);
    duration = pulseIn(echo, HIGH);

    // convert the time into a distance
    inches = microsecondsToInches(duration);
    cm = microsecondsToCentimeters(duration);

    if (inches > maxdepth)
    {
        Serial.println("Exceeded maxdepth");
    }

#ifdef _DEBUG
    Serial.print(inches);
    Serial.print("in, ");
    Serial.print(cm);
    Serial.print("cm");
    Serial.println();
#endif

    return inches;
}

long microsecondsToInches(long microseconds)
{
    // According to Parallax's datasheet for the PING))), there are
    // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
    // second). This gives the distance travelled by the ping, outbound
    // and return, so we divide by 2 to get the distance of the obstacle.
    // See: http://www.parallax.com/dl/docs/prod/acc/28015-PI...
    return microseconds / 74 / 2;
}

long microsecondsToCentimeters(long microseconds)
{
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the
    // object we take half of the distance travelled.
    return microseconds / 29 / 2;
}



