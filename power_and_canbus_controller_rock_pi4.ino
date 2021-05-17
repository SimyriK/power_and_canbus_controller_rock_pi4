/*
  Скетч для CarPC by Simyrik v1.0
    Функционал:
      Управление питанием платы RockPi 4:
        При обнаружении ACC
          ИЛИ При отсутствии питания на плате RockPi 4 - подать его
          ИЛИ При присутствии питания - нажать кнопку включения
        При отключении ACC
          1. Выждать время = PushPowerButtonTimer для фильтрации случайного выключения / работы стартера. 
          2. Нажать кнопку включения (на время = ButtonPressTime).
          3. Выждать время = PowerSupplyDelayTimer до отключения питания на плату RockPi 4. Необходимо самостоятельно зеаершить работу операционной системы, так как контроль на данном этапе не осуществляется.
      Перехват пакетов CAN-BUS из ЭБУ Haltech посредством MCP2515 и передача их в формате, понятном приложению RealDash (Realdash-can). Функция активируется только при включенном устройстве.
      Определение напряжения на аналоговых входах A0-A7 и из последующая передача в формате, понятном приложению RealDash (Realdash-can). Функция активируется только при включенном устройстве.
      (https://github.com/janimm/RealDash-extras)
    
*/
// МОДУЛЬ ПИТАНИЯ
#define PIN_RELAY 4 // Определяем пин, используемый для подключения реле, коммутируемое питание платы RockPi 4
#define PIN_ACC 3 // Определяем пин, используемый для подключения датчика контроля ACC
#define PIN_BUTTON 5 // Определяем пин, используемый для подключения кнопки включения платы RockPi 4 (pin 16)
#define PIN_VOLUMEUPBUTTON 6 // Определяем пин, используемый для подключения кнопки увеличения громкости платы RockPi 4 (pin 18)
volatile boolean AccFlag = false;   // Флаг наличия ACC
volatile boolean SleepFlag = false; // Флаг состояния перехода в сон
volatile boolean PowerOffFlag = false; // Флаг состояния выключения питания
volatile boolean PowerOnFlag = false; // Флаг состояния включения питания
volatile boolean SleepStatus = true; // Флаг состояния нахождения во сне
volatile boolean CanInitStarted = false; // Флаг начала инициализации соединения CAN-BUS
volatile boolean CanConnected = false; // Флаг состояния соединения CAN-BUS
volatile boolean DisableSerialSpam = false; // Флаг для отладки (отключает режим чтения и отсылки сообщения CAN-BUS и RealDash-CAN)
volatile boolean AnalogToCan = true;
#include "GyverTimer.h"   // библиотека таймера
GTimer PushPowerButtonTimer(MS, 10000); // Задержка до подачи сигнала для перехода в сон
GTimer PowerSupplyDelayTimer(MS, 7200000); // Задержка до отключения питания платы
GTimer CanRetryTimer(MS, 1000); // Задержка между попытками подключиться к CAN-BUS
//GTimer ButtonPressTimer(MS, 500);
//unsigned long timing;
//unsigned long PushPowerButtonDelayTiming = 0; // Обнуляем время задержки до нажатия кнопки питания
//const long PushPowerButtonDelay = 3000; // Устанавливаем требуемое время задержки нажатия на кнопку питания
//unsigned long ButtonPressTiming = 0; // Обнуляем время нажатия
const long ButtonPressTime = 300; // Устанавливаем требуемое время удержания кнопки (500 мс)
//unsigned long PowerSupplyDelayTiming = 0; // Обнуляем время до выключения подачи питания платы RockPi 4
//const long PowerSupplyDelay = 7000; // Устанавливаем требуемое время ожидания до выключения подачи питания платы RockPi 4 (2 часа = 7200000 мс)
// МОДУЛЬ ПИТАНИЯ

// ANALOG TO CAN
int analogPins[8] = {0};
// ANALOG TO CAN

// HALTECH TO CAN
#include <SPI.h>
#include <mcp_can.h>
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 10;
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin
// HALTECH TO CAN

void setup() {
  Serial.begin(2000000);
  
// МОДУЛЬ ПИТАНИЯ
  pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле подачи питания платы как выход
  pinMode(PIN_BUTTON, OUTPUT); // Объявляем пин кнопки питания как выход
  pinMode(PIN_VOLUMEUPBUTTON, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH); // Выключаем питание - посылаем высокий сигнал
  attachInterrupt(1, AccChange, CHANGE); // Прерывания для функции контроля наличия напряжения ACC
  PushPowerButtonTimer.stop();
  PowerSupplyDelayTimer.stop();
  CanRetryTimer.stop();
  if (digitalRead(PIN_ACC) == HIGH) { // Если питание ACC появляется
    delay(100);
    if (digitalRead(PIN_ACC) == HIGH) {
    AccFlag = true;
      }}
//  ButtonPressTimer.stop();
// МОДУЛЬ ПИТАНИЯ
}


