#include "ADUC841.h"
#include <stdio.h>
#include <intrins.h>
#include "flash.h"
#define max 2.474257
#define min 0.0901//0.039063														  // Должно быть: long data Sin=0; long data Cos=0; long data dac_peremen=0; int idata usred_sin=0; и usred_sin и cos без коментариев
#define kof (max - min)  // 2.21 - max=2.25 min=0.04

unsigned int data tim_h_0=0;
unsigned int data tim_l_0=0;
	  
unsigned long data Sin=0; // значения канала 1	
unsigned long data Cos=0; // значения канала 2	
double delta=0;  // разница между первым и вторым каналом
unsigned int data kol_siemov=128; // количество усредненых значений	// char
unsigned int data ust_kol_siemov=128;									// char
bit gotov=0; // флаг готовности результата АЦП после 4 преобразований на каждом канале

unsigned int data dac_period=500;
unsigned int data schet_dac=1;
unsigned int data dac_sec=10;

unsigned int xdata period_fazi=100;
unsigned int xdata period_zaderjki=10;
float xdata verhniy_pridel=1;
float xdata nijniy_pridel=-1;
float xdata interval=0;
float xdata koef_usileniya=0;
unsigned int period=0;
long data usred_sin=0; 
long data usred_cos=0;
int data usred_sin_dac=0; 
int data usred_cos_dac=0;  
//int i=0;
//unsigned int schet_uart=250;
const float ves_razrayda=0.0006103515625;
//float uart_perem=0;
bit dac_gotov=0;
long data dac_peremen=0;
//int data dac_peremen1=0;
bit pit=0;
bit vx=0;
bit write_flash_verh=0;
bit write_flash_niz=0;
bit dac_out=1;
//------------------------------------------
float float_ceil_floor(float fl)
{
float rezult=0;

rezult=fl-(long)fl;

if(rezult>0.5)
 {
 fl=(long)(fl)+1;
 return fl;
 }
else
 {
 fl=(long)fl;
 return fl;
 }
}
//------------------------------------------
void delay(long length)	large reentrant
{
 while(length >=0)
 length--;
}	 
//------------------------------------------
void _INT0_ (void) interrupt 0
{
delay(2000);
if(INT0==0)
{
nijniy_pridel=delta+0.0006103515625;
interval=verhniy_pridel-nijniy_pridel;	  // маштабирование
koef_usileniya=kof/interval;
write_flash_niz=1;
IE0=0;
}
}
//------------------------------------------
void _INT1_ (void) interrupt 2
{
delay(2000);
if(INT1==0)
{
verhniy_pridel=delta-0.0006103515625;
interval=verhniy_pridel-nijniy_pridel;	  // маштабирование
koef_usileniya=kof/interval;
write_flash_verh=1;
IE1=0;
}
}
//------------------------------------------
void _TR2_ (void) interrupt 5
{
P3=P3^0x30;
//T0=~T0;
//T1=~T1;
//P3^=0x30;
pit^=1;
//vx=pit|rejim;
if(pit)									   
{
TH0=tim_h_0;
TL0=tim_l_0;
TR0=1;
}
TF2=0;
}


//------------------------------------------
void _TR0_ (void) interrupt 1  // задержка перед работой АЦП
{
//RD=1;
SCONV=1;
TR0=0;
EA=0;
WDWR=1;
WDE=1; 	 
EA=1;
}



//------------------------------------------
void _TR1_ (void) interrupt 3  // ЦАП	  
{
TH1=0xA9;			   // период 0.002
TL1=0x99;
if(schet_dac==dac_period)
{
 if(gotov==1)
   {
   dac_gotov=1; 
   gotov=0;
   }
schet_dac=1;
}
else
schet_dac++;	
}



