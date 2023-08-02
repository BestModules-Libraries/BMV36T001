#if defined(__AVR_ATmega328P__)
#else
    #include "Arduino.h"
    #include "ht32.h"
    #include "mTM.h"

    void (*tmHandle)();

    /*********************************************************************************************************//**
      * @brief   Configures TM for delay function.
      * @retval  None
      * @details Configuration as "TM_FREQ_HZ" Hz.
      ***********************************************************************************************************/
    void mTM_Configuration(uint32_t freq, void(*func)())
    {
      #if 0 // Use following function to configure the IP clock speed.
      CKCU_SetPeripPrescaler(CKCU_PCLK_GPTM0, CKCU_APBCLKPRE_DIV2);
      #endif

      { /* Enable peripheral clock                                                                              */
        CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
        CKCUClock.Bit.GPTM0 = 1;
        CKCU_PeripClockConfig(CKCUClock, ENABLE);
      }

      { /* Time base configuration                                                                              */

        /* !!! NOTICE !!!
          Notice that the local variable (structure) did not have an initial value.
          Please confirm that there are no missing members in the parameter settings below in this function.
        */
        TM_TimeBaseInitTypeDef TimeBaseInit;

        TimeBaseInit.Prescaler = 1 - 1;                         // Timer clock = CK_AHB / 1
        TimeBaseInit.CounterReload = SystemCoreClock / freq - 1;
        TimeBaseInit.RepetitionCounter = 0;
        TimeBaseInit.CounterMode = TM_CNT_MODE_UP;
        TimeBaseInit.PSCReloadTime = TM_PSC_RLD_IMMEDIATE;
        TM_TimeBaseInit(HT_GPTM0, &TimeBaseInit);

        /* Clear Update Event Interrupt flag since the "TM_TimeBaseInit()" writes the UEV1G bit                 */
        TM_ClearFlag(HT_GPTM0, TM_FLAG_UEV);
      }

      /* Enable Update Event interrupt                                                                          */
      NVIC_EnableIRQ(GPTM0_IRQn);
      TM_IntConfig(HT_GPTM0, TM_INT_UEV, ENABLE);

      TM_Cmd(HT_GPTM0, ENABLE);
      tmHandle = func;
    }

    void stopTimer(void)
    {
      TM_SetCounter(HT_GPTM0, 0);
      TM_IntConfig(HT_GPTM0, TM_INT_UEV, DISABLE);
    }

    void startTimer(void)
    {
      
      TM_IntConfig(HT_GPTM0, TM_INT_UEV, ENABLE);
    }

    void GPTM0_IRQHandler(void)
    {
    // static boolean output = HIGH;
      TM_ClearFlag(HT_GPTM0, TM_INT_UEV);

      (*tmHandle)();
        
      
      // digitalWrite(2, output);
      // output = !output;
    }
#endif