// МОДУЛЬ ПИТАНИЯ
void AccChange() { // Если напряжение ACC изменило свой статус
    AccFlag = true; // Поднять Флаг факта изменения статуса ACC
}
// МОДУЛЬ ПИТАНИЯ

// ANALOG TO CAN
void ReadAnalogStatuses() {
  // read analog pins (1-7)
  for (int i=0; i<8; i++) {
    analogPins[i] = analogRead(i);
  }
}
void SendCANFramesToSerial() {
  byte buf[8];
  // build 2nd CAN frame, Arduino digital pins and 2 analog values
  memcpy(buf, &analogPins[0], 2);
  memcpy(buf + 2, &analogPins[1], 2);
  memcpy(buf + 4, &analogPins[2], 2);
  memcpy(buf + 6, &analogPins[3], 2);

  // write 2nd CAN frame to serial
  SendCANFrameToSerial(3201, buf);

  // build 3rd CAN frame, rest of Arduino analog values
  if (analogPins[7]<0.09) {
    analogPins[7]=0.0;//statement to quash undesired reading !
  }
  memcpy(buf, &analogPins[4], 2);
  memcpy(buf + 2, &analogPins[5], 2);
  memcpy(buf + 4, &analogPins[6], 2);
  memcpy(buf + 6, &analogPins[7], 2);

  // write 3rd CAN frame to serial
  SendCANFrameToSerial(3202, buf);
}
// ANALOG TO CAN

// HALTECH AND ANALOG TO CAN
void SendCANFrameToSerial(unsigned long canFrameId, const byte* frameData) {
  // the 4 byte identifier at the beginning of each CAN frame
  // this is required for RealDash to 'catch-up' on ongoing stream of CAN frames
  const byte serialBlockTag[4] = { 0x44, 0x33, 0x22, 0x11 };
  Serial.write(serialBlockTag, 4);
  // the CAN frame id number (as 32bit little endian value)
  Serial.write((const byte*)&canFrameId, 4);
  // CAN frame payload
  Serial.write(frameData, 8);
}
// HALTECH AND ANALOG TO CAN

