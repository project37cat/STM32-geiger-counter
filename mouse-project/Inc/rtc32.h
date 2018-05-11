/*
 * rtc.h
 *
 * 12-03-2018  4mouse
 *
 */

#ifndef RTC32_H_
#define RTC32_H_


#include "stm32f1xx_hal.h"



#define RTC_CNTH_ADDR  0x40002818
#define RTC_CNTL_ADDR  0x4000281C

#define RTC_CRL_ADDR   0x40002804

#define RTC_CRH_ADDR   0x40002800


//-------------------------------------------------------------------------------------------------
uint32_t get_rtc_cnt(void)
    {
    uint16_t low = 0;
    uint16_t high1 = 0;
    uint16_t high2 = 0;
    uint32_t rtc = 0;

    high1 = ((*(volatile uint32_t*)RTC_CNTH_ADDR) & 0xFFFFU);
    low   = ((*(volatile uint32_t*)RTC_CNTL_ADDR) & 0xFFFFU);
    high2 = ((*(volatile uint32_t*)RTC_CNTH_ADDR) & 0xFFFFU);

    if(high1 != high2)
        { rtc = (((uint32_t)high2 << 16U) | ((*(volatile uint32_t*)RTC_CNTL_ADDR) & 0xFFFFU)); }
    else
        { rtc = (((uint32_t)high1 << 16U) | low); }

    return rtc;
    }

//-------------------------------------------------------------------------------------------------
void write_rtc_cnt(uint32_t data)
    {
    while(((*(volatile uint32_t*)RTC_CRL_ADDR) & RTC_CRL_RTOFF) == (uint32_t)RESET);

    SET_BIT(*(volatile uint32_t*)RTC_CRL_ADDR, RTC_CRL_CNF);

    (*(volatile uint32_t*)RTC_CNTH_ADDR) = (data >> 16U);
    (*(volatile uint32_t*)RTC_CNTL_ADDR) = (data & 0xFFFFU);

    CLEAR_BIT(*(volatile uint32_t*)RTC_CRL_ADDR, RTC_CRL_CNF);

    while(((*(volatile uint32_t*)RTC_CRL_ADDR) & RTC_CRL_RTOFF) == (uint32_t)RESET);
    }


//=================================================================================================

#define DATE_BASE 2451911 // 01 01 2001

typedef struct
{
  uint16_t year;
  uint8_t  month;
  uint8_t  mday;
  uint8_t  wday;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;

} srtc_date_typedef;


//-------------------------------------------------------------------------------------------------
uint32_t date_to_counter(srtc_date_typedef *srtc)
    {
    uint32_t c;

    uint8_t  a = (14-srtc->month)/12;
    uint8_t  m = srtc->month+(12*a)-3;
    uint16_t y = srtc->year+4800-a;

    c = srtc->mday;
    c += (153*m+2)/5;
    c += 365*y;
    c += y/4;
    c += -y/100;
    c += y/400;
    c += -32045;

    c += -DATE_BASE;
    c *= 86400;
    c += (srtc->hour*3600);
    c += (srtc->minute*60);
    c += (srtc->second);

    return c;
    }

//-------------------------------------------------------------------------------------------------
void counter_to_date(uint32_t counter, srtc_date_typedef *srtc)
    {
    uint32_t a;
    uint8_t b;
    uint8_t d;
    uint8_t m;

    srtc->wday = (counter/86400+1) % 7;

    a = (counter/86400)+32044+DATE_BASE;
    b = (4*a+3)/146097;
    a = a-((146097*b)/4);
    d = (4*a+3)/1461;
    a = a-((1461*d)/4);
    m = (5*a+2)/153;
    srtc->mday = a-((153*m+2)/5)+1;
    srtc->month = m+3-(12*(m/10));
    srtc->year = 100*b+d-4800+(m/10);
    srtc->hour = (counter/3600)%24;
    srtc->minute = (counter/60)%60;
    srtc->second = (counter%60);
    }


//xxx
//RTC_BKP_DR1 - RTC_BKP_DR10
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
uint32_t read_backup_reg(RTC_HandleTypeDef *hrtc, uint32_t reg)
  {
    return HAL_RTCEx_BKUPRead(hrtc, reg);
  }

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void write_backup_reg(RTC_HandleTypeDef *hrtc, uint32_t reg, uint32_t data)
  {
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_RTCEx_BKUPWrite(hrtc, reg, data);
    HAL_PWR_DisableBkUpAccess();
    __HAL_RCC_BKP_CLK_DISABLE();
  }
/*
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void sys_save_date_bkup(void)
    {
    HAL_RTC_GetDate(&hrtc, &currdate, RTC_FORMAT_BIN);

    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, currdate.WeekDay);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, currdate.Year);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, currdate.Month);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, currdate.Date);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR6, 0x3737);  //flag
    HAL_PWR_DisableBkUpAccess();
    __HAL_RCC_BKP_CLK_DISABLE();
    }

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
void sys_read_date_bkup(void)
    {
    currdate.WeekDay = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
    currdate.Year = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
    currdate.Month = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);
    currdate.Date = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);

    HAL_RTC_SetDate(&hrtc, &currdate, RTC_FORMAT_BIN);
    }
*/


#endif /* RTC32_H_ */
