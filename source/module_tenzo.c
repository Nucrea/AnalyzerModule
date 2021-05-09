#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include "main.h"
#include "module_tenzo.h"
#include "module_uart.h"


#define TENZO_CLK_HIGH() PORT_TENZO |= (1<<PIN_CLK);
#define TENZO_CLK_LOW()  PORT_TENZO &= ~(1<<PIN_CLK);

#define IMPULSE_DELAY() for(volatile register uint8_t i=0; i<2; ++i)

//------------------------------------------
union {
  uint8_t array[2];
  int16_t val16;

} static volatile tenzo_value_unfiltered;


static volatile TENZO_CHANNEL tenzo_channel = A_128;//A_64;
static volatile int16_t tenzo_offset = 0; //2199;

volatile float tenzo_coeff = 0.7;  //18.856;
volatile int16_t tenzo_value = 0;
volatile int16_t tenzo_force = 0;
//------------------------------------------

//------------------------------------------
static inline uint8_t tenzo_is_ready(void);
static inline uint8_t tenzo_read_byte(void);
static inline void tenzo_set_gain(uint8_t gain);
static inline void tenzo_read(void);
//------------------------------------------


//------------------------------------------//
//------------- TENZO FUNCTIONS ------------//
//------------------------------------------//

void tenzo_init(void)
{
  cli();
  DDR_TENZO &= ~(1<<PIN_CLK | 1<<PIN_DTA);
  DDR_TENZO |= 1<<PIN_CLK | 0<<PIN_DTA;
  PORT_TENZO &= ~(1<<PIN_CLK | 1<<PIN_DTA);
  sei();

  tenzo_coeff = eeprom_read_float(TENZO_COEFF_ADDR);

  tenzo_to_zero();
}

void tenzo_set_channel(TENZO_CHANNEL channel) {
  tenzo_channel = channel;
}

int16_t tenzo_get_force(void) {
  return tenzo_force;
}

int32_t tenzo_get_value(void) {
  return tenzo_value;
}

void tenzo_to_zero(void)
{
  uint8_t measures_count = 0;
  while(measures_count <= 4)
  {
    if ( tenzo_is_ready() ) {
      tenzo_process();
      measures_count++;
    }
  }

  cli();
  tenzo_offset = tenzo_value;
  sei();
}


void tenzo_process(void)
{
  if ( tenzo_is_ready() )
  {
    tenzo_read();
    tenzo_value = (tenzo_value >> 1) + (tenzo_value_unfiltered.val16 >> 1);   //low-pass filter

    cli();
    tenzo_force = tenzo_coeff * (tenzo_value - tenzo_offset);
    sei();
  }
}


//------------------------------------------//
//------------ STATIC FUNCTIONS ------------//
//------------------------------------------//

static inline uint8_t tenzo_is_ready(void) {
  return (INPUT_TENZO & (1<<PIN_DTA) ) == 0;
}


static inline uint8_t tenzo_read_byte(void)
{
  volatile register uint8_t value = 0;

  for (volatile register uint8_t i=0; i<8; ++i) {
    TENZO_CLK_HIGH();
    IMPULSE_DELAY();
    TENZO_CLK_LOW();
    value = (value << 1) | ( (INPUT_TENZO & (1<<PIN_DTA))? 1 : 0 );
    IMPULSE_DELAY();
  }

  return value;
}


static inline void tenzo_set_gain(uint8_t gain)
{
  for(register uint8_t i=0; i<gain; ++i) {
    TENZO_CLK_HIGH();
    IMPULSE_DELAY();
    TENZO_CLK_LOW();
    IMPULSE_DELAY();
  }
}


static inline void tenzo_read(void)
{
  tenzo_value_unfiltered.array[1] = tenzo_read_byte();
  tenzo_value_unfiltered.array[0] = tenzo_read_byte();
  tenzo_read_byte();
  tenzo_set_gain( (uint8_t)tenzo_channel );
}
