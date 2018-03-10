// Время нажатия на кнопку - 2 секунды:
#define PRESS_DELAY 2000
// Время, через которое MMC отключился после выключения зажигания (5 секунд):
#define MMC_OFF_TIMEOUT_AFTER_ACC_OFF 5000 //1000*5;
// Время, через которое MMC отключится после ручного включения и выключенного зажигания (40 минут):
#define MMC_OFF_TIMEOUT_AFTER_MANUAL_ON_AND_ACC_OFF 2400000 //1000*60*40;

// Pins config
#define ACC_INPUT 2
#define USB_POWER 3
#define MAIN_BUTTON 4
#define LED 1

int mmc_status=0;
int acc_status=0;

#define ON 1
#define OFF 0

enum states {
  MMC_OFF=0,
  MMC_NEED_START=1,
  MMC_ON=2,
  MMC_NEED_OFF=3,
  MMC_MANUAL_OFF=4,
  MMC_MANUAL_ON=5
} st;

int state=MMC_OFF;

void setup()
{  
  pinMode(ACC_INPUT, INPUT);
  pinMode(MAIN_BUTTON, INPUT);
  pinMode(USB_POWER, INPUT);
  pinMode(LED, OUTPUT);
}

int press_key(void)
{
  pinMode(MAIN_BUTTON, OUTPUT);
  digitalWrite(MAIN_BUTTON, LOW);
  digitalWrite(LED,HIGH);

  delay(PRESS_DELAY);
  pinMode(MAIN_BUTTON, INPUT);
  digitalWrite(LED,LOW);
}

int checkStateMachine()
{
  acc_status=digitalRead(ACC_INPUT);
  mmc_status=digitalRead(USB_POWER);

  switch (state)
  {
    case MMC_OFF:
    {
      // Проверяем, не включили ли вручную?
      if (mmc_status==ON)
      {
        // включили вручную, обновляем статус:
        state=MMC_MANUAL_ON;
      }
      else
      {
        // mmc не включена, проверяем, не появилось ли питание на ACC:
        if(acc_status==ON)
        {
          // на АСС появилось питание - значит нужно зупускать MMC:
          state=MMC_NEED_START;
        }
      }
      break;
    }
    case MMC_NEED_START:
    {
      if(mmc_status==ON)
      {
        // уже включено:
        state=MMC_ON;
      }
      else
      {
        // нажимаем кнопку включения:
        press_key();
        // ждём 3 секунды:
        delay(3000);
      }
      break;
    }
    case MMC_ON:
    {
      if(mmc_status==OFF)
      {
        // mmc выключена вручную
        state=MMC_MANUAL_OFF;
      }
      else if(acc_status==OFF)
      {
        // Значит нужно выключать MMC:
        // делаем паузу перед отключением:
        for(unsigned long t=0;t<MMC_OFF_TIMEOUT_AFTER_ACC_OFF;t+=500)
        {
          acc_status=digitalRead(ACC_INPUT);
          mmc_status=digitalRead(USB_POWER);
          if(acc_status==ON||mmc_status==OFF)break;
          delay(500);
        }
        // Обновляем текущий статус ACC:
        acc_status=digitalRead(ACC_INPUT);
        if(acc_status==OFF)
        {
          // переводим в режим необходимости выключения:
          state=MMC_NEED_OFF;
        }
      }
      break;
    }
    case MMC_NEED_OFF:
    {
      if(mmc_status==OFF)
      {
        // mmc уже выключено
        state=MMC_OFF;
      }
      else
      {
        // выключаем mmc:
        press_key();
        // ждём 5 секунды:
        delay(5000);
      }
      break;
    }
    case MMC_MANUAL_OFF:
    {
      // в этом режиме не включаем автоматически MMC даже при наличии ACC
      if (mmc_status==ON)
      {
        // опять включили вручную - обновляем статус:
        state=MMC_MANUAL_ON;
      }
      break;
    }
    case MMC_MANUAL_ON:
    {
      if(mmc_status==ON)
      {
        if(acc_status==ON)
        {
          //  ACC включено - штатный режим
          // переводим в обычный режим:
          state=MMC_ON;
        }
        else
        {
          // ACC выключено, но MMC включили вручную.
          // ждём 40 минут и отключаем MMC:
          for(unsigned long t=0;t<MMC_OFF_TIMEOUT_AFTER_MANUAL_ON_AND_ACC_OFF;t+=500)
          {
            acc_status=digitalRead(ACC_INPUT);
            mmc_status=digitalRead(USB_POWER);
            if(acc_status==ON||mmc_status==OFF)break;
            delay(500);
          }
          // Обновляем текущий статус ACC:
          acc_status=digitalRead(ACC_INPUT);
          if(acc_status==OFF)
          {
            // ACC всё ещё выключенно, значит выключаем MMC:
            state=MMC_NEED_OFF;
          }
        }
      }
      else
      {
        if (acc_status==ON)
        {
          // ACC включено, но MMC выключили вручную, чтобы исключить навязчивое автоматическое включение, переводим в режим
          // ручного выключенного:
          state=MMC_MANUAL_OFF;
        }
        else
        {
          // всё выключено - переводим в обычный выключенный режим 
          state=MMC_OFF;
        }
      }
      break;
    }
  }

  return 0;
}

void loop()
{
  checkStateMachine();
}