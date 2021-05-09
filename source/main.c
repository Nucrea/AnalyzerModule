#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "main.h"
#include "module_uart.h"
#include "module_tenzo.h"
#include "module_drive.h"
#include "module_operations.h"


//State machine data
volatile ModuleData module_data;/* = {
  .operation = OPERATION_MOVE_BACK
};*/


int main(void)
{
  uart_init();
  tenzo_init();
  drive_init();

  module_data.board_id = eeprom_read_byte(BOARD_ID_ADDR);

  while(true)
  {
    while (module_data.operation == OPERATION_NONE) {
      uart_process();
      tenzo_process();
    }

    module_data.stop_flag = 0;
    module_data.status = STATUS_EXECUTING;

    //To stop operations, stop_flag var is used
    switch(module_data.operation) {
      case OPERATION_TOUCH_AND_MEASURE: module_data.operation_result = operation_touch_and_measure(); break;
      case OPERATION_TOUCH: module_data.operation_result = operation_touch(); break;
      case OPERATION_MOVE_FORWARD: module_data.operation_result = operation_move_forward(); break;
      case OPERATION_MOVE_BACK: module_data.operation_result = operation_move_back(); break;
      case OPERATION_MOVE: module_data.operation_result = operation_move(); break;
      default: module_data.operation_result = false; break;
    }

    module_data.status = module_data.operation_result? STATUS_OK : STATUS_ERROR;
    module_data.status = module_data.stop_flag? STATUS_STOPPED : module_data.status;
    module_data.stop_flag = 0;
    module_data.timeout_counter = 0;
    module_data.operation = OPERATION_NONE;
  }
}


void uart_process(void)
{
  if (uart_data.count < 4){
    return;
  }

  //----------------------------------------------
  //Установить режим позиционирования (Для OPERATION_MOVE)
  if (uart_data.command >= 1 && uart_data.command <= 4)
  {
    switch(uart_data.command) {
      case 1: module_data.position_mode = uart_data.argument; break;
      case 2: module_data.position = uart_data.argument; break;
      case 3: module_data.speed = uart_data.argument; break;
      case 4: module_data.force = uart_data.argument; break;
    }
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Эхо (ответ = 1 байт)
  else if (uart_data.command == CMD_ECHO) {
    uart_data.value8 = 5;
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Остановка работы (ответ = 1 байт)
  else if (uart_data.command == CMD_STOP) {
    drive_stop();
    module_data.stop_flag = 1;
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Запуск операции (ответ = 1 байт)
  else if (uart_data.command == CMD_START_OPERATION) {
    module_data.operation = uart_data.argument;
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Запись номера платы (ответ = 1 байт)
  else if (uart_data.command == CMD_SET_BOARD_ID) {
    eeprom_write_byte(BOARD_ID_ADDR, uart_data.value8);
    module_data.board_id = uart_data.value8 = eeprom_read_byte(BOARD_ID_ADDR);
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //Чтение номера платы (ответ = 1 байт)
  else if (uart_data.command == CMD_READ_BOARD_ID) {
    uart_data.value8 = module_data.board_id;
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Чтение данных датчиков, положения, скорости
  else if (uart_data.command == CMD_READ_SENSORS)
  {
    uint8_t answer_count = sizeof(uint8_t);

    switch(uart_data.argument) {
      case 1: uart_data.value8 = BUTTON_STATE_BACK(); break;
      case 2: uart_data.value8 = BUTTON_STATE_FORWARD(); break;
      case 3: uart_data.value8 = BUTTON_STATE_TOUCH(); break;
      case 4: uart_data.value16 = tenzo_force; answer_count = 2; break;
      case 5: uart_data.valueF = drive_get_speed(); answer_count = 4; break;
      case 6: uart_data.valueF = drive_get_position(); answer_count = 4; break;
    }

    uart_answer(answer_count);
    module_data.timeout_counter = 0;
  }

  //----------------------------------------------
  //Чтение рабочих данных
  else if (uart_data.command == CMD_READ_STATUS)
  {
    uint8_t answer_count = sizeof(uint8_t);

    switch(uart_data.argument) {
      case 1: uart_data.value8 = ((module_data.operation == OPERATION_NONE)? 1 : 0); break;
      case 2: uart_data.value8 = module_data.status; break;
      case 3: uart_data.value8 = drive_get_trigger(); break;
      case 4: uart_data.valueF = module_data.measured_radius; answer_count = 4; break;
      case 5: uart_data.valueF = module_data.measured_stiffness; answer_count = 4; break;
      case 6: uart_data.valueF = module_data.measured_emg; answer_count = 4; break;
      case 7: uart_data.valueF = module_data.measured_radius_delta; answer_count = 4; break;
    }

    uart_answer(answer_count);
    module_data.timeout_counter = 0;
  }

  //Обнулить показания тензодатчика
  else if (uart_data.command == CMD_TENZO_TO_ZERO) {
    tenzo_to_zero();
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //Установить первую часть коэффициента
  else if (uart_data.command == CMD_SET_TENZO_COEFF_L) {
    eeprom_write_word(TENZO_COEFF_ADDR, uart_data.argument);
    uart_answer( sizeof(uint16_t) );
    module_data.timeout_counter = 0;
  }

  //Установить вторую часть коэффициента
  else if (uart_data.command == CMD_SET_TENZO_COEFF_H) {
    eeprom_write_word(TENZO_COEFF_ADDR + 2, uart_data.argument);
    tenzo_coeff = (volatile float) eeprom_read_float(TENZO_COEFF_ADDR);
    uart_answer( sizeof(uint16_t) );
    module_data.timeout_counter = 0;
  }

  //Просто вращение вперед с заданной скоростью
  else if (uart_data.command == CMD_ROTATE_FORWARD) {
    drive_stop();
    drive_rotate(FORWARD, uart_data.argument);
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  //Просто вращение назад с заданной скоростью
  else if (uart_data.command == CMD_ROTATE_BACK) {
    drive_stop();
    drive_rotate(BACK, uart_data.argument);
    uart_answer( sizeof(uint8_t) );
    module_data.timeout_counter = 0;
  }

  uart_data.count = 0;
}
