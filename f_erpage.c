#include <ADUC842.H>
#include "f_adi.h"

/*---------------------------------------------------------
---------------------------------------------------------*/
unsigned long  flash_erase_page (unsigned long address)
{
unsigned int addr;

if (address >= ADI_EEMEM_SIZE)
  return (0UL);
	addr = (unsigned int) address;
	EADRL = addr >> 2;
	ECON = ADI_EE_ERASE_PAGE ;
return (1);
}

/*---------------------------------------------------------
---------------------------------------------------------*/