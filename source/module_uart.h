#ifndef __MODULE_UART__
#define __MODULE_UART__

#include <avr/io.h>
#include <stdbool.h>


#ifndef F_CPU
  #define F_CPU                16000000UL
#endif

//#define UART_SPEED             115200UL
//#define UART_SPEED_DIVIDER     (F_CPU/4/UART_SPEED - 1)/2

//#define UART_SPEED_DIVIDER     25 //76800
#define UART_SPEED_DIVIDER     16 //115200
//#define UART_SPEED_DIVIDER     7 //250000

/*char    uart_str_buff[128];
#define uprintf(...){ sprintf(uart_str_buff, __VA_ARGS__ ); uart_print(uart_str_buff); }*/


//Rx data array
typedef struct UartData
{
  union {
    struct { uint16_t command; uint16_t argument;  uint16_t tail; };
    struct { uint16_t command; uint32_t value32; };
    struct { uint16_t command; uint16_t value16; };
    struct { uint16_t command; uint8_t value8; };
    struct { uint16_t command; float valueF; };

    uint8_t buffer[6];
  };

  uint8_t count;

} UartData;


extern volatile UartData uart_data;


void uart_init(void);
void uart_print(char* str);
void uart_answer(uint8_t len);
void uart_write(uint8_t* data, uint8_t len);


#endif
