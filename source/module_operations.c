#include <util/delay.h>
#include "main.h"
#include "module_operations.h"
#include "module_tenzo.h"
#include "module_drive.h"
#include "module_uart.h"

#define FORCE_TOUCH         15
#define FORCE_STIFFNESS     300
#define FORCE_BREAK         1000

#ifdef Z_DRIVE
  #define SPEED_TOUCH         10
  #define SPEED_TOUCH_SLOW    5
  #define SPEED_STIFFNESS     5
  #define SPEED_MAX           12
#else
  #define SPEED_TOUCH         20
  #define SPEED_TOUCH_SLOW    10
  #define SPEED_STIFFNESS     5
  #define SPEED_MAX           30
#endif

#define TIMEOUT_MAX         100000UL
#define DELTA_POS_MAX       300


extern volatile ModuleData module_data;


void operation_delay(uint16_t msecs)
{
  //Minimum delay 250ms
  while(msecs--)
  {
    if (module_data.stop_flag) { break; }
    uart_process();
    tenzo_process();
    _delay_ms(1);
  }
}


//Истина, если окончание операции с нужным триггером (выключателем)
bool operation_wait_trigger(EDriveTrigger trigger)
{
  while(drive_get_status()==EXECUTING && !module_data.stop_flag)
  {
    uart_process();
    tenzo_process();

    if (++module_data.timeout_counter >= TIMEOUT_MAX) {
      drive_stop();
      module_data.stop_flag = 1;
      break;
    }
  }

  //Minimum delay 800ms
  operation_delay(100);

  return (!module_data.stop_flag && (drive_get_trigger() == trigger) );
}

//Истина, если окончание операции с нужным результатом
bool operation_wait_status(EDriveStatus status)
{
  while(drive_get_status()==EXECUTING && !module_data.stop_flag)
  {
    uart_process();
    tenzo_process();

    if (++module_data.timeout_counter >= TIMEOUT_MAX) {
      drive_stop();
      module_data.stop_flag = 1;
      break;
    }
  }

  //Minimum delay 250ms
  operation_delay(100);

  return (!module_data.stop_flag  && (drive_get_status() == status) );
}


bool operation_move(void)
{
  drive_move(module_data.position_mode, module_data.position, module_data.speed, module_data.force);
  if ( !operation_wait_status(FINISHED) ) { return false; }
}


bool operation_touch(void)
{
  //----------------------------------------------------------------
  //Движение до касания
  drive_move(RELATIVE, DELTA_POS_MAX, SPEED_TOUCH, FORCE_TOUCH);
  return operation_wait_trigger(TENZO);
}


bool operation_touch_and_measure(void)
{
  //----------------------------------------------------------------
  //Отход назад
  //if ( !operation_move_back() ) { return false; }

  //----------------------------------------------------------------
  //Движение до касания
  drive_move(RELATIVE, DELTA_POS_MAX, SPEED_TOUCH, FORCE_TOUCH);
  if ( !operation_wait_trigger(TENZO) ) { return false; }

  //----------------------------------------------------------------
  //Отход назад на 1см
  drive_move(RELATIVE, -10, 10, FORCE_BREAK);
  if ( !operation_wait_status(FINISHED) ) { return false; }
  tenzo_to_zero();

  //----------------------------------------------------------------
  //Медленное приближение до касания, замер расстояния
  drive_move(RELATIVE, DELTA_POS_MAX/2, SPEED_TOUCH_SLOW, FORCE_TOUCH);
  if ( !operation_wait_trigger(TENZO) ) { return false; }

  module_data.measured_radius = drive_get_position();

  //----------------------------------------------------------------
  //Вдавливание, нахождение твердости
  drive_move(RELATIVE, DELTA_POS_MAX/2, SPEED_STIFFNESS, FORCE_STIFFNESS);
  if ( !operation_wait_trigger(TENZO) ) { return false; }

  module_data.measured_stiffness = 1/(drive_get_position() - module_data.measured_radius);
  module_data.measured_radius_delta = (drive_get_position() - module_data.measured_radius);
  operation_delay(100);

  //----------------------------------------------------------------
  //Возврат в ноль
  //return operation_move_back();
}


bool operation_move_back(void)
{
  drive_move(RELATIVE, -DELTA_POS_MAX, SPEED_MAX, FORCE_BREAK);
  bool result = operation_wait_trigger(BUTTON_BACK);

  if (result) {
    tenzo_to_zero();
    drive_position_to_zero();
  }

  return result;
}


bool operation_move_forward(void)
{
  drive_move(RELATIVE, DELTA_POS_MAX, SPEED_MAX, FORCE_BREAK);
  bool result = operation_wait_trigger(BUTTON_FORWARD);

  return result;
}
