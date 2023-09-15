unsigned long current_millis;
unsigned long pre_millis[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int states[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Переменные для датчиков
float temp_in_basement;
float temp_in_attic;
float temp_in_kitchen;

bool time_of_day;
bool gas;
bool secure_mode;

// Псевдо-параллельные процессы
bool _millis(unsigned long interval, int key) {
  // Функция для псевдо параллельных процессов.
  // Отслеживает время включения и выключения.
  if((current_millis - pre_millis[key]) > interval) {
    // Занесем текующее время во время сработки
    pre_millis[key] = millis();
    return true;
  } else {
    return false;
  }
}

// Настройки кухни
void setKitchen() {
  // # D13 - реле освещения Кухня
  // # D12 - датчик движения Кухня
  pinMode(13, OUTPUT);
  pinMode(12, INPUT);
}

// Настройки улица
void setStreet() {
  // D11 - реле освещения улица
  // D10 - датчик движения улица
  // D6 - реле сигнализации
  pinMode(11, OUTPUT);
  pinMode(10, INPUT);
  pinMode(6, OUTPUT);
}

// Настройки чердак
void setAttic() {
  // D9 - реле вентилятора
  pinMode(9, OUTPUT);
}

// Логика работы чердака
void Attic(float temp_in_attic) {
  // Если высокая температура, то
  // запускаем вентилятор
  // 415 АЦП - 35 градусов цельсия
  if(temp_in_attic < 415) {
    digitalWrite(9, HIGH);
  } else {
    digitalWrite(9, LOW);
  }
}

// Логика работы подвала
void Basement(float temp_in_basement) {
  // Если высокая температура, то
  // запускаем вентилятор
  // 750 АЦП - 0 градусов цельсия
  if(temp_in_basement > 750) {
    digitalWrite(8, HIGH);
  } else {
    digitalWrite(8, LOW);
  }
}

// Расчёт температуры термистора
// pin - аналоговый вход, с которого необходимо считать данные
// a, b, c - коэфициенты термистора
// r - сопротивление делителя постоянное
// Соотношение Стейтхарта - Харта
float getTemp(int pin,
  float a,
  float b,
  float c,
  float r) {
  float v_out = analogRead(pin);
  float log_rt = log(r * (v_out / (1024 - v_out)));
  return (1 / (a + b * log_rt + c * log_rt * log_rt * log_rt)) - 273.15;
}

// Настройки подвал
void setBasement() {
  // D8 - реле отопления
  // D7 - реле вентилятора
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
}

// Функция проверяет время суток по датчику освещенности
bool isEvening(int lighting = 200) {
  // Считываем данные с датчика освещенности на улице
  if(analogRead(0) < lighting) {
    return true;
  } else {
    return false;
  }
}

// Функция проверяет наличие или отсутствие газа
// MQ-2
bool isGas(int gas = 200) {
  // Считываем данные с датчика освещенности на улице
  if(analogRead(0) < gas) {
    return true;
  } else {
    return false;
  }
}

// Функция, которая проверяет наличие движения
bool moveDet(int pin,
  int key,
  unsigned long interval = 10000) {
  // int pin - то, откуда считываем данные
  // int key - ключ, куда заносим состяние
  // unsigned long interval - через сколько отключать освещение
  // Состояние включения освещения или силовой части
  if(pin) {
    states[key] = HIGH;
    pre_millis[key] = millis();
    return true;
  } else {
    if(_millis(interval, key)) {
      states[key] = LOW;
    }
    return false;
  }
}

// Функция отправки данных на компьютер
void sendMessage(float temp_in_basement = 0,
  float temp_in_attic = 0,
  float temp_in_kitchen = 0,
  bool gas = false,
  bool time_of_day = false,
  bool secure_mode = false
  ) {
  // Формирует сообщение и отправляет его
  // через Serial
  String str = "";

  str.concat("temp_in_basement:");
  str.concat(String(temp_in_basement, 2));
  str.concat(";");

  str.concat("temp_in_attic:");
  str.concat(String(temp_in_attic, 2));
  str.concat(";");

  str.concat("temp_in_kitchen:");
  str.concat(String(temp_in_kitchen, 2));
  str.concat(";");

  if(gas) {
    str.concat("gas:1;");
  } else {
    str.concat("gas:0;");
  }

  if(time_of_day) {
    str.concat("time_of_day:1;");
  } else {
    str.concat("time_of_day:0;");
  }

  if(secure_mode) {
    str.concat("secure_mode:1;");
  } else {
    str.concat("secure_mode:0;");
  }
  Serial.println(str);
  str = "";
}

// Инициализация основных настроек
void setup() {
  /*Предустановки.*/

  secure_mode = false;
  setKitchen();
  setStreet();
  setAttic();
  setBasement();

  // Устанавливаем соединение
  Serial.begin(9600);
}

// Основной цикл контроллера
void loop() {
  // Проверяем текущее время
  current_millis = millis();

  // Снимаем показания с датчиков
  temp_in_basement = getTemp(2, 1.009249522e-03, 2.378405444e-04, 2.019202697e-07, 10000);
  temp_in_attic = getTemp(4, 1.009249522e-03, 2.378405444e-04, 2.019202697e-07, 10000);
  temp_in_kitchen = getTemp(5, 1.009249522e-03, 2.378405444e-04, 2.019202697e-07, 10000);
  gas = isGas(250);
  time_of_day = isEvening(300);

  // Запускаем основные процессы управления
  // климат контролем
  Attic(temp_in_attic);
  Basement(temp_in_basement);

  // Формируем массив с данными
  // и отправляем его
  if(_millis(5000, 12)) {
    sendMessage(temp_in_basement,
      temp_in_attic,
      temp_in_kitchen,
      gas,
      time_of_day,
      secure_mode);
  }

  // Освещение и сирена
  if(!secure_mode) {
    if(isEvening()) {
      // Наблюдаем за датчиками движения
      // Включаем освещение
      moveDet(digitalRead(12), 13, 10000); // # Кухня
      moveDet(digitalRead(10), 11, 10000); // # Улица
      digitalWrite(13, states[13]);
      digitalWrite(11, states[11]);
    }
  } else {
    // Если есть движение
    if(moveDet(digitalRead(12), 13) || moveDet(digitalRead(10), 11)) {
      // Включить сигнализацию
      digitalWrite(6, HIGH);
    }
  }
}
