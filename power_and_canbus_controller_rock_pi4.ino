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
*/
#define PIN_RELAY 4 // Определяем пин, используемый для подключения реле, коммутируемое питание платы RockPi 4
#define PIN_ACC 3 // Определяем пин, используемый для подключения датчика контроля ACC
#define PIN_BUTTON 5 // Определяем пин, используемый для подключения кнопки включения платы RockPi 4 (pin 16)
#define PIN_VOLUMEUPBUTTON 6 // Определяем пин, используемый для подключения кнопки увеличения громкости платы RockPi 4 (pin 16)
volatile boolean AccFlag = false;   // флаг
volatile boolean SleepFlag = false;
volatile boolean PowerOffFlag = false;
volatile boolean PowerOnFlag = false;
volatile boolean SleepStatus = false;
//volatile uint32_t debounce;
#include "GyverTimer.h"   // библиотека таймера
GTimer PushPowerButtonTimer(MS, 10000);       // создать таймер (по умолч. в режиме интервала)
GTimer PowerSupplyDelayTimer(MS, 30000);
//GTimer ButtonPressTimer(MS, 500);
//unsigned long timing;
//unsigned long PushPowerButtonDelayTiming = 0; // Обнуляем время задержки до нажатия кнопки питания
//const long PushPowerButtonDelay = 3000; // Устанавливаем требуемое время задержки нажатия на кнопку питания (10 с = 10000 мс)
//unsigned long ButtonPressTiming = 0; // Обнуляем время нажатия
const long ButtonPressTime = 200; // Устанавливаем требуемое время удержания кнопки (500 мс)
//unsigned long PowerSupplyDelayTiming = 0; // Обнуляем время до выключения подачи питания платы RockPi 4
//const long PowerSupplyDelay = 7000; // Устанавливаем требуемое время ожидания до выключения подачи питания платы RockPi 4 (2 часа = 7200000 мс)


void setup() {
// МОДУЛЬ ПИТАНИЯ
  Serial.begin(115200);
  pinMode(PIN_RELAY, OUTPUT); // Объявляем пин реле подачи питания платы как выход
  pinMode(PIN_BUTTON, OUTPUT); // Объявляем пин реле подачи питания платы как выход
  pinMode(PIN_VOLUMEUPBUTTON, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH); // Выключаем питание - посылаем высокий сигнал
  attachInterrupt(1, AccChange, CHANGE); // Прерывания для функции контроля наличия напряжения ACC
  PushPowerButtonTimer.stop();
  PowerSupplyDelayTimer.stop();
//  ButtonPressTimer.stop();
// МОДУЛЬ ПИТАНИЯ
}


// МОДУЛЬ ПИТАНИЯ
void AccChange() { // Если напряжение ACC изменило свой статус
    AccFlag = true; // Поднять флаг факта изменения статуса ACC
}
// МОДУЛЬ ПИТАНИЯ


void loop() {
  Serial.print(digitalRead(PIN_BUTTON));
// МОДУЛЬ ПИТАНИЯ
  if (AccFlag) { // Если флаг изменения ACC поднят
    delay(10); // Выждать для предотвращения дребезга контактов
    if (AccFlag) { // Проверить еще раз
      AccFlag = false;    // сбрасываем флаг
      int AccControl = digitalRead(PIN_ACC); // Считваем значение с датчика питания платы в отдельную переменную
      if (AccControl == HIGH) {
        Serial.println("ACC ON");
        PushPowerButtonTimer.stop();
        PowerSupplyDelayTimer.stop();
//        ButtonPressTimer.stop();
        SleepFlag = false;
        PowerOffFlag = false;
        PowerOnFlag = true;
      } else {
        Serial.println("ACC OFF");
        SleepFlag = true;
        PowerOffFlag = true;
        PowerOnFlag = false;
      }
    }
  }
  if(PowerOnFlag) {
    PowerOnFlag = false;
    int powerconrol = digitalRead(PIN_RELAY); // Считваем значение с датчика питания платы в отдельную переменную
    if (powerconrol == HIGH) {
      digitalWrite(PIN_RELAY, LOW); // Включаем питание - посылаем высокий уровень сигнала
      Serial.println("Power Supply ON");
    }
    else {
      if (SleepStatus) {
        Serial.println("Push power button to power on");
        digitalWrite(PIN_BUTTON, HIGH); // Нажимаем кнопку - посылаем высокий уровень сигнала
        delay(500);
        digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
        SleepStatus = false;
//        ButtonPressTimer.reset();
//        ButtonPressTimer.start();
      }
    }
  }
  if (SleepFlag) {
    SleepFlag = false;
    PushPowerButtonTimer.reset();
    PushPowerButtonTimer.start();
  }
  if (PushPowerButtonTimer.isReady()) {
    PushPowerButtonTimer.stop();
    Serial.println("Push power button to power off");
    digitalWrite(PIN_BUTTON, HIGH); // Нажимаем кнопку - посылаем высокий уровень сигнала
    delay(500);
    digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
    SleepStatus = true;
//    ButtonPressTimer.reset();
//    ButtonPressTimer.start();
  }
  if (PowerOffFlag) {
    PowerOffFlag = false;
    PowerSupplyDelayTimer.reset();
    PowerSupplyDelayTimer.start();
  }
  if (PowerSupplyDelayTimer.isReady()) {
    digitalWrite(PIN_RELAY, HIGH); // Отключаем питание - посылаем высокий уровень сигнала
    Serial.println("Power Supply OFF");
    PowerSupplyDelayTimer.stop();
  }
//  if (ButtonPressTimer.isReady()) {
//    digitalWrite(PIN_BUTTON, LOW); // Отпускаем кнопку - посылаем низкий уровень сигнала
//    ButtonPressTimer.stop();
//  }
// МОДУЛЬ ПИТАНИЯ
}
