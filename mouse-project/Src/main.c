/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Includes */

#include "SSD1306.h"
#include "rtc32.h"

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

#define BOARD_LED_OFF  (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET))
#define BOARD_LED_ON   (HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET))

#define BAT_DIV_OFF  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_SET))
#define BAT_DIV_ON  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, GPIO_PIN_RESET))

#define HV_OFF  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET))
#define HV_ON  (HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET))

#define BUTTON_1  (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0))
#define BUTTON_2  (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1))


#define GEIGER_TIME 75

uint16_t radimp[GEIGER_TIME+1];   //pulse counter

uint32_t glog[128];

extern char strbuff[32];

volatile uint32_t gf=0;      // flag - auto 1Hz - got geiger counter data
volatile uint32_t gcnt=0;    // counter - geiger counter data for the previous second
volatile uint32_t sescnt=0;  // counter - duration of the current session

/*
//%%%%%%%%%%%%%%%%%%%%%  temp for debug  %%%%%%%%%%%%%%%
uint32_t d_t1;
uint32_t d_t2;
uint32_t d_t3;
uint32_t d_t4;

uint32_t d_t5;
uint32_t d_t6;
uint32_t d_t7;
uint32_t d_t8;
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
*/
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

uint16_t get_bat(uint16_t vdd);
uint16_t get_vdd(void);
uint16_t get_adc(uint32_t channel, uint16_t samples);
void sys_standby(void);

void sys_init(void);
uint8_t button1_h(void);
uint8_t button2_h(void);

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

//-----------------------------------------------------------------------------
uint16_t get_adc(uint32_t channel, uint16_t samples)
    {

    ADC_ChannelConfTypeDef sConfig;

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    uint32_t tempdata = 0;

    for (uint16_t n=0; n<samples; n++)
      {
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, 100);
        if(n) tempdata += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
      }

    tempdata /= samples-1;

    return (uint16_t)tempdata;
    }

//-----------------------------------------------------------------------------
uint16_t get_vdd(void)
    {
    uint16_t vrefint = get_adc(ADC_CHANNEL_VREFINT, 256);

    static uint16_t olddata = 0;

    if ((olddata == 0)
     || (vrefint == 0)
     || (vrefint == 4095)
     || (vrefint > olddata + 1)
     || (vrefint < olddata - 1))  { olddata = vrefint; }
//d_t1=vrefint;
    uint16_t vdd = 0;
    if(olddata) vdd=((1207*4095)/olddata);
    return vdd;
    }

//-----------------------------------------------------------------------------
uint16_t get_bat(uint16_t vdd)
    {
    uint16_t battery = get_adc(ADC_CHANNEL_9, 256);

    static uint16_t olddata = 0;

    if ((olddata == 0)
     || (battery == 0)
     || (battery == 4095)
     || (battery > olddata + 1)
     || (battery < olddata - 1))  { olddata = battery; }
//d_t2=battery;
    uint16_t bat = ((olddata*vdd)/4095)*2.010;
    return bat;
    }


//-----------------------------------------------------------------------------
uint8_t button1_h(void)
    {
    static uint8_t old=0;
    uint8_t new = BUTTON_1;

    if(new!=old)
        {
        HAL_Delay(50);
        new = BUTTON_1;
        if(new!=old) old=new;
        }
    return old;
    }

//-----------------------------------------------------------------------------
uint8_t button2_h(void)
    {
    static uint8_t old=0;
    uint8_t new = BUTTON_2;

    if(new!=old)
        {
        HAL_Delay(50);
        new = BUTTON_2;
        if(new!=old) old=new;
        }
    return old;
    }

//xxx
//-----------------------------------------------------------------------------
void sys_backup(uint32_t newttot, uint32_t newdtot)
    {
    write_backup_reg(&hrtc, RTC_BKP_DR2, newttot>>16);  //write backup data
    write_backup_reg(&hrtc, RTC_BKP_DR3, newttot);

    write_backup_reg(&hrtc, RTC_BKP_DR4, newdtot>>16);
    write_backup_reg(&hrtc, RTC_BKP_DR5, newdtot);
    }


