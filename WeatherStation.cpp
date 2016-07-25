/*
 Weather Station using the Electric Imp
 By: Nathan Seidle
 SparkFun Electronics
 Date: October 4th, 2013
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 */

#include "WeatherStation.h"

/****************************************************************************/

WeatherStation::WeatherStation(int16_t (*_wdir)()) {
	WDIR = _wdir;
}

void WeatherStation::setup() {
	resetVars(); //Reset rain totals

	seconds = 0;
	lastSecond = millis();
}

void WeatherStation::rainIRQ() {
// Count rain gauge bucket tips as they occur
// Activated by the magnet and reed switch in the rain gauge, attached to input D2
  if (millis() - rainlast > 10) { // ignore switch-bounce glitches less than 10mS after initial edge
    dailyrainin += 0.011; //Each dump is 0.011" of water
    rainHour[minutes] += 0.011; //Increase this minute's amount of rain

    rainlast = millis(); // set up for next event
  }
}

void WeatherStation::wspeedIRQ() {
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
  if (millis() - lastWindIRQ > 10) { // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
    lastWindIRQ = millis(); //Grab the current time
    windClicks++; //There is 1.492MPH for each click per second.
  }
}

void WeatherStation::update() {
	//Keep track of which minute it is
	if(millis() - lastSecond >= 1000) {
		lastSecond += 1000;

		//Take a speed and direction reading every second for 2 minute average
		if(++seconds_2m > 119) seconds_2m = 0;

		//Calc the wind speed and direction every second for 120 second to get 2 minute average
		windspeedmph = get_wind_speed();
		winddir = WDIR();
		windspdavg[seconds_2m] = (int16_t)windspeedmph;
		winddiravg[seconds_2m] = winddir;
		//if(seconds_2m % 10 == 0) displayArrays();

		//Check to see if this is a gust for the minute
		if(windspeedmph > windgust_10m[minutes_10m]) {
			windgust_10m[minutes_10m] = windspeedmph;
			windgustdirection_10m[minutes_10m] = winddir;
		}

		//Check to see if this is a gust for the day
		//Resets at midnight each night
		if(windspeedmph > windgustmph) {
			windgustmph = windspeedmph;
			windgustdir = winddir;
		}

		//If we roll over 60 seconds then update the arrays for rain and windgust
		if(++seconds > 59) {
			seconds = 0;

			if(++minutes > 59) minutes = 0;
			if(++minutes_10m > 9) minutes_10m = 0;

			rainHour[minutes] = 0; //Zero out this minute's rainfall amount
			windgust_10m[minutes_10m] = 0; //Zero out this minute's gust

			minutesSinceLastReset++; //It's been another minute since last night's midnight reset
		}
	}
}

//Prints the various arrays for debugging
void WeatherStation::displayArrays() {
	//Windgusts in this hour
	Serial.println();
	Serial.print(minutes);
	Serial.print(":");
	Serial.println(seconds);

	Serial.print("Windgust last 10 minutes:");
	for(int16_t i = 0 ; i < 10 ; i++) {
		if(i % 10 == 0) Serial.println();
		Serial.print(" ");
		Serial.print(windgust_10m[i]);
	}

	//Wind speed avg for past 2 minutes
	/*Serial.println();
	 Serial.print("Wind 2 min avg:");
	 for(int16_t i = 0 ; i < 120 ; i++)
	 {
	 if(i % 30 == 0) Serial.println();
	 Serial.print(" ");
	 Serial.print(windspdavg[i]);
	 }*/

	//Rain for last hour
	Serial.println();
	Serial.print("Rain hour:");
	for(int16_t i = 0 ; i < 60 ; i++) {
		if(i % 30 == 0) Serial.println();
		Serial.print(" ");
		Serial.print(rainHour[i]);
	}

}

//When the imp tells us it's midnight, reset the total amount of rain and gusts
void WeatherStation::resetVars() {
	//These are all the weather values that wunderground expects:
	dailyrainin = 0; //Reset daily amount of rain

	windgustmph = 0; //Zero out the windgust for the day
	windgustdir = 0; //Zero out the gust direction for the day

	minutes = 0; //Reset minute tracker
	seconds = 0;
	lastSecond = millis(); //Reset variable used to track minutes

	minutesSinceLastReset = 0; //Zero out the backup midnight reset variable
}

//Calculates each of the variables that wunderground is expecting
void WeatherStation::calcWeather() {
	//current winddir, current windspeed, windgustmph, and windgustdir are calculated every 100ms throughout the day

	//Calc windgustmph_10m
	//Calc windgustdir_10m
	//Find the largest windgust in the last 10 minutes
	windgustmph_10m = 0;
	windgustdir_10m = 0;
	//Step through the 10 minutes
	for(int16_t i = 0; i < 10 ; i++) {
		if(windgust_10m[i] > windgustmph_10m) {
			windgustmph_10m = windgust_10m[i];
			windgustdir_10m = windgustdirection_10m[i];
		}
	}
}

int16_t WeatherStation::getWindDir() {
  return winddir;
}

float WeatherStation::getWindSpeedMPH() {
  return windspeedmph;
}

float WeatherStation::getWindGustMPH() {
  return windgustmph;
}

int16_t WeatherStation::getWindGustDir() {
  return windgustdir;
}

float WeatherStation::getWindSpeedMPH_Avg2m() {
  //Calc windspdmph_avg2m
  float windspdmph_avg2m = 0;
  for(int16_t i = 0 ; i < 120 ; i++)
    windspdmph_avg2m += windspdavg[i];
  windspdmph_avg2m /= 120.0;

  return windspdmph_avg2m;
}

int16_t WeatherStation::getWindDir_Avg2m() {
  //Calc winddir_avg2m, Wind Direction
  //You can't just take the average. Google "mean of circular quantities" for more info
  //We will use the Mitsuta method because it doesn't require trig functions
  //And because it sounds cool.
  //Based on: http://abelian.org/vlf/bearings.html
  //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
  int32_t sum = winddiravg[0];
  int16_t D = winddiravg[0];
  for(int16_t i = 1 ; i < WIND_DIR_AVG_SIZE ; i++) {
    int16_t delta = winddiravg[i] - D;

    if(delta < -180)
      D += delta + 360;
    else if(delta > 180)
      D += delta - 360;
    else
      D += delta;

    sum += D;
  }
  int16_t winddir_avg2m = sum / WIND_DIR_AVG_SIZE;
  while(winddir_avg2m >= 360) winddir_avg2m -= 360;
  while(winddir_avg2m < 0) winddir_avg2m += 360;

  return winddir_avg2m;
}

float WeatherStation::getWindGustMPH_10m() {
  return windgustmph_10m;
}

int16_t WeatherStation::getWindGustDir_10m() {
  return windgustdir_10m;
}

float WeatherStation::getRainIn() {
	//Total rainfall for the day is calculated within the interrupt
        //Calculate amount of rainfall for the last 60 minutes
        float rainin = 0;
        for(int16_t i = 0 ; i < 60 ; i++)
                rainin += rainHour[i];

	return rainin;
}

float WeatherStation::getDailyRainIn() {
  return dailyrainin;
}

//Returns the instataneous wind speed
float WeatherStation::get_wind_speed() {
	float deltaTime = millis() - lastWindCheck; //750ms

	deltaTime /= 1000.0; //Covert to seconds

	float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

	windClicks = 0; //Reset and start watching for new wind
	lastWindCheck = millis();

	windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

	return(windSpeed);
}

