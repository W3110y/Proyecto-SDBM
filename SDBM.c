#include "SDBM.h"
extern UART_HandleTypeDef huart2;
int _write(int file, char *ptr, int len) {
 int i=0;
 for(i=0; i<len; i++)
 HAL_UART_Transmit(&huart2,(uint8_t *)(ptr++),1,1000);
 return len;
 }
void waiting(unsigned int delay) {
 unsigned int i;
 for (i=0; i<delay; i++);
 }
void Bin2Ascii(signed short numero, unsigned char* cadena){

	signed short parcial, cociente, divisor;
	unsigned short i;

  if (numero<0)
	{
		numero = numero*(-1);
		*(cadena)='-';
	}
  else
	{
		*(cadena)='0';
	}
	parcial = numero;
  divisor = 10000;

	for (i=1; i<6; i++){
      cociente = parcial/divisor;
      *(cadena+i) = '0'+(unsigned char)cociente;
      parcial = parcial - (cociente * divisor);
      divisor = divisor / 10;
      }
}

