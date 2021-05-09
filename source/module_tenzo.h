#ifndef __MODULE_TENZO__
#define __MODULE_TENZO__

#include <avr/io.h>

//----------------------------------------------

//DDRx |= [0-in; 1-out]
//(if DDRx==0) PORTx |= [0-HI-Z; 1-PullUp100k]
//(if DDRx==1) PORTx |= [0-low; 1-high]
//PINx reads pins, no matter of DDR or PORT regs

#define PORT_TENZO        PORTD
#define DDR_TENZO         DDRD
#define INPUT_TENZO       PIND
#define PIN_CLK           6
#define PIN_DTA           5

//----------------------------------------------

typedef enum TENZO_CHANNEL
{
  A_128 = 1,
  A_64  = 3,
  B_32  = 2

} TENZO_CHANNEL;

//----------------------------------------------


extern volatile int16_t tenzo_value;
extern volatile int16_t tenzo_force;
extern volatile float tenzo_coeff;


void     tenzo_init(void);
void     tenzo_set_channel(TENZO_CHANNEL channel);
int16_t  tenzo_get_force(void);
int32_t  tenzo_get_value(void);
void     tenzo_to_zero(void);
void     tenzo_process(void);

#endif
