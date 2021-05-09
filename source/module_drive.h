#ifndef __MODULE_FUNCTIONS__
#define __MODULE_FUNCTIONS__


#include <avr/io.h>
#include <stdbool.h>


#define cbit(a, b) (a &= ~(1<<b))   //Clear bit
#define sbit(a, b) (a |= 1<<b)   //Set bit
#define tbit(a, b) (a ^= 1<<b)   //Toggle bit
#define rbit(a, b) (a & (1<<b))  //Read bit

#define SIGN(x) ((x>0)? (1):(-1))
#define ABS(x) ((x>0)? (x):-(x))


#define DDR_BUTTON        DDRD
#define PIN_BUTTON        PIND
#define PORT_BUTTON       PORTD
#define PIN_BACK          3
#define PIN_FORWARD       2
#define PIN_TOUCH         4

#define PORT_MOTOR_EN     PORTD
#define DDR_MOTOR_EN      DDRD
#define PIN_EN            7
#define PORT_MOTOR        PORTB
#define DDR_MOTOR         DDRB
#define PIN_STEP          0
#define PIN_DIR           1


#define  BUTTON_STATE_BACK()       ( rbit(PIN_BUTTON, PIN_BACK) )
#define  BUTTON_STATE_TOUCH()      ( rbit(PIN_BUTTON, PIN_TOUCH) )
#define  BUTTON_STATE_FORWARD()    ( rbit(PIN_BUTTON, PIN_FORWARD) )


typedef enum {
	NO_OPERATION = 0, //Begin value
	EXECUTING = 1,    //In progress
	ABORTED = 2,      //When something gone wrong
	FINISHED = 3      //When successfull finished

} EDriveStatus;


typedef enum {
	NO_TRIGGER = 0,
	BUTTON_BACK = 1,
	BUTTON_TOUCH = 2,
	BUTTON_FORWARD = 3,
	TENZO = 4,
	CANT_MOVE = 5,
	EXTERNASL_STOP = 6

} EDriveTrigger;


typedef enum {
	ABSOLUTE = 0,
	RELATIVE = 1

} EDriveMode;


typedef enum {
  FORWARD = 0,
  BACK = 1

} EDriveDirection;


void  drive_position_to_zero(void);
void  drive_init(void);
void  drive_stop(void);
void  drive_rotate(EDriveDirection dir, uint16_t steps_per_second);
void  drive_move(EDriveMode mode, float pos, float speed, int16_t force);
float drive_get_position(void);
float drive_get_speed(void);

EDriveStatus drive_get_status(void);
EDriveTrigger drive_get_trigger(void);


#endif