//-----------------------------------------------------------------------------
void draw_graph(void)
    {
    const uint8_t hight = 10;
    const uint8_t x = 0;
    const uint8_t y = 63;

    for(uint8_t k=0; k<75; k++)
        {
        uint8_t rm = radimp[k+1]/hight;
        if(rm>hight-1) rm=hight-1;

        uint8_t rm0 = radimp[k]/hight;
        if(rm0>hight-1) rm0=hight-1;

        if(k) oled_line(k-1+x,   y-(hight+1)-(radimp[k]%hight),
                        k+x,     y-(hight+1)-(radimp[k+1]%hight)); //draw top graph

        oled_line(k-1+x, y-hight+rm0+1,
                  k+x,   y-hight+rm+1);  //bottom part of graph
        }
    }


//-----------------------------------------------------------------------------
void draw_graph2(void)
    {
    const uint8_t hight = 11;
    const uint8_t y = 35;

    uint32_t max = 0;

    for(uint8_t i=0; i<128; i++)
        {
        if(glog[i]>max) max=glog[i];
        }

    uint32_t div = max/hight;

    for(uint8_t i=0; i<(128-1); i++)
        {
        uint32_t t = glog[i]/(div+1);
        uint32_t t2 = glog[i+1]/(div+1);

        oled_line(i, y-t, i+1, y-t2);

        if(i%2) oled_pixel(i, y+4);
        if(!(i%30)) oled_pixel(i+1, y+3);
        }

    oled_pixel(127, y+4);

    sprintf(strbuff, "x%lu",div+1);
    oled_print(0, y+16, strbuff);
    }


//-----------------------------------------------------------------------------
void sys_standby(void)
    {
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;  // Enable PWR module
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;  // Set SLEEPDEEP bit of Cortex System Control Register
    PWR->CR |= PWR_CR_PDDS;             // Select Standby mode
    PWR->CR |= PWR_CR_CWUF;             // Clear the WUF Wakeup Flag
    PWR->CSR |= PWR_CSR_EWUP;           // Enable WKUP pin
    __WFI();                            // Wait For Interrupt
    }


///////////////////////////////////////////////////////////////////////////////////////////////////
void sys_init(void)
    {
    HV_ON;      // ENABLE HV booster circuit
    BAT_DIV_ON; // ENABLE battery divider circuit

    HAL_ADCEx_Calibration_Start(&hadc1);  // ADC self-calibration

    HAL_TIM_Base_Start(&htim2);           // start pulse counter
    HAL_TIM_Base_Start_IT(&htim3);        // start 1sec timer

    oled_init();         // display init
    }


//=================================================================================================
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim3)  // 1 Hz interrupt
    {
    if(gf)  { gcnt += htim2.Instance->CNT; }
    else    { gcnt = htim2.Instance->CNT;  }

    htim2.Instance->CNT = 0;  //clear the cnt register

    sescnt++;

    gf=1;
    }