void loop() {
// МОДУЛЬ ПИТАНИЯ
  if (AccFlag) { // Если Флаг изменения ACC поднят
    delay(300); // Выждать для предотвращения дребезга контактов
    if (AccFlag) { // Проверить еще раз
      AccFlag = false;    // Cбрасываем Флаг
      int AccControl = digitalRead(PIN_ACC); // Считваем значение с датчика питания платы в отдельную переменную
      if (AccControl == HIGH) { // Если питание ACC появляется
        Serial.println("ACC ON");
        PushPowerButtonTimer.stop(); // Сбрасываем таймер нажатия на кнопку
        PowerSupplyDelayTimer.stop(); // Сбрасываем таймер задержки нажатия на кнопку
//        ButtonPressTimer.stop();
        SleepFlag = false; // Сбрасываем флаг состояния перехода в сон
        PowerOffFlag = false; // Сбрасываем флаг состояния выключения питания
        PowerOnFlag = true; // Устанавливаем флаг состояния включения питания (для отлова дальнешего события)
      } else { // Если питание ACC пропадает
        Serial.println("ACC OFF");
        SleepFlag = true; // Устанавливаем флаг состояния перехода в сон (для отлова дальнешего события)
        PowerOffFlag = true; // Устанавливаем флаг состояния выключения питания (для отлова дальнешего события)
        PowerOnFlag = false; // Сбрасываем флаг состояния включения питания
      }
    }
  }
  if(PowerOnFlag) { // Если установлен флаг состояния включения питания
    PowerOnFlag = false; // Сюрасываем флаг для предотвращения повторения цикла
    int powerconrol = digitalRead(PIN_RELAY); // Считваем значение с датчика питания платы в отдельную переменную
    if (powerconrol == HIGH) { // Если питания на плате нет 
      digitalWrite(PIN_RELAY, LOW); // Включаем питание - посылаем низкий уровень сигнала
      Serial.println("Power Supply ON");
      SleepStatus = false; // Сбрасываем флаг состояния нахождения в сон
      if (CanConnected == true) {
        CAN.wake();
        Serial.println("CAN wake");
      } else {
        CanInitStarted = false;
      }
    }
    else {
      if (SleepStatus == true) { // Проверяем флаг статуса режима сна (чтобы будить только в случае нахождения во сне)
        SleepStatus = false; // Сбрасываем флаг состояния нахождения в сон
        Serial.println("Push power button to power on");
        digitalWrite(PIN_BUTTON, HIGH); // Нажимаем кнопку - посылаем высокий уровень сигнала
        delay(ButtonPressTime);
        digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
        SleepStatus = false; // Сбрасываем флаг статуса режима сна
        if (CanConnected == true) {
          CAN.wake();
          Serial.println("CAN wake");
        } else {
          CanInitStarted = false;
        }
//        ButtonPressTimer.reset();
//        ButtonPressTimer.start();
      }
    }
  }
  if (SleepFlag) { // Если установлен флаг состояния перехода в сон
    SleepFlag = false; // Сюрасываем флаг для предотвращения повторения цикла
    PushPowerButtonTimer.reset(); // Сбрасываем таймер задержки нажатия на кнопку питания для перехода в сон
    PushPowerButtonTimer.start(); // Запускаем таймер задержки нажатия на кнопку на кнопку питания для перехода в сон
  }
  if (PushPowerButtonTimer.isReady()) { // Когда таймер задержки нажатия на кнопку питания для перехода в сон настанет
    PushPowerButtonTimer.stop(); // Останавливаем таймер
    Serial.println("Push power button to power off");
    digitalWrite(PIN_BUTTON, HIGH); // Нажимаем кнопку - посылаем высокий уровень сигнала
    delay(ButtonPressTime); // Выжидаем время импульса нажатия на кнопку
    digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
    SleepStatus = true; // Устанавливаем флаг статуса нахождения в режиме сна
    if (CanConnected == true) {
      CAN.sleep();
      Serial.println("CAN sleep");
    }
//    ButtonPressTimer.reset();
//    ButtonPressTimer.start();
  }
  if (PowerOffFlag) { // Если установлен флаг состояния выключения
    PowerOffFlag = false; // Сюрасываем флаг для предотвращения повторения цикла
    PowerSupplyDelayTimer.reset(); // Сбрасываем таймер задержки выключения питания
    PowerSupplyDelayTimer.start(); // Запускаем таймер задержки выключения питания
  }
  if (PowerSupplyDelayTimer.isReady()) { // Когда таймер задержки выключения питания настанет
    digitalWrite(PIN_BUTTON, HIGH); // Нажимаем кнопку питания - посылаем высокий уровень сигнала
    delay(500);
    digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку питания - посылаем низкий уровень сигнала
    delay(3000);
    digitalWrite(PIN_VOLUMEUPBUTTON, HIGH); // Нажимаем кнопку увеличения громкости - посылаем высокий уровень сигнала
    delay(3000); // Долгое нажатие
    digitalWrite(PIN_VOLUMEUPBUTTON, LOW); // Отпускаем кнопку увеличения громкости - посылаем низкий уровень сигнала 
    delay(30000); // Ждемы завершения работы
    digitalWrite(PIN_RELAY, HIGH); // Отключаем питание - посылаем высокий уровень сигнала
    Serial.println("Power Supply OFF");
    PowerSupplyDelayTimer.stop();
  }
//  if (ButtonPressTimer.isReady()) {
//    digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
//    ButtonPressTimer.stop();
//  }
// МОДУЛЬ ПИТАНИЯ

// ANALOG TO CAN
//  if (SleepStatus == false && DisableSerialSpam == false && AnalogToCan == true) {
//    ReadAnalogStatuses();
//    SendCANFramesToSerial();
//  }
// ANALOG TO CAN

// HALTECH TO CAN
  if (SleepStatus == false && DisableSerialSpam == false && CanInitStarted == false) {
    Serial.println("CAN BUS INIT");
    CanRetryTimer.reset();
    CanRetryTimer.start();
    CanInitStarted = true;
//    if (CanInitStarted == false) {
//      CanInitStarted = true;
//      if (CAN_OK == CAN.begin(CAN_1000KBPS, MCP_8MHz)) { // init can bus : baudrate = 1000k
//        CanRetryTimer.stop();
//        CanConnected = true;
//        Serial.println("CAN BUS Shield init ok!");
//      } else {
//        Serial.println("CAN BUS Shield init fail");
//        Serial.println(" Init CAN BUS Shield again");
//        CanRetryTimer.reset();
//        CanRetryTimer.start();
//  //      delay(100);
//      }
    }
     
 if (CanRetryTimer.isReady()) {
      Serial.println("CAN BUS TIMER");
      CanRetryTimer.stop();
      if (CAN_OK == CAN.begin(CAN_1000KBPS, MCP_8MHz)) { // init can bus : baudrate = 1000k
          Serial.println("CAN BUS Shield init ok!");
          CanConnected = true;
      } else {
        Serial.println("CAN BUS Shield init fail");
        Serial.println(" Init CAN BUS Shield again");
        CanRetryTimer.reset();
        CanRetryTimer.start();
  //      delay(100);
      }
    }
  
 
// HALTECH TO CAN
// HALTECH TO CAN
  if (CanConnected && DisableSerialSpam == false) {
    unsigned char len = 0;
    unsigned char buf[8];
    if(CAN_MSGAVAIL == CAN.checkReceive()) { // check if data coming
      CAN.readMsgBuf(&len, buf); // read data, len: data length, buf: data buf
      unsigned int canId = CAN.getCanId();
      SendCANFrameToSerial(canId, buf);
    }
  }
// HALTECH TO CAN
}
