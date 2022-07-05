#include "IR_Imitator.h"

//Task handle
TaskHandle_t xTaskMain;
TaskHandle_t  xTaskButtonControl;

//Analog input level for button identification
const short int ButtonsVoltage[13] {0, 15, 45, 80, 115, 160, 300, 400, 465, 610, 620, 635, 645};

const int DEFUALT_NUMBER_OF_REPEATS_TO_SEND = 3;

IRData StoredIRDataList[10];
bool ButtonStatusList[12];
char DisplayCharacter[17];
unsigned long LastPressedTime;

LiquidCrystal lcd (LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);


void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }

  WDT_Prescaler_Change();

  //Pin setup for interrupt
  pinMode(2, INPUT);

  //Set up LCD
  pinMode(LCD_BUCKLIGHT, OUTPUT);
  digitalWrite(LCD_BUCKLIGHT, HIGH);
  lcd.begin(16, 2);
  LcdDisplay(1, 0, "Wake up");

  //Read out from EEPROM
  EEPROMControler(false);

  //set up tasks to run independently.
  xTaskCreate(
    TaskMain,
    NULL,
    256,
    NULL,
    1,
    &xTaskMain);

  xTaskCreate(
    TaskButtonControl,
    NULL,
    128,
    NULL,
    2,
    &xTaskButtonControl);
}