//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

  sys_init();

  while(button1_h());

  uint8_t df = 0x00;  //display flag

  uint8_t smode = 0x00;
  uint8_t nummen = 0;

  uint8_t ssmenu = 0;
  uint8_t dtmenu = 0;
  uint8_t crmenu = 0;

  uint16_t sysvdd = 0x0000;   //Vdd
  uint16_t sysbat = 0x0000;   //Vbat

  uint32_t rate = 0x00000000;  //doserate
  uint32_t maxrate = 0x00000000; //peak doserate

  uint32_t gses = 0x00000000;  // total of pulses in the current session

  srtc_date_typedef srtc;   //current date and time
  srtc_date_typedef newsrtc;   //new date and time

  uint32_t sestime = 0;  // duration of the current session
  uint32_t tottime = 0;  // total duration of work (from bkp)
  uint32_t sesdose = 0;  // session dose
  uint32_t totdose = 0;  // total dose (from bkp)

  tottime = (read_backup_reg(&hrtc, RTC_BKP_DR2)<<16) + read_backup_reg(&hrtc, RTC_BKP_DR3);
  totdose = (read_backup_reg(&hrtc, RTC_BKP_DR4)<<16) + read_backup_reg(&hrtc, RTC_BKP_DR5);

  while(1)
    {
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */
  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
      if(gf==1)  // calc
          {
          radimp[0] = (uint16_t)gcnt;
          for(uint8_t k=GEIGER_TIME; k>0; k--) radimp[k]=radimp[k-1]; //shift

          uint32_t tmprate=0;
          for(uint8_t k=GEIGER_TIME; k>0; k--) tmprate+=radimp[k]; //dose rate
          if(tmprate>999999) tmprate=999999; //overflow
          rate=tmprate;

          if(tmprate>maxrate) maxrate=tmprate;   //peak

          gses+=radimp[0];
          if(gses>999999UL*3600/GEIGER_TIME) gses=999999UL*3600/GEIGER_TIME;  //overflow

          sesdose = (gses*GEIGER_TIME/3600);   //dose

          sestime = sescnt;

          counter_to_date(get_rtc_cnt(), &srtc);   //get date and time

          sysvdd = get_vdd();          // get Vdd
          sysbat = get_bat(sysvdd);    // get Vbat

          glog[0]=rate;

          if(!(sestime%60)) for(uint8_t k=127; k>0; k--) glog[k]=glog[k-1]; //shift //write log //xxx

          gf=0;
          df=0;
          }

      if(df==0)  //++++++++++++++++++++++++++++++> display <+++++++++++++++++++++++++++++++++++++++
          {
          oled_clear();

          if(sysbat<3000)  // battery less than 3V
              {
              oled_print(0,10,"BATTERY LOW");
              oled_update();
              HAL_Delay(1000);
              sys_backup(tottime+sestime, totdose+sesdose);
              oled_off();
              sys_standby();
              }

          static uint8_t batlow=0;
          if(sysbat<3400) batlow=1;
          if(sysbat>3500) batlow=0;

          if(batlow) oled_line(0,10,35,10);     //// > HEADER < ////

          sprintf(strbuff, "%1u.%03u", sysbat/1000, sysbat%1000);  // print battery voltage
          oled_print(0,7,strbuff);
          oled_char(31,7,'V');

          sprintf(strbuff, "%0u-%02u-%02u", srtc.wday, srtc.mday, srtc.month);  // print date
          oled_print(48,7,strbuff);

          sprintf(strbuff, "%02u:%02u", srtc.hour, srtc.minute);  // print time
          oled_print(99,7,strbuff);

          if(smode==0)  ////////////////////////////  > main screen <   ////////////////////////////
              {
              sprintf(strbuff," %6lu uR", sesdose);
              oled_print(0,24,strbuff);

              sprintf(strbuff," %6lu uR/h", maxrate);
              oled_print(0,34,strbuff);

              draw_graph();

              sprintf(strbuff,"%lu", rate);
              if(rate<=9) oled_print16x30(94, 46, strbuff );
              else if(rate<=99) oled_print16x30(87, 46, strbuff );
              else if(rate<=999) oled_print16x30(80, 46, strbuff );
              else oled_print(87, 34, strbuff );

              uint8_t tday = (sestime/86400)%100; //99 days max
              uint8_t thour = (sestime/3600)%24;
              uint8_t tminute = (sestime/60)%60;

              sprintf(strbuff, "%02u-%02u:%02u", tday, thour, tminute);
              oled_print(81,63,strbuff);
              }

          if(smode==1 && nummen==0)   /////////////////   > secondary screen <   /////////////////////
              {
              sprintf(strbuff, "%1u.%03u", sysvdd/1000, sysvdd%1000);  // print Vdd
              oled_print(0,19,strbuff);
              oled_char(31,19,'V');

              sprintf(strbuff, "-%04u", srtc.year);
              oled_print(60,18,strbuff);

              sprintf(strbuff, ":%02u", srtc.second);
              oled_print(111,18,strbuff);

              draw_graph2();

              sprintf(strbuff, "%10lu uR", totdose+sesdose);  //tot dose
              oled_print(0,63,strbuff);

              uint32_t newttot=tottime+sestime;
              uint16_t tday = (newttot/86400)%10000; //9999 days max
              uint8_t thour = (newttot/3600)%24;
              uint8_t tminute = (newttot/60)%60;

              sprintf(strbuff, "%4u", tday);  //tot day of work  //tot time of work
              oled_print(105,52,strbuff);

              sprintf(strbuff, "%02u:%02u", thour, tminute);
              oled_print(99,63,strbuff);
              }

          if(smode==1 && nummen>0 && dtmenu==0  && crmenu==0 && ssmenu==0)  ////  draw Main menu  ////
              {
              newsrtc=srtc;  //update the data before setup entering

              oled_print(7,25,"Not Allowed");
              oled_print(7,37,"Set Date & Time");
              oled_print(7,49,"Clear Data");

              if(nummen==1) oled_char(0,25,'>');
              if(nummen==2) oled_char(0,37,'>');
              if(nummen==3) oled_char(0,49,'>');

              oled_print(98,63,"^down"); //help
              }

          if(smode==1 && nummen==1 && ssmenu>0)  ////  ////
              {
              //
              }

          if(smode==1 && nummen==2 && dtmenu>0)    ////  draw Set Date and Time menu  ////
              {
              sprintf(strbuff, "-%04u", srtc.year);
              oled_print(59,18,strbuff);

              sprintf(strbuff, ":%02u", srtc.second);
              oled_print(110,18,strbuff);

              sprintf(strbuff, "%02u-%02u", newsrtc.mday, newsrtc.month);
              oled_print(59,35,strbuff);

              sprintf(strbuff, "%02u:%02u", newsrtc.hour, newsrtc.minute);
              oled_print(98,35,strbuff);

              sprintf(strbuff, "-%04u", newsrtc.year);
              oled_print(59,46,strbuff);

              sprintf(strbuff, ":%02u", newsrtc.second);
              oled_print(110,46,strbuff);

              if(dtmenu==1) { oled_char_inv(58,35,13); }  //invert selected item
              if(dtmenu==2) { oled_char_inv(76,35,13); }
              if(dtmenu==3) { oled_char_inv(64,46,25); }
              if(dtmenu==4) { oled_char_inv(97,35,13); }
              if(dtmenu==5) { oled_char_inv(115,35,13); }
              if(dtmenu==6) { oled_char_inv(115,46,13); }

              oled_print(86,63,"^change"); //help
              }

          if(smode==1 && nummen==3 && crmenu>0)    ////  draw the Clear Data menu  ////
              {
              oled_print(7,25,"Current Dose Rate");
              oled_print(7,37,"Total Dose & Time");

              if(crmenu==1) oled_char(0,24,'>');
              if(crmenu==2) oled_char(0,36,'>');

              oled_print(93,63,"^clear"); //help
              }

          oled_update();
          df=1;
          }               // end of Display

    if(button1_h())  /////////////////////////////  button #1  ////////////////////////////////////
        {
        while(button1_h());

        if(smode==0)  //system off
            {
            sys_backup(tottime+sestime, totdose+sesdose);
            oled_off();
            sys_standby();
            }

        if(smode==1)  //enter the main menu
            {
            if(dtmenu==0 && crmenu==0 && ssmenu==0)  { if(++nummen>3) nummen=0; }  // switching the main menu items

            if(nummen==1)  //
                {
                //
                }

            if(nummen==2)  //Set Date and Time
                {
                if(dtmenu==1)  { if(++newsrtc.mday>31)   newsrtc.mday=1;    }
                if(dtmenu==2)  { if(++newsrtc.month>12)  newsrtc.month=1;   }
                if(dtmenu==3)  { if(++newsrtc.year>2050) newsrtc.year=2018; }
                if(dtmenu==4)  { if(++newsrtc.hour>23)   newsrtc.hour=0;    }
                if(dtmenu==5)  { if(++newsrtc.minute>59) newsrtc.minute=0;  }
                if(dtmenu==6)  { newsrtc.second=0; }
                }

            if(nummen==3)  //Clear Data
                {
                if(crmenu==1) //clear current dose rate
                    {
                    for(uint8_t k=0; k<(GEIGER_TIME+1); k++) radimp[k]=0;
                    maxrate=0;
                    rate=0;
                    crmenu=0; //clr for exit
                    nummen=0;
                    smode=0;
                    }

                if(crmenu==2) //clear total dose and time
                    {
                    sescnt=0;  //clr timer
                    sestime=0;
                    tottime=0;
                    gses=0;    //clr dose
                    sesdose=0;
                    totdose=0;
                    crmenu=0;  //clr for exit
                    nummen=0;
                    }
                }

            df=0;
            }
        }

    if(button2_h())  ////////////////////////////  button #2  /////////////////////////////////////
        {
        while(button2_h());

        if(smode==0 || (smode==1 && nummen==0))  // switching screens
            {
            if(++smode>1) smode=0;
            }

        if(smode==1 && nummen==1)
            {
            //
            }

        if(smode==1 && nummen==2)  // switching the Set Time and Date menu items
            {
            if(++dtmenu>6)
                {
                dtmenu=0;
                write_rtc_cnt(date_to_counter(&newsrtc));  // save data
                srtc=newsrtc;
                }
            }

        if(smode==1 && nummen==3)  // switching the Clear Menu items
            {
            if(++crmenu>2)
                {
                crmenu=0;
                }
            }

        df=0;
        }

    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);  //power down
    }
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_ADC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Common config 
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure Regular Channel 
    */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* RTC init function */