//------------------------------------------
void _ADC_ (void) interrupt 6
{
 if(CS0==0)
 Sin+=(((ADCDATAH&0x0F)<<8)|ADCDATAL);
 else
 {
 Cos+=(((ADCDATAH&0x0F)<<8)|ADCDATAL);
 kol_siemov--;
 }
 CS0^=1;
 CS1=0;
    
if(kol_siemov==0)
 {
 kol_siemov=ust_kol_siemov;
 if(dac_out==1)
  {
 usred_sin=Sin>>7;// 2
 usred_cos=Cos>>7;// 2
  }
 Cos=Sin=Sin&0x00;
 gotov=1;
 }
}
//-----------------------------------------

void main (void)
{
T0=0;
T1=0;
RD=1;
RD=0;

kol_siemov=128;
ust_kol_siemov=128;

flash_read (&verhniy_pridel,sizeof(verhniy_pridel),0x00000000);
flash_read (&nijniy_pridel,sizeof(nijniy_pridel),0x00000004);

if(_chkfloat_(verhniy_pridel)==4)
verhniy_pridel=1;
if(_chkfloat_(nijniy_pridel)==4)
nijniy_pridel=-1;
DACCON=0x09;		 // 2.5 вольта
dac_period=dac_period/dac_sec;		 // количество раз в секунду


interval=verhniy_pridel-nijniy_pridel;	  // маштабирование
koef_usileniya=((float)kof)/interval;

T2CON=0x00;
TMOD=0x11; //16 - разрядный таймеры 0 и 1, внутренние стробирование
ET0=1; //разрешение прер-я от Timer0
ET1=1; //разрешение прер-я от Timer1
ET2=1; //разрешение прер-я от Timer2

period=0xFFFF-(((float)(period_fazi/2)*0.000001)/0.00000009); // период P2.6 и P2.7
RCAP2H=TH2=(period&0xFF00)>>8;	 // 100 микросекунд
RCAP2L=TL2=period&0x00FF;

period=0xFFFF-(int)(((((float)period_zaderjki*0.000001)-(32*0.00000009))/0.00000009)); // задержка после P2.6=1; 76 расчитано от переключения Р2_6 и до входа в подпрограмму ADC
tim_h_0=TH0=(period&0xFF00)>>8;	 // 20 микросекунд
tim_l_0=TL0=period&0x00FF;

TH1=0xA9;	  // для DAC период счета 0.002
TL1=0x99;

EA=1; //разрешение всех прерываний

ADCCON1=0xA0;	  // делитель 4(0x9),8(0xA), пропуск тактов 1, внутреннее опорное
ADCCON2=0x00;	 // 0 канал, однократное преобразование
EADC=1;			 //разрешение прер-я от ADC

PT0=1;
//PADC=1;
PT2=1;	// наивысший приоритет у 2 таймера
//PX0=1;
//PX1=1;

IT1=1;	// включение прерывания INT0 INT1 по фронту
IT0=1;
EX0=1;	// разрешение прерывания INT0 INT1
EX1=1;

T1=1;
T0=0;

EA=0;   
WDWR=1;
WDCON=0x02;
EA=1;

TR2=1; //включение Timer2
TR1=1; //включение Timer1

while(1)
{
 if(write_flash_verh)
    {
	flash_write (&verhniy_pridel,sizeof(verhniy_pridel),0x00000000);
	write_flash_verh=0;
	}

 if(write_flash_niz)
    {
	flash_write (&nijniy_pridel,sizeof(nijniy_pridel),0x00000004);
	write_flash_niz=0;
	} 

  if(dac_gotov==1)
    {					
	dac_out=0;
	delta=((usred_sin-usred_cos)*ves_razrayda);
	dac_out=1; 
    DACCON=0x09;
    dac_peremen=(long)float_ceil_floor(((((interval-(verhniy_pridel-delta))*koef_usileniya)+min)/ves_razrayda));

   if(dac_peremen>(((float)max/ves_razrayda)+1))
    dac_peremen=(((float)max/ves_razrayda)+1);

   if(dac_peremen<(((float)min/ves_razrayda)+1))
    dac_peremen=(((float)min/ves_razrayda)+1);

   DAC0H=(dac_peremen&0x00000F00)>>8;
   DAC0L=(dac_peremen&0x000000FF);
   DACCON=0x0D;
   dac_gotov=0;

   
	}
}
	 
}