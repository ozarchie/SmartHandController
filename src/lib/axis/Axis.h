// -----------------------------------------------------------------------------------
// Axis motion control
#pragma once

#include "../../Common.h"

#ifdef MOTOR_PRESENT

// default location of settings in NV
#ifndef NV_AXIS_SETTINGS_REVERT
#define NV_AXIS_SETTINGS_REVERT     100    // bytes: 2   , addr:  100.. 101
#endif
#ifndef NV_AXIS_SETTINGS_BASE
#define NV_AXIS_SETTINGS_BASE       102    // bytes: 25  , addr:  102.. 224 (for 9 axes)
#endif

// polling frequency for monitoring axis motion (default 100X/second) 
#ifndef FRACTIONAL_SEC
  #define FRACTIONAL_SEC            100.0F
#endif
#define FRACTIONAL_SEC_US           (lround(1000000.0F/FRACTIONAL_SEC))

#include "../../libApp/commands/ProcessCmds.h"
#include "motor/Motor.h"
#include "motor/stepDir/StepDir.h"
#include "motor/servo/Servo.h"

#pragma pack(1)
typedef struct AxisLimits {
  float min;
  float max;
} AxisLimits;

// helpers for step/dir and servo parameters
#define subdivisions param1
#define subdivisionsGoto param2
#define currentHold param3
#define currentRun param4
#define currentGoto param5
#define integral param1
#define porportional param2
#define derivative param3
#define integralGoto param4
#define porportionalGoto param5
#define derivativeGoto param6

#define AxisSettingsSize 45
typedef struct AxisSettings {
  double     stepsPerMeasure;
  int8_t     reverse;
  float      param1, param2, param3, param4, param5, param6;
  AxisLimits limits;
  float      backlashFreq;
} AxisSettings;
#pragma pack()

typedef struct AxisSense {
  int32_t    homeTrigger;
  int8_t     homeInit;
  int32_t    minTrigger;
  int32_t    maxTrigger;
  int8_t     minMaxInit;
} AxisSense;

typedef struct AxisPins {
  int16_t    min;
  int16_t    home;
  int16_t    max;
  AxisSense  axisSense;
} AxisPins;

typedef struct AxisErrors {
  uint8_t minLimitSensed:1;
  uint8_t maxLimitSensed:1;
} AxisErrors;

enum AutoRate: uint8_t {AR_NONE, AR_RATE_BY_TIME_ABORT, AR_RATE_BY_TIME_END, AR_RATE_BY_DISTANCE, AR_RATE_BY_TIME_FORWARD, AR_RATE_BY_TIME_REVERSE};
enum HomingStage: uint8_t {HOME_NONE, HOME_FINE, HOME_SLOW, HOME_FAST};

class Axis {
  public:
    // constructor
    Axis(uint8_t axisNumber, const AxisPins *pins, const AxisSettings *settings);

    // process axis commands
    bool command(char *reply, char *command, char *parameter, bool *supressFrame, bool *numericReply, CommandError *commandError);

    // sets up the driver step/dir/enable pins and any associated driver mode control
    void init(Motor *motor, void (*callback)());

    // enables or disables the associated step/dir driver
    void enable(bool value);

    // get the enabled state
    inline bool isEnabled() { return enabled; }

    // time (in ms) before automatic power down at standstill, use 0 to disable
    void setPowerDownTime(int value);

    // time (in ms) to disable automatic power down at standstill, use 0 to disable
    void setPowerDownOverrideTime(int value);

    // get steps per measure
    inline double getStepsPerMeasure() { return settings.stepsPerMeasure; }

    // get tracking mode steps per slewing mode step
    inline int getStepsPerStepSlewing() { return motor->getStepsPerStepSlewing(); }

    // reset motor and target angular position, in "measure" units
    CommandError resetPosition(double value);

    // reset motor and target angular position, in steps
    CommandError resetPositionSteps(long value);

    // resets target position to the motor position
    inline void resetTargetToMotorPosition() { motor->resetTargetToMotorPosition(); }

    // get motor position, in "measure" units
    double getMotorPosition();

    // get motor position, in steps
    inline long getMotorPositionSteps() { return motor->getMotorPositionSteps(); }

