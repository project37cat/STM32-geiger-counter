// stm32f103 internal RTC
//
// rtc32.c // Mouse project // 25-May-2018
//


#include "stm32f1xx_hal.h"

#include "rtc32.h"


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

