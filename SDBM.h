#include "stm32l1xx_hal.h"
extern UART_HandleTypeDef huart2;
int _write(int file, char *ptr, int len);
void waiting(unsigned int delay);
void Bin2Ascii(signed short numero, unsigned char* cadena);