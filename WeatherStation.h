/*
 Weather Station using the Electric Imp
 By: Nathan Seidle
 SparkFun Electronics
 Date: October 4th, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
*/

#ifndef __WEATHER_STATION_H__
#define __WEATHER_STATION_H__

#if defined(ARDUINO) && ARDUINO >= 100
        #include "Arduino.h"
#else
        #include "WProgram.h"
#endif

#include <stdint.h>

#define WIND_DIR_AVG_SIZE 120

class WeatherStation
{
private:
  int32_t lastSecond; //The millis counter to see when a second rolls by
  uint16_t minutesSinceLastReset; //Used to reset variables after 24 hours. Imp should tell us when it's midnight, this is backup.
  uint8_t seconds; //When it hits 60, increase the current minute
  uint8_t seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
  uint8_t minutes; //Keeps track of where we are in various arrays of data
  uint8_t minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data
  
  int32_t lastWindCheck = 0;
  volatile int32_t lastWindIRQ = 0;
  volatile uint8_t windClicks = 0;
  
  //We need to keep track of the following variables:
  //Wind speed/dir each update (no storage)
  //Wind gust/dir over the day (no storage)
  //Wind speed/dir, avg over 2 minutes (store 1 per second)
  //Wind gust/dir over last 10 minutes (store 1 per minute)
  //Rain over the past hour (store 1 per minute)
  //Total rain over date (store one per day)
  uint8_t windspdavg[120]; //120 bytes to keep track of 2 minute average
  int16_t winddiravg[WIND_DIR_AVG_SIZE]; //120 ints to keep track of 2 minute average
  float windgust_10m[10]; //10 floats to keep track of largest gust in the last 10 minutes
  int16_t windgustdirection_10m[10]; //10 ints to keep track of 10 minute max
  volatile float rainHour[60]; //60 floating numbers to keep track of 60 minutes of rain
  
  //These are all the weather values that wunderground expects:
  int16_t winddir; // [0-360 instantaneous wind direction]
  float windspeedmph; // [mph instantaneous wind speed]
  float windgustmph; // [mph current wind gust, using software specific time period]
  int16_t windgustdir; // [0-360 using software specific time period]
  float windgustmph_10m; // [mph past 10 minutes wind gust mph ]
  int16_t windgustdir_10m; // [0-360 past 10 minutes wind gust direction]
  volatile float dailyrainin; // [rain inches so far today in local time]
  
  // volatiles are subject to modification by IRQs
  volatile uint32_t rainlast;
  
  //pins
  int16_t (*WDIR)();
public:
  WeatherStation(int16_t (*_wdir)()); 
  void rainIRQ();
  void wspeedIRQ();
  void setup();
  void update();
  void displayArrays();
  void resetVars();
  void calcWeather();

  int16_t getWindDir();
  float getWindSpeedMPH();
  float getWindGustMPH();
  int16_t getWindGustDir();
  float getWindSpeedMPH_Avg2m();
  int16_t getWindDir_Avg2m();
  float getWindGustMPH_10m();
  int16_t getWindGustDir_10m();
  float getRainIn();
  float getDailyRainIn();
  
private:
  float get_wind_speed();
  int16_t get_wind_direction();
  int16_t averageAnalogRead();
};

#endif // __RF24_H__