    // get index position, in "measure" units
    double getIndexPosition();

    // get index position, in steps
    inline long getIndexPositionSteps() { return motor->getIndexPositionSteps(); }

    // coordinate wrap, in "measures" (radians, microns, etc.)
    // wrap occurs when the coordinate exceeds the min or max limits
    // default 0.0 (disabled)
    inline void coordinateWrap(double value) { wrapAmount = value; wrapEnabled = fabs(value) > 0.0F; }

    // set instrument coordinate, in "measures" (radians, microns, etc.)
    void setInstrumentCoordinate(double value);

    // set instrument coordinate, in steps
    inline void setInstrumentCoordinateSteps(long value) { motor->setInstrumentCoordinateSteps(value); }

    // get instrument coordinate
    double getInstrumentCoordinate();

    // get instrument coordinate, in steps
    inline long getInstrumentCoordinateSteps() { return motor->getInstrumentCoordinateSteps(); }

    // set instrument coordinate park, in "measures" (radians, microns, etc.)
    // with backlash disabled this indexes to the nearest position where the motor wouldn't cog
    void setInstrumentCoordinatePark(double value);

    // set target coordinate park, in "measures" (degrees, microns, etc.)
    // with backlash disabled this moves to the nearest position where the motor doesn't cog
    void setTargetCoordinatePark(double value);

    // set target coordinate, in "measures" (degrees, microns, etc.)
    void setTargetCoordinate(double value);

    // set target coordinate, in steps
    inline void setTargetCoordinateSteps(long value) { motor->setTargetCoordinateSteps(value); }

    // get target coordinate, in "measures" (degrees, microns, etc.)
    double getTargetCoordinate();

    // get target coordinate, in steps
    inline long getTargetCoordinateSteps() { return motor->getTargetCoordinateSteps(); }

    // distance to target in "measures" (degrees, microns, etc.)
    double getTargetDistance();

    // returns true if at target
    bool atTarget();

    // set backlash amount in "measures" (radians, microns, etc.)
    void setBacklash(float value);

    // set backlash amount in steps
    inline void setBacklashSteps(long value) { if (autoRate == AR_NONE) motor->setBacklashSteps(value); }

    // get backlash amount in "measures" (radians, microns, etc.)
    float getBacklash();

    // get frequency in "measures" (degrees, microns, etc.) per second
    float getFrequency();

    // get frequency in steps per second
    float getFrequencySteps() { return motor->getFrequencySteps(); }

    // gets backlash frequency in "measures" (degrees, microns, etc.) per second
    float getBacklashFrequency();

    // set base movement frequency in "measures" (radians, microns, etc.) per second
    void setFrequencyBase(float frequency);

    // set slew frequency in "measures" (radians, microns, etc.) per second
    void setFrequencySlew(float frequency);

    // set minimum slew frequency in "measures" (radians, microns, etc.) per second
    void setFrequencyMin(float frequency);

    // set maximum frequency in "measures" (radians, microns, etc.) per second
    void setFrequencyMax(float frequency);

    // set acceleration rate in "measures" per second per second (for autoSlew)
    void setSlewAccelerationRate(float mpsps);

    // set acceleration rate in seconds (for autoSlew)
    void setSlewAccelerationTime(float seconds);

    // set acceleration for emergency stop movement in "measures" per second per second
    void setSlewAccelerationRateAbort(float mpsps);

    // set acceleration for emergency stop movement in seconds (for autoSlewStop)
    void setSlewAccelerationTimeAbort(float seconds);

    // slew with rate by distance
    // \param distance: acceleration distance in measures (to frequency)
    // \param frequency: optional frequency of slew in "measures" (radians, microns, etc.) per second
    CommandError autoSlewRateByDistance(float distance, float frequency = NAN);

    // auto slew with acceleration in "measures" per second per second
    // \param direction: direction of motion, DIR_FORWARD or DIR_REVERSE
    // \param frequency: optional frequency of slew in "measures" (radians, microns, etc.) per second
    CommandError autoSlew(Direction direction, float frequency = NAN);

    // slew to home, with acceleration in "measures" per second per second
    CommandError autoSlewHome(unsigned long timeout);

