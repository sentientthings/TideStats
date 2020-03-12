/*
  TideStats.h - Sentient Things tide statistics library
  Copyright (c) 2020 Sentient Things, Inc.  All right reserved.
  Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International Public License

  Version 0.0.3

*/
// ensure this library description is only included once
#ifndef TideStats_h
#define TideStats_h

#include "IoTNode.h"

// #define CAL_DEBUG

#ifdef CAL_DEBUG
#define MIN_CALIBRATION_READINGS 3
#define MIN_CALIBRATION_SEC_DURATION 180
#define MAX_CALIBRATION_GAP_SEC 120

#define DATUM_PERIOD_S 300 // 5 min
#define DATUM_MAX_GAP_S 120 //2 min
#else
#define MIN_CALIBRATION_READINGS 13
#define MIN_CALIBRATION_SEC_DURATION 46800
#define MAX_CALIBRATION_GAP_SEC 3660

#define DATUM_PERIOD_S 90000 // 25 hours to measure both low tides
#define DATUM_MAX_GAP_S 3600 //60 minutes
#endif

// Struct to store the state of the node
// Structure to store state of device through power down cycles
typedef struct
{
  char runID[17]; // Compare this to nodeID
  int mode; //0=GPS,1=calibrate,2=run
  int type; //0=xxxx, 1=range, 2=pressure/depth
  float lat;
  float lon;
  float A;
  float k;
  uint32_t lastReadingTime;
}state_t;

enum modeName {waitmode, gpsmode, calmode, runmode, errormode};

typedef struct
{  
  uint32_t num;
  double mean;
  double s;
}stat_t;

typedef struct
{  
  uint32_t startTime;
  float mllw; // This is found looking for a minimum over tide cycle
  float mhhw; // This is found looking for a maximum over tide cycle
}periodDatum_t;

typedef struct
{
  float high;
  uint32_t highTime;
  float low;
  uint32_t lowTime;  
}record_t;

typedef struct
{
  uint32_t numReadings;
  uint32_t totalDurationSec;
  bool calibrated;
}calib_t;


// library interface description
class TideStats
{
  // user-accessible "public" interface
  public:
    TideStats(IoTNode& node);

    /**
     * @brief Push distance from the sensor. Upwards is positive.
     * 
     * i.e. depth is a positive number, and
     * ultrasonic or radar range from a sensor above
     * the water is a negative number (downwards)
     * 
     * @param distUp - distance to the water level from the sensor
     */
    void pushDistanceUpwards(double distUp);

    /**
     * @brief Push distance from the sensor. Upwards is positive.
     * 
     * i.e. depth is a positive number, and
     * ultrasonic or radar range from a sensor above
     * the water is a negative number (downwards)
     * 
     * @param distUp - distance to the water level from the sensor
     * @param readingTime - Epoch or Unix time in seconds
     */
    void pushDistanceUpwards(double distUp, uint32_t readingTime);

    bool isCalibrated();

    /**
     * @brief Run to set up the tide variables in Fram
     * 
     * @return true 
     * @return false 
     */
    bool initialize();

    /**
     * @brief Clear ALL the tide Fram variables and reset
     * 
     */
    void hardReset();

    /**
     * @brief Clear all the tide variables
     * 
     */
    void clear();

    /**
     * @brief Returns the mean lower low water
     * relative to the position of the sensor.
     * 
     * @return double - the mean lower low water
     */
    double mllw();
    double mhhw();
    double msl();

    /**
     * @brief Returns the number of hours left for the first mllw
     * 
     * @return float 
     */
    float mllwCalibrationHoursLeft();

    double standardDeviationMllw();
    double standardDeviationMhhw();
    double standardDeviationMsl();
  // library-accessible "private" interface
  private:

    IoTNode _node;

    state_t state;

    stat_t calibDist;

    calib_t calib;

    // Structs for datum calculations
    // For range it is relative to the sensor above water
    // if up is positive then range readings are negative and (mllw is negative) 
    // and the formula will be the same.
    // Height above mllw = reading - mllw
    // These values are the depth or range values relative to the measurment points
    // They can be converted to mllw as shown above
    stat_t mllwStats;
    stat_t mhhwStats;
    stat_t mslStats;

    periodDatum_t periodDatum;
    record_t record;

    // Create FRAM ring 
    // framRing myframring;
    // Create arrays
    framArray framState;
    framArray framCalibDist;
    framArray framCalib;
    framArray framMllw;
    framArray framMhhw;
    framArray framMsl;
    framArray framRecord;
    framArray framPeriodDatum;

    void loadFram();
    void saveFram();

    stat_t pushStats(double x, framArray& statArray);

    int numMllwValues();
    int numMhhwValues();
    int numMslValues();

    bool periodEnd = false;

    double standardDeviation(stat_t stats);


    int numDataValues() const;

    int n;
    double oldM, newM, oldS, newS;
};

#endif