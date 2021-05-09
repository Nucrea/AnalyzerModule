#include <avr/io.h>
#include <avr/interrupt.h>
#include "module_uart.h"


volatile UartData uart_data = {0};


void uart_init(void)
{
  UCSR0A = 1<<U2X0;
  UBRR0 = UART_SPEED_DIVIDER;
  UCSR0B = 1<<TXEN0 | 1<<RXEN0;
  UCSR0C = 1<<UCSZ00 | 1<<UCSZ01;
  UCSR0B |= 1<<RXCIE0;     //UART rx interrupt

  TCCR0A = 0;
  TIMSK0 |= 1<<TOIE0;     //Ovf interrupt*/
  sei();
}


void uart_print(char* str)
{
  while(*str) {
    while( !(UCSR0A & (1<<UDRE0)) );
    UDR0 = *str++;
  }
}


void uart_answer(uint8_t len)
{
  volatile uint8_t* ptr = uart_data.buffer;
  len += 2;

  while(len--) {
    while( !(UCSR0A & (1<<UDRE0)) );
    UDR0 = *ptr++;
  }
}


void uart_write(uint8_t* arr, uint8_t len)
{
  while(len--) {
    while( !(UCSR0A & (1<<UDRE0)) );
    UDR0 = *arr++;
  }
}


//Timeout timer
ISR(TIMER0_OVF_vect) {
  uart_data.count = 0;
  TCCR0B = 0;
}


//UART Rx int
ISR(USART_RX_vect)
{
  if (uart_data.count >= 4) {
    TCNT0 = 0;
    TCCR0B = 1<<CS02;
    volatile uint8_t dummy = UDR0;
    return;
  }

  //Read uart byte
  uart_data.buffer[uart_data.count++] = UDR0;

  TCNT0 = 0;
  TCCR0B = 1<<CS02;
}
