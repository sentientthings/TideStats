/*
  TideStats.h - Sentient Things tide statistics library
  Copyright (c) 2020 Sentient Things, Inc.  All right reserved.
  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License

  Version 0.0.3

*/
#include "Particle.h"

// include this library's description file
#include "TideStats.h"

#include "math.h"

#define SERIAL_DEBUG

#ifdef SERIAL_DEBUG
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define DEBUG_PRINTLNF(...) Serial.printlnf(__VA_ARGS__)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
  #define DEBUG_PRINTLNF(...)
  #define DEBUG_PRINTF(...)
#endif

#define PI 3.14159265

LEDStatus blinkWhite(RGB_COLOR_WHITE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

// include description files for other libraries used (if any)

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances

TideStats::TideStats(IoTNode& node) : _node(node),
                                        framState(node.makeFramArray(1, sizeof(state))),
                                        framCalibDist(node.makeFramArray(1, sizeof(calibDist))),
                                        framCalib(node.makeFramArray(1, sizeof(calib))),
                                        framMllw(node.makeFramArray(1, sizeof(mllwStats))),
                                        framMhhw(node.makeFramArray(1, sizeof(mhhwStats))),
                                        framMsl(node.makeFramArray(1, sizeof(mslStats))),
                                        framRecord(node.makeFramArray(1, sizeof(record))),
                                        framPeriodDatum((node.makeFramArray(1, sizeof(periodDatum))))                                                                              
                                        
{
  // initialize this instance's variables


  // do whatever is required to initialize the library

}

// Public Methods //////////////////////////////////////////////////////////////

bool TideStats::initialize()
{
  
  if (!Wire.isEnabled())
  {
      Wire.begin();
  }

  if (_node.ok())
  {
    loadFram();

    String framID = String(state.runID);

    if (!framID.equals(_node.nodeID))
    {
        blinkWhite.setActive(true);
        delay(3000);
        blinkWhite.setActive(false);
        clear();
        // Then this is the first time running
        _node.nodeID.toCharArray(state.runID,16);
        saveFram();
    }
    return true;
  }
  return false;
}


bool TideStats::isCalibrated()
{
  return calib.calibrated;
}


void TideStats::pushDistanceUpwards(double distUp)
{
      uint32_t readingTime = _node.unixTime();
            
      // Calculate msl
      mslStats = pushStats(distUp, framMsl);

      // Check for first run of tide period stats
      if (periodDatum.startTime == 0 || periodEnd || (readingTime - state.lastReadingTime) > DATUM_MAX_GAP_S)
      {
        DEBUG_PRINTLN("Period start");
        periodDatum.mhhw = distUp;
        periodDatum.mllw = distUp;
        periodDatum.startTime = readingTime; // exit here?
        periodEnd = false;
      }
      // If we are still in the current datum period
      else if (readingTime - periodDatum.startTime < DATUM_PERIOD_S)
      {
        // Same period so update period mllw and mhhw if applicable
        DEBUG_PRINT(readingTime - periodDatum.startTime);
        DEBUG_PRINT("s of ");
        DEBUG_PRINT(DATUM_PERIOD_S);
        DEBUG_PRINT("s period: range:");
        DEBUG_PRINT(distUp);
        DEBUG_PRINT(" period low:");
        DEBUG_PRINT(periodDatum.mllw);
        DEBUG_PRINT(" period high");
        DEBUG_PRINTLN(periodDatum.mhhw);   
        if (distUp < periodDatum.mllw)
        {
          periodDatum.mllw = distUp;
          DEBUG_PRINTLNF("Period mllw of %f",distUp);
        }

        if (distUp > periodDatum.mhhw)
        {        
          periodDatum.mhhw = distUp;
          DEBUG_PRINTLNF("Period mhhw of %f",distUp);
        }

      }
      else
      {
        DEBUG_PRINTLN("Period end");
        periodEnd = true;
        mllwStats = pushStats(periodDatum.mllw, framMllw);
        mhhwStats = pushStats(periodDatum.mhhw, framMhhw);
        periodDatum.startTime = readingTime;
      }
      state.lastReadingTime = readingTime;
      saveFram(); // Could me more efficient to only save what has changed
}

void TideStats::pushDistanceUpwards(double distUp, uint32_t readingTime)
{
     
      // Calculate msl
      mslStats = pushStats(distUp, framMsl);

      // Check for first run of tide period stats
      if (periodDatum.startTime == 0 || periodEnd || (readingTime - state.lastReadingTime) > DATUM_MAX_GAP_S)
      {
        DEBUG_PRINTLN("Period start");
        periodDatum.mhhw = distUp;
        periodDatum.mllw = distUp;
        periodDatum.startTime = readingTime;
        periodEnd = false;
      }
      // If we are still in the current datum period
      else if (readingTime - periodDatum.startTime < DATUM_PERIOD_S)
      {
        // Same period so update period mllw and mhhw if applicable
        DEBUG_PRINT(readingTime - periodDatum.startTime);
        DEBUG_PRINT("s of ");
        DEBUG_PRINT(DATUM_PERIOD_S);
        DEBUG_PRINT("s period: range:");
        DEBUG_PRINT(distUp);
        DEBUG_PRINT(" period low:");
        DEBUG_PRINT(periodDatum.mllw);
        DEBUG_PRINT(" period high");
        DEBUG_PRINTLN(periodDatum.mhhw);   
        if (distUp < periodDatum.mllw)
        {
          periodDatum.mllw = distUp;
          DEBUG_PRINTLNF("Period mllw of %f",distUp);
        }

        if (distUp > periodDatum.mhhw)
        {
          periodDatum.mhhw = distUp;
          DEBUG_PRINTLNF("Period mhhw of %f",distUp);
        }

      }
      else
      {
        periodEnd = true;
        mllwStats = pushStats(periodDatum.mllw, framMllw);
        mhhwStats = pushStats(periodDatum.mhhw, framMhhw);
        periodDatum.startTime = readingTime;
        DEBUG_PRINTLN("Period ended");
        DEBUG_PRINTLNF("Period mllw of %f",periodDatum.mllw);
        DEBUG_PRINTLNF("Period mhhw of %f",periodDatum.mhhw);
        DEBUG_PRINTLNF("mllwStats.mean of %f",mllwStats.mean);
        DEBUG_PRINTLNF("mhhwStats.mean of %f",mhhwStats.mean);        
      }
      state.lastReadingTime = readingTime;
      saveFram(); // Could me more efficient to only save what has changed
}

float TideStats::mllwCalibrationHoursLeft()
{
  uint32_t readingTime = _node.unixTime();
  float hoursLeft;
  if (mllw()==0)
  {
    hoursLeft = (DATUM_PERIOD_S - float(readingTime - periodDatum.startTime))/3600.0;
  }
  else
  {
    hoursLeft = 0;
  }
  
  return hoursLeft;
}

// Private Methods /////////////////////////////////////////////////////////////
// Functions only available to other functions in this library

void TideStats::hardReset()
{
  clear();
  delay(20);
  System.reset();
}

void TideStats::clear()
{
  strcpy (state.runID, "FFFFFFFFFFFFFFFF"); // Compare this to nodeID
  state.mode = gpsmode; //0=GPS,1=calibrate,2=run
  state.lat = 0;
  state.lon = 0;
  state.A = 0;
  state.k = 0;
  state.lastReadingTime = 0;

  calibDist.num = 0;
  calibDist.mean = 0;
  calibDist.s = 0;
  calib.numReadings = 0;
  calib.totalDurationSec = 0;
  calib.calibrated = false;
  mllwStats.num = 0;
  mllwStats.mean = 0;
  mllwStats.s = 0;
  mhhwStats.num = 0;
  mhhwStats.mean = 0;
  mhhwStats.s = 0;
  mslStats.num = 0;
  mslStats.mean = 0;
  mslStats.s = 0;
  record.high = 0;
  record.highTime = 0;
  record.low = 0;
  record.lowTime = 0;
  periodDatum.startTime = 0;  
  periodDatum.mhhw = 0;
  periodDatum.mllw = 0;

  saveFram();
}


stat_t TideStats::pushStats(double x, framArray& statArray)
{
  stat_t stats;
  
  statArray.read(0, (uint8_t*)&stats);
  stats.num++;
  n=stats.num;
  oldM = stats.mean;
  oldS = stats.s;

  // https://www.johndcook.com/blog/standard_deviation/
  // See Knuth TAOCP vol 2, 3rd edition, page 232
  if (n == 1)
  {
      stats.mean = x;
  }
  else
  {
      newM = oldM + (x - oldM)/n;
      newS = oldS + (x - oldM)*(x - newM);

      // set up for next iteration
      stats.mean = newM; 
      stats.s = newS;
  }

  statArray.write(0, (uint8_t*)&stats);  
  return stats;
}

int TideStats::numDataValues() const
{
    return n;
}

double TideStats::mllw()
{
  framMllw.read(0, (uint8_t*)&mllwStats);
  return mllwStats.mean;
}

double TideStats::mhhw()
{
  framMhhw.read(0, (uint8_t*)&mhhwStats);
  return mhhwStats.mean;
}

double TideStats::msl()
{
  framMsl.read(0, (uint8_t*)&mslStats);
  return mslStats.mean;
}

double TideStats::standardDeviationMllw()
{
  framMllw.read(0, (uint8_t*)&mllwStats);
  if (mllwStats.num > 1)
  {
    return sqrt (mllwStats.s/(mllwStats.num - 1));
  }
  else
  {
    return 0.0;
  }
}

double TideStats::standardDeviationMhhw()
{
  framMhhw.read(0, (uint8_t*)&mhhwStats);
  if (mhhwStats.num > 1)
  {
    return sqrt (mhhwStats.s/(mhhwStats.num - 1));
  }
  else
  {
    return 0.0;
  }
}

double TideStats::standardDeviationMsl()
{
  framMsl.read(0, (uint8_t*)&mslStats);
  if (mslStats.num > 1)
  {
    return sqrt (mslStats.s/(mslStats.num - 1));
  }
  else
  {
    return 0.0;
  }
}

double TideStats::standardDeviation(stat_t stats)
{
  if (stats.num > 0)
  {
    return sqrt(stats.s/(stats.num-1));
  }
  else
  {
    return 0.0;
  }
}

void TideStats::loadFram()
{
  framState.read(0, (uint8_t*)&state);
  framCalib.read(0, (uint8_t*)&calib);
  framMllw.read(0, (uint8_t*)&mllwStats);
  framMhhw.read(0, (uint8_t*)&mhhwStats);
  framMsl.read(0, (uint8_t*)&mslStats);
  framPeriodDatum.read(0, (uint8_t*)&periodDatum);
  framRecord.read(0, (uint8_t*)&record);
}

void TideStats::saveFram()
{
  framState.write(0, (uint8_t*)&state);
  framCalib.write(0, (uint8_t*)&calib);
  framMllw.write(0, (uint8_t*)&mllwStats);
  framMhhw.write(0, (uint8_t*)&mhhwStats);
  framMsl.write(0, (uint8_t*)&mslStats);
  framPeriodDatum.write(0, (uint8_t*)&periodDatum);
  framRecord.write(0, (uint8_t*)&record);
}