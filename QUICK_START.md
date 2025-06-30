# 🚀 БЫСТРЫЙ СТАРТ

## 1️⃣ ПЕРВЫЙ ТЕСТ: I2C Scanner

### Подключение:
```
PN532 → ESP32
VCC   → 3.3V
GND   → GND  
SDA   → GPIO 21
SCL   → GPIO 22
```

### Команды для I2C теста:
```bash
# 1. Скопировать I2C тест в main.cpp
cp test/i2c_scanner_test.cpp src/main_temp.cpp
mv src/main.cpp src/main_backup.cpp
mv src/main_temp.cpp src/main.cpp

# 2. Компилировать и загрузить
pio run --target upload
pio device monitor

# 3. После тестирования вернуть основной код
mv src/main.cpp src/main_temp.cpp
mv src/main_backup.cpp src/main.cpp
rm src/main_temp.cpp
```

### Ожидаемый результат:
```
✅ I2C устройство найдено по адресу: 0x24 <- PN532 NFC/RFID модуль!
```

## 2️⃣ ОСНОВНОЙ ТЕСТ

```bash
pio run --target upload
pio device monitor
```

Поднести RFID карту к PN532.

## 🛒 КОМПОНЕНТЫ ДЛЯ ПОКУПКИ

- [ ] PN532 NFC/RFID модуль  
- [ ] 2x HP4067 мультиплексор
- [ ] Breadboard и провода
- [ ] RFID карты для тестирования

## 🔧 РЕШЕНИЕ ПРОБЛЕМ

**PN532 не найден**: Проверить питание 3.3V и подключение I2C.

**Ошибки компиляции**: `pio run --target clean`

**ESP32 не подключается**: Проверить USB кабель и драйверы. 