//To sleep mode setup ONLY
void loop()
{
  //Disable Digital Input on Analogue Pins
  DIDR0 = 0x3F;

  //Disable Analogue Comparator
  ACSR &= ~_BV(ACIE);
  ACSR |= _BV(ACD);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  //Disable interrupt
  portENTER_CRITICAL();
  sleep_enable();

  //Disable BOD
  sleep_bod_disable();

  //Enable interrupt
  portEXIT_CRITICAL();
  sleep_cpu();

  sleep_reset();
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskMain(void *pvParameters) {
  (void)pvParameters;

  IrReceiver.begin(PIN_RECV);
  IrSender.begin(PIN_SEND);

  //Flag for If receive some signal, stop the receive loop untill bottun is released.
  bool receiverFlag = false;
  //Mode change Flag; False = Transmit Mode, True = Receive Mode
  bool modeFlag = false;
  bool isAnyBtnPressed, storeCntFlag, StoredFlag = false;
  int zeroBtnCnt = 0;
  for (;;) {

    isAnyBtnPressed = false;
    int i;
    for (i = 0; i < 12; i++) {

      //If pressed Button is 8:Transmit Mode 0:Receive Mode
      if (i == 8) {
        if (ButtonStatusList[8]) {
          modeFlag = false;
          LcdDisplay(1, 0, "Transmit Mode");
          zeroBtnCnt = 0;
        }
      } else if (i == 0) {
        if (ButtonStatusList[0] && !storeCntFlag) {
          modeFlag = true;
          LcdDisplay(1, 0, "Store Mode");
        }

        //IF 0 Button pressed 3 itmes in a row, store the IRDatas
        if (zeroBtnCnt > 3) {
          EEPROMControler(true);
          zeroBtnCnt = 0;
        } else if (!storeCntFlag && ButtonStatusList[0]) {
          zeroBtnCnt++;
          storeCntFlag = true;
        }

        //Transmit or Receive of pressed number
      } else {

        //Assign number to exsit IRData's number
        int tmp = i;
        if (tmp == 10) {
          tmp = 0;
        } else if (tmp == 11) {
          tmp = 8;
        }

        if (!modeFlag && ButtonStatusList[i]) {
          IRTransmiter(tmp);
        } else if (modeFlag && ButtonStatusList[i] && !receiverFlag) {
          if (IRReceiver(tmp)) {
            receiverFlag = true;
          }
        }
      }
      //Detact a any button is pressed or not.
      isAnyBtnPressed = isAnyBtnPressed | ButtonStatusList[i];
    }

    if (!isAnyBtnPressed && receiverFlag) {
      receiverFlag = false;
    }

    if (!isAnyBtnPressed && storeCntFlag) {
      storeCntFlag = false;
    }

    if (isAnyBtnPressed) {
      TimeKeeper(true);
    }

    //If time is out, skip a Notify and goto sleepMode
    if (TimeKeeper(isAnyBtnPressed)) {
      xTaskNotifyGive(xTaskButtonControl);
    } else {
      //Now goto sleep
      SetUpSleep();
    }

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  }
}

//Detect buttons status And set ButtonStatusList
void TaskButtonControl(void *pvParameters) {
  (void)pvParameters;

  int val, cnt, tmp = 0;
  int pressedButton;

  for (;;) {

    //Find a pressed button
    val = analogRead(1);
    pressedButton = 255;
    int i;
    for (i = 0; i < 12; i++) {
      if (ButtonsVoltage[i] <= val && val < ButtonsVoltage[i + 1]) {

        //Wait for same number counted 7 times in a row
        if (tmp == i) {
          cnt++;
        } else {
          cnt = 0;
        }

        if (cnt > 5) {
          pressedButton = i;
        }
        tmp = i;
      }

      //Reset the Button Status
      ButtonStatusList[i] = false;
    }

    if (pressedButton != 225) {
      ButtonStatusList[pressedButton] = true;
    }

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    xTaskNotifyGive(xTaskMain);
  }
}

//Print a text except it's Displayed already
void LcdDisplay(bool clearDisp, bool cursorSet, char text[17]) {

  if (strcmp(DisplayCharacter, text)) {
    //Display clear
    //1:Desplay clear
    if (clearDisp) {
      lcd.clear();
    }

    //Cursor set and clear a row
    //0:cursor to column 0
    //1:cursor to column 1
    int i;
    for (i = 0; i < 16; i++) {
      lcd.setCursor(i, cursorSet);
      lcd.write(' ');
    }
    lcd.setCursor(0, cursorSet);

    lcd.write(text);

    strcpy(DisplayCharacter, text);
  }
}

void IRTransmiter(int buttonNum) {
  IrReceiver.stop();
  if (StoredIRDataList[buttonNum].protocol == 0) {
    LcdDisplay(0, 1, "Nothing stored");
  } else {
    IrSender.write(&StoredIRDataList[buttonNum], DEFUALT_NUMBER_OF_REPEATS_TO_SEND);

    char tmp[] = "Sending ";
    char protocol[9];
    ReadOutTextFromPRGMEM(protocol, StoredIRDataList[buttonNum].protocol);
    strcat(tmp, protocol);
    LcdDisplay(0, 1, tmp);
  }
  IrReceiver.start();
}


bool IRReceiver(int buttonNum) {

  bool receiveFlag = false;

  LcdDisplay(0, 1, "Receiving...");

  if (IrReceiver.decode()) {
    IRData *ReceivedData = IrReceiver.read();

    //Store IRData or Ignore it
    //Set ignore's
    if (ReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
      //Ignore repeat
    } else if (ReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
      //Ignore autorepeat
    } else if (ReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
      //Ignore parity error
    } else if (ReceivedData->protocol == UNKNOWN) {
      LcdDisplay(1, 0, "Unknown");
      receiveFlag = true;
    } else {
      //Store the receved IRData

      StoredIRDataList[buttonNum] = *ReceivedData;

      if (StoredIRDataList[buttonNum].protocol != 0) {
        StoredIRDataList[buttonNum].flags = 0;
        char tmp[] = "Receive ";
        char protocol[9];
        ReadOutTextFromPRGMEM(protocol, StoredIRDataList[buttonNum].protocol);
        strcat(tmp, protocol);
        LcdDisplay(0, 1, tmp);
        receiveFlag = true;
      }
    }

    IrReceiver.resume();
  }
  return receiveFlag;
}

void EEPROMControler(bool storeOrRead) {
  int address = 0x00;
  //True:Store  False:Read
  if (storeOrRead) {
    int i;
    for (i = 0; i < 10; i++) {
      EEPROM.put(address, StoredIRDataList[i]);
      address += sizeof(IRData);
    }
    LcdDisplay(1, 0, "Stored to EEPROM");
  } else {
    int i;
    for (i = 0; i < 10; i++) {
      EEPROM.get(address, StoredIRDataList[i]);
      address += sizeof(IRData);
    }
  }
}

void ReadOutTextFromPRGMEM(char returnText[], int protocolNum) {
  strcpy_P(returnText, (char *)pgm_read_word(&(ProtocolList[protocolNum])));
}

void WakeUp() {
  digitalWrite(LCD_BUCKLIGHT, HIGH);
  lcd.begin(16, 2);
  TimeKeeper(true);
  sleep_reset();
  detachInterrupt(0);
  xTaskNotifyGive(xTaskButtonControl);
}

void SetUpSleep() {
  digitalWrite(LCD_BUCKLIGHT, LOW);
  lcd.noDisplay();

  attachInterrupt(0, WakeUp, LOW);
}

bool TimeKeeper(bool timeReset) {

  if (timeReset) {
    LastPressedTime = millis();
    return true;
  } else {
    unsigned long currentTime = millis();
    if ((currentTime - LastPressedTime) >= 5 * 1000) {
      return false;
    } else {
      return true;
    }
  }
}

void WDT_Prescaler_Change() {
  portENTER_CRITICAL();
  wdt_reset();

  //Start timed sequence
  WDTCSR |= (1 << WDCE) | (1 << WDE);

  //Set new prescaler to 0.5s
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0);
  portEXIT_CRITICAL();
}