    // stops, with deacceleration by time
    void autoSlewStop();

    // emergency stops, with deacceleration by time
    void autoSlewAbort();

    // checks if slew is active on this axis
    bool isSlewing();

    // returns 1 if departing origin or -1 if approaching target
    inline int getRampDirection() { return motor->getRampDirection(); }

    // set synchronized state (automatic movement of target at setFrequencySteps() rate)
    inline void setSynchronized(bool state) { motor->setSynchronized(state); }

    // get synchronized state (automatic movement of target at setFrequencySteps() rate)
    inline bool getSynchronized() { return motor->getSynchronized(); }

    // report fault status of motor driver, if available
    inline bool fault() { return motor->getDriverStatus().fault; };

    // get associated motor driver status
    DriverStatus getStatus();

    // enable/disable numeric position range limits (doesn't apply to limit switches)
    void setMotionLimitsCheck(bool state);

    // checks for an error that would disallow motion in a given direction or DIR_BOTH for any motion
    bool motionError(Direction direction);

    // checks for an sense error that would disallow motion in a given direction or DIR_BOTH for any motion
    bool motionErrorSensed(Direction direction);

    // monitor movement
    void poll();

    // associated motor
    Motor *motor;

    AxisSettings settings;

  private:
    // set frequency in "measures" (degrees, microns, etc.) per second (0 stops motion)
    void setFrequency(float frequency);

    // sets driver power on/off
    void powered(bool value);

    // distance to origin or target, whichever is closer, in "measures" (degrees, microns, etc.)
    double getOriginOrTargetDistance();

    // returns true if traveling through backlash
    bool inBacklash();

    // convert from unwrapped (full range) to normal (+/- wrapAmount) coordinate
    double wrap(double value);

    // convert from normal (+/- wrapAmount) to an unwrapped (full range) coordinate
    double unwrap(double value);

    // convert from normal (+/- wrapAmount) to an unwrapped (full range) coordinate
    // nearest the instrument coordinate
    double unwrapNearest(double value);

    bool decodeAxisSettings(char *s, AxisSettings &a);

    bool validateAxisSettings(int axisNum, AxisSettings a);
    
    AxisErrors errors;
    bool lastErrorResult = false;
    bool commonMinMaxSense = false;

    uint8_t axisNumber = 0;
    char axisPrefix[13] = "MSG: Axis_, ";

    bool enabled = false;        // enable/disable logical state (disabled is powered down)
    bool limitsCheck = true;     // enable/disable numeric position range limits (doesn't apply to limit switches)

    uint8_t homeSenseHandle = 0; // home sensor handle
    uint8_t minSenseHandle = 0;  // min sensor handle
    uint8_t maxSenseHandle = 0;  // max sensor handle

    uint16_t backlashStepsStore;
    volatile uint16_t backlashSteps = 0;
    volatile uint16_t backlashAmountSteps = 0;

    volatile bool takeStep = false;

    // coordinate handling
    double wrapAmount = 0.0L;
    bool wrapEnabled = false;

    // power down standstill control
    bool powerDownStandstill = false;
    bool powerDownOverride = false;
    unsigned long powerDownDelay;
    unsigned long powerDownOverrideEnds;
    bool poweredDown = false;
    unsigned long powerDownTime = 0;

    // timeout for home switch detection
    unsigned long homeTimeoutTime = 0;

    // rates (in measures per second) to control motor movement
    float freq = 0.0F;
    float rampFreq = 0.0F;
    float baseFreq = 0.0F;
    float minFreq = 0.0F;
    float slewFreq = 0.0F;
    float maxFreq = 0.0F;

    AutoRate autoRate = AR_NONE;       // auto slew mode
    float slewAccelerationDistance;    // auto slew rate distance in measures to max rate
    float slewMpspfs;                  // auto slew rate in measures per second per frac-sec
    float abortMpspfs;                 // abort slew rate in measures per second per fracsec
    float slewAccelTime = NAN;         // auto slew acceleration time in seconds
    float abortAccelTime = NAN;        // abort slew acceleration time in seconds

    HomingStage homingStage = HOME_NONE;

    const AxisPins *pins;

    void (*volatile callback)() = NULL;
};

#endif
