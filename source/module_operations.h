#ifndef __MODULE_OPERATIONS__
#define __MODULE_OPERATIONS__


#include <avr/io.h>
#include <stdbool.h>
#include "module_drive.h"


extern float measured_radius;
extern float measured_stiffness;
extern float measured_emg;


bool operation_move(void);
bool operation_touch_and_measure(void);
bool operation_touch(void);
bool operation_move_back(void);
bool operation_move_forward(void);

bool operation_wait_trigger(EDriveTrigger trigger);
bool operation_wait_status(EDriveStatus status);


#endif
