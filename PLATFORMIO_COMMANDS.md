# 🛠️ PLATFORMIO - ОСНОВНЫЕ КОМАНДЫ

## 📋 **ОСНОВНЫЕ КОМАНДЫ СБОРКИ**

```bash
# Компиляция проекта
pio run

# Компиляция + загрузка в ESP32
pio run --target upload

# Очистка сборки
pio run --target clean

# Проверка размера прошивки
pio run --target checkprogsize

# Мониторинг серийного порта
pio device monitor
```

## 🔧 **КОМАНДЫ УПРАВЛЕНИЯ**

```bash
# Инициализация VS Code интеграции
pio project init --ide vscode

# Список подключенных устройств
pio device list

# Информация о платформе
pio platform show espressif32

# Обновление библиотек
pio lib update
```

## 📡 **КОМАНДЫ ЗАГРУЗКИ**

```bash
# Автоматическое определение порта и загрузка
pio run -t upload

# Загрузка на конкретный порт
pio run -t upload --upload-port /dev/cu.usbserial-1410

# Загрузка с мониторингом
pio run -t upload && pio device monitor
```

## 🔍 **ОТЛАДКА И ДИАГНОСТИКА**

```bash
# Verbose режим компиляции
pio run -v

# Информация о проекте  
pio project data

# Проверка зависимостей
pio check --verbose

# Список установленных библиотек
pio lib list
```

## 🎯 **СПЕЦИФИЧНО ДЛЯ НАШЕГО ПРОЕКТА**

### **Быстрая компиляция и загрузка:**
```bash
pio run -t upload && pio device monitor --baud 115200
```

### **Полная пересборка:**
```bash
pio run -t clean && pio run
```

### **Проверка состояния:**
```bash
pio run --target checkprogsize
```

## ⚡ **ТЕКУЩИЙ СТАТУС ПРОЕКТА**

- ✅ **PlatformIO**: 6.1.18 установлен
- ✅ **ESP32 Platform**: espressif32@6.11.0
- ✅ **Компиляция**: успешна (43.29 сек)
- ✅ **RAM**: 7.4% использовано (24120 байт)
- ✅ **Flash**: 9.9% использовано (310541 байт)
- ✅ **USB устройство**: `/dev/cu.usbserial-1410` обнаружено

## 🚀 **ГОТОВО К ЗАГРУЗКЕ НА ESP32!** 