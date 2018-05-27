// stm32f103 internal RTC
//
// rtc32.h // Mouse project // 25-May-2018
//


#include "stdint.h"


#ifndef RTC32_H_
#define RTC32_H_


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


uint32_t get_rtc_cnt(void);
void write_rtc_cnt(uint32_t data);
uint32_t date_to_counter(srtc_date_typedef *srtc);
void counter_to_date(uint32_t counter, srtc_date_typedef *srtc);


#endif /* RTC32_H_ */