static void MX_RTC_Init(void)
{

  RTC_TimeTypeDef sTime;
  RTC_DateTypeDef DateToUpdate;

    /**Initialize RTC Only 
    */
  hrtc.Instance = RTC;
  if(HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0x32F2){
  hrtc.Init.AsynchPrediv = RTC_AUTO_1_SECOND;
  hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initialize RTC and set the Time and Date 
    */
  sTime.Hours = 1;
  sTime.Minutes = 0;
  sTime.Seconds = 0;

  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  DateToUpdate.WeekDay = RTC_WEEKDAY_MONDAY;
  DateToUpdate.Month = RTC_MONTH_JANUARY;
  DateToUpdate.Date = 1;
  DateToUpdate.Year = 1;

  if (HAL_RTC_SetDate(&hrtc, &DateToUpdate, RTC_FORMAT_BIN) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    HAL_RTCEx_BKUPWrite(&hrtc,RTC_BKP_DR1,0x32F2);
  }

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 0xffff;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_ETRMODE2;
  sClockSourceConfig.ClockPolarity = TIM_CLOCKPOLARITY_INVERTED;
  sClockSourceConfig.ClockPrescaler = TIM_CLOCKPRESCALER_DIV1;
  sClockSourceConfig.ClockFilter = 0;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM3 init function */
static void MX_TIM3_Init(void)
{

  TIM_ClockConfigTypeDef sClockSourceConfig;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 15999;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11|GPIO_PIN_3, GPIO_PIN_SET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA2 PA3 PA4 PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB11 PB3 */
  GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  BOARD_LED_ON;
  while(1)
  {
  }
  //^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
