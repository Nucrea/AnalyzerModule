#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "module_tenzo.h"
#include "module_uart.h"
#include "module_drive.h"


#define cbit(a, b) (a &= ~(1<<b)) //Clear bit
#define sbit(a, b) (a |= 1<<b)    //Set bit
#define tbit(a, b) (a ^= 1<<b)    //Toggle bit
#define rbit(a, b) (a & (1<<b))   //Read bit

#define SIGN(x) ((x>0)? (1):(-1))
#define ABS(x)  ((x>0)? (x):-(x))

#ifdef Z_DRIVE
  #define OFFSET_PER_STEP         0.025
#else
  #define OFFSET_PER_STEP         0.01
#endif

//SPS - Steps Per Second
//Speed in mm/s, offset in mm
#define SPEED_TO_SPS(speed)       (speed/OFFSET_PER_STEP)
#define OFFSET_TO_STEPS(offset)   (offset/OFFSET_PER_STEP)
#define SPS_TO_SPEED(rps)         (rps*OFFSET_PER_STEP)
#define STEPS_TO_OFFSET(steps)    (steps*OFFSET_PER_STEP)

#define TMR_CLKDIV_1     (1<<CS10)
#define TMR_CLKDIV_8     (1<<CS11)
#define TMR_CLKDIV_64    ((1<<CS11) | (1<<CS10))
#define TMR_CLKDIV_256   (1<<CS12)
#define TMR_CLKDIV_1024  ((1<<CS12) | (1<<CS10))
#define TMR_ENABLE       (1<<WGM12)

#define SET_DRIVE_SPS(sps) { OCR1A = (31250.0) / ((float) sps); }

struct {
  EDriveDirection direction;
  EDriveTrigger trigger;
  EDriveStatus status;
  int16_t  target_force;
  int32_t  target_steps;
  uint16_t target_rps;
  uint16_t steps_per_second;
  int32_t  steps;
  int8_t  steps_increment;

} volatile drive = {0};


//----------------------------------------
#ifdef Z_DRIVE
  static const uint16_t  drive_minimum_sps = 100;
  static const uint16_t  drive_maximum_sps = 600;
#else
  static const uint16_t  drive_minimum_sps = 100;
  static const uint16_t  drive_maximum_sps = 1200;
#endif

static const int16_t   drive_maximum_force = 500;
//----------------------------------------

static void _set_state(EDriveStatus status, EDriveTrigger trigger);

static inline void drive_set_direction(EDriveDirection dir);
static inline void drive_set_power(bool power);
static inline void drive_start_rotate(uint16_t rps);
static inline void drive_stop_rotate(void);


//------------------------------------------//
//------------ BASE FUNCTIONS --------------//
//------------------------------------------//

void drive_set_power(bool power)
{
  if (power) {
    cbit(PORT_MOTOR_EN, PIN_EN);
  } else {
    sbit(PORT_MOTOR_EN, PIN_EN);
  }
}


void drive_set_direction(EDriveDirection dir)
{
  drive.direction = dir;

  if (dir == FORWARD) {
    sbit(PORT_MOTOR, PIN_DIR);
  } else {
    cbit(PORT_MOTOR, PIN_DIR);
  }
}


void drive_start_rotate(uint16_t sps)
{
  drive_set_power(true);
  drive.steps_per_second = sps<drive_minimum_sps? drive_minimum_sps : sps;
  drive.steps_per_second = sps>drive_maximum_sps? drive_maximum_sps : sps;
  SET_DRIVE_SPS(drive.steps_per_second);
  TCCR1B = TMR_ENABLE | TMR_CLKDIV_256;
}


void drive_stop_rotate(void)
{
  TCCR1B = 0;
  drive.steps_per_second = 0;
  drive_set_power(false);
}


//------------------------------------------//
//------------ DRIVE FUNCTIONS -------------//
//------------------------------------------//

