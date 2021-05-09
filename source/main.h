#ifndef __MAIN_H__
#define __MAIN_H__


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "module_uart.h"
#include "module_tenzo.h"
#include "module_drive.h"
#include "module_operations.h"


#define BOARD_ID_ADDR     ((void*) 0)
#define TENZO_COEFF_ADDR  ((void*) 1)

#define delay(t) _delay_ms(t)


typedef enum {
  STATUS_OK = 0,
  STATUS_EXECUTING = 1,
  STATUS_STOPPED = 2,
  STATUS_ERROR = 3

} ModuleStatus;


typedef enum  {
  OPERATION_NONE = 0,
  OPERATION_MOVE = 1,
  OPERATION_MOVE_BACK = 2,
  OPERATION_MOVE_FORWARD = 3,
  OPERATION_TOUCH = 4,
  OPERATION_MEASURE = 5,
  OPERATION_TOUCH_AND_MEASURE = 6

} ModuleOperations;


typedef enum
{
  CMD_SET_POSITION_MODE = 1,
  CMD_SET_POSITION = 2,
  CMD_SET_SPEED = 3,
  CMD_SET_FORCE = 4,
  CMD_ECHO = 5,
  CMD_STOP = 6,
  CMD_START_OPERATION = 7,
  CMD_SET_BOARD_ID = 8,
  CMD_READ_BOARD_ID = 9,
  CMD_READ_SENSORS = 10,
  CMD_READ_STATUS = 11,
  CMD_TENZO_TO_ZERO = 0xA1,
  CMD_SET_TENZO_COEFF_L = 0xA2,
  CMD_SET_TENZO_COEFF_H = 0xA3,
  CMD_ROTATE_FORWARD = 0xF1,
  CMD_ROTATE_BACK = 0xF2

} ModuleCommands;


typedef enum {
  ARG_READ_BUTTON_BACK = 1,
  ARG_READ_BUTTON_FORWARD = 2,
  ARG_READ_BUTTON_TOUCH = 3,
  ARG_READ_FORCE = 4,
  ARG_READ_SPEED = 5,
  ARG_READ_POSITION = 6

} ReadSensorsArguments;


typedef enum {
  ARG_IS_OPERATION_FINISHED = 1,
  ARG_READ_STATUS = 2,
  ARG_READ_TRIGGER = 3,
  ARG_READ_RADIUS = 4,
  ARG_READ_STIFFNESS = 5,
  ARG_READ_EMG = 6

} ReadStatusArguments;


//State machine data
typedef struct ModuleData
{
  uint8_t board_id;

  //Status and operation of state machine
  ModuleOperations operation;
  ModuleStatus status;
  bool operation_result;

  //UART INT stop flag
  uint8_t stop_flag;

  //Move settings
  EDriveMode position_mode;
  int16_t  position;
  uint16_t speed;
  uint16_t force;

  //Timeout counter (when no requests from PC)
  uint32_t timeout_counter;

  //Measured data
  float measured_radius;
  float measured_stiffness;
  float measured_emg;
  float measured_radius_delta;

} ModuleData;


void uart_process(void);


#endif