void drive_init()
{
  DDR_BUTTON &= ~(1<<PIN_TOUCH | 1<<PIN_BACK | 1<<PIN_FORWARD);
  PORT_BUTTON &= ~(1<<PIN_TOUCH | 1<<PIN_BACK | 1<<PIN_FORWARD);

  DDR_MOTOR |= 1<<PIN_STEP | 1<<PIN_DIR;
  PORT_MOTOR &= ~(1<<PIN_STEP | 1<<PIN_DIR);
  DDR_MOTOR_EN |= 1<<PIN_EN;
  PORT_MOTOR_EN &= ~(1<<PIN_EN);

  //Timer1 config
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 |= 1<<OCIE1A;     //Compare interrupt

  drive_set_power(false);
  sei();
}

EDriveStatus drive_get_status(void) {
  return drive.status;
}

EDriveTrigger drive_get_trigger(void) {
  return drive.trigger;
}

float drive_get_position(void) {
  return STEPS_TO_OFFSET(drive.steps);
}

float drive_get_speed(void) {
  return SPS_TO_SPEED(drive.steps_per_second);
}

void drive_position_to_zero(void) {
  drive.steps = 0;
}

void drive_stop(void) {
  sei();
  drive_stop_rotate();
  _set_state(ABORTED, EXTERNASL_STOP);
  cli();
}


void drive_rotate(EDriveDirection dir, uint16_t steps_per_second)
{
  if (drive.status != EXECUTING) {
    drive_stop_rotate();
    drive.target_force = drive_maximum_force;
    drive.target_steps = drive.steps + 100;
    drive.steps_increment = 0;
    _set_state(EXECUTING, NO_TRIGGER);
    drive_set_direction(dir);
    drive_start_rotate(steps_per_second);
  }
}


void drive_move(EDriveMode position_mode, float pos, float speed, int16_t force)
{
  drive_stop_rotate();

  int32_t base_steps = (position_mode==RELATIVE)? drive.steps : 0;
  int32_t given_steps = OFFSET_TO_STEPS(pos);
  int32_t target_steps = base_steps + given_steps;
  EDriveDirection target_dir = (target_steps >= drive.steps)? FORWARD : BACK;

  if ( target_dir==FORWARD && BUTTON_STATE_FORWARD()
      || target_dir==BACK && BUTTON_STATE_BACK() ) {
      //|| ABS(tenzo_get_force() >= force) ) {
    _set_state(ABORTED, CANT_MOVE);
    return;
  }

  _set_state(EXECUTING, NO_TRIGGER);
  drive.target_steps = target_steps;
  drive.target_force = (force<drive_maximum_force)? force: drive_maximum_force;
  drive.steps_increment = (target_dir==FORWARD)? 1 : -1;
  drive_set_direction(target_dir);
  drive_start_rotate( SPEED_TO_SPS(speed) );
}


//------------------------------------------//
//------------ STATIC FUNCTIONS ------------//
//------------------------------------------//

static inline void _set_state(EDriveStatus status, EDriveTrigger trigger) {
  drive.status = status;
  drive.trigger = trigger;
}


//------------------------------------------------------------------
//--------------------- HANDLERS OF STEPS ---------------------------
//------------------------------------------------------------------

//Handler of movement
ISR(TIMER1_COMPA_vect)
{
  if ( !rbit(PORT_MOTOR, PIN_STEP) )
  {
    if ( drive.direction==FORWARD && BUTTON_STATE_FORWARD() ) {
      _set_state(ABORTED, BUTTON_FORWARD);
      drive_stop_rotate();
      return;
    }
    else if ( drive.direction==BACK && BUTTON_STATE_BACK() ) {
      _set_state(ABORTED, BUTTON_BACK);
      drive.steps = 0;
      drive_stop_rotate();
      return;
    }
    else if ( drive.direction==FORWARD && (tenzo_force >= drive.target_force) ) {
      _set_state(ABORTED, TENZO);
      drive_stop_rotate();
      return;
    }
    else if (drive.steps == drive.target_steps) {
      _set_state(FINISHED, NO_TRIGGER);
      drive_stop_rotate();
      return;
    }

    drive.steps += drive.steps_increment;
  }

  tbit(PORT_MOTOR, PIN_STEP);
}
