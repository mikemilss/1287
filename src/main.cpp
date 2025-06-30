/*
 * RFID Matrix 8×12 Scanner - ОТЛАДОЧНАЯ ВЕРСИЯ  
 * ESP32 + PN532 (I2C) + 2x HP4067 мультиплексоры
 * 
 * РЕЖИМЫ РАБОТЫ:
 * - DEBUG: Текущий режим - диагностика БЕЗ антенн
 * - PRODUCTION: Для работы с полной матрицей 96 антенн
 * 
 * ДЛЯ ПЕРЕКЛЮЧЕНИЯ НА ПРОИЗВОДСТВЕННЫЙ РЕЖИМ:
 * 1. Замените debug функции на RFID сканирование  
 * 2. Добавьте обработку найденных карт
 * 3. Реализуйте пользовательский интерфейс
 * 4. Уберите Serial отладку для скорости
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "state_manager.h"
#include "rfid_manager.h"
#include "multiplexer.h"
#include "scan_matrix.h"
#include "display_manager.h"

// =============================================
// RFID MATRIX 8×12 - ОСНОВНОЙ ФАЙЛ
// =============================================

// Глобальные объекты
StateManager stateManager;
RFIDManager rfidManager;
MultiplexerManager muxManager;
ScanMatrix scanMatrix(&muxManager, &rfidManager);
DisplayManager displayManager(&stateManager, &rfidManager, &scanMatrix, &muxManager);

// Метрики производительности
PerformanceMetrics metrics = {0};
unsigned long lastMetricsUpdate = 0;
unsigned long cycleStartTime = 0;

// =============================================
// ОБЪЯВЛЕНИЯ ФУНКЦИЙ
// =============================================

bool initializeI2C();
bool initializePN532();
void resetMetrics();
void updateMetrics();
void printPerformanceMetrics();
void processSerialCommands();

// =============================================
// ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ
// =============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("RFID Matrix 8×12 - Запуск системы");
    DEBUG_PRINTLN("Версия: Событийное сканирование v3.0");
    DEBUG_PRINTLN("========================================");
    
    // Инициализация I2C
    if (!initializeI2C()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: Проблемы с I2C");
    }
    
    // Инициализация PN532
    DEBUG_PRINTLN("Инициализация PN532 RFID модуля...");
    if (!initializePN532()) {
        DEBUG_PRINTLN("ПРЕДУПРЕЖДЕНИЕ: PN532 не инициализирован");
        DEBUG_PRINTLN("Система будет пытаться переподключиться автоматически");
    }
    
    // Инициализация остальных компонентов
    DEBUG_PRINTLN("Инициализация мультиплексоров HP4067...");
    muxManager.initialize();
    
    DEBUG_PRINTLN("Инициализация матрицы сканирования...");
    scanMatrix.initialize();
    
    DEBUG_PRINTLN("Инициализация менеджера состояний...");
    stateManager.initialize();
    
    DEBUG_PRINTLN("Инициализация менеджера отображения...");
    displayManager.initialize();
    
    resetMetrics();
    
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("СИСТЕМА ЗАПУЩЕНА!");
    DEBUG_PRINTLN("Начинаю событийное сканирование матрицы 8×12...");
    DEBUG_PRINTLN("========================================");
    
    cycleStartTime = millis();
}

// =============================================
// ОСНОВНОЙ НЕБЛОКИРУЮЩИЙ ЦИКЛ
// =============================================

void loop() {
    // ПРОСТОЕ СОБЫТИЙНОЕ СКАНИРОВАНИЕ
    scanMatrix.update();
    
    // Периодическое обновление дисплея раз в 5 секунд
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay >= 5000) {
        displayManager.printSystemStatus();
        lastDisplay = millis();
    }
    
    // Проверка подключения RFID раз в 10 секунд
    static unsigned long lastRFIDCheck = 0;
    if (millis() - lastRFIDCheck >= 10000) {
        rfidManager.checkConnection();
        lastRFIDCheck = millis();
    }
    
    delay(1);
}

// =============================================
// ИНИЦИАЛИЗАЦИЯ I2C
// =============================================

bool initializeI2C() {
    DEBUG_PRINTLN("Инициализация I2C...");
    
    Wire.begin(PN532_SDA_PIN, PN532_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);  // 100kHz для стабильной работы
    
    DEBUG_PRINTF("I2C настроен: SDA=%d, SCL=%d, частота=%dkHz\n", 
                 PN532_SDA_PIN, PN532_SCL_PIN, I2C_FREQUENCY / 1000);
    
    // БЫСТРОЕ тестирование I2C - проверяем только PN532
    DEBUG_PRINTLN("Быстрая проверка I2C шины...");
    
    // Проверяем только адрес PN532 (0x24)
    Wire.beginTransmission(0x24);
    if (Wire.endTransmission() == 0) {
        DEBUG_PRINTLN("PN532 найден на I2C адресе 0x24 - I2C работает!");
        return true;
    }
    
    DEBUG_PRINTLN("PN532 не найден на адресе 0x24");
    DEBUG_PRINTLN("Проверяем другие распространенные адреса...");
    
    // Проверяем еще несколько популярных адресов
    int commonAddresses[] = {0x48, 0x68, 0x76, 0x77, 0x3C, 0x3D};
    int deviceCount = 0;
    
    for (int i = 0; i < 6; i++) {
        Wire.beginTransmission(commonAddresses[i]);
        if (Wire.endTransmission() == 0) {
            DEBUG_PRINTF("I2C устройство найдено на адресе 0x%02X\n", commonAddresses[i]);
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        DEBUG_PRINTLN("ВНИМАНИЕ: I2C устройства не найдены");
        DEBUG_PRINTLN("Проверьте подключения и подтягивающие резисторы 3.3kΩ");
        DEBUG_PRINTLN("Продолжаем работу...");
    } else {
        DEBUG_PRINTF("Найдено %d I2C устройств\n", deviceCount);
    }
    
    return true;  // Продолжаем даже если нет устройств
}

// =============================================
// ИНИЦИАЛИЗАЦИЯ PN532 RFID МОДУЛЯ
// =============================================

bool initializePN532() {
    DEBUG_PRINTLN("Проверка доступности PN532 на I2C адресе 0x24...");
    
    // Проверяем наличие PN532 на шине I2C
    Wire.beginTransmission(0x24);
    if (Wire.endTransmission() != 0) {
        DEBUG_PRINTLN("ОШИБКА: PN532 не найден на I2C адресе 0x24");
        return false;
    }
    
    DEBUG_PRINTLN("PN532 обнаружен на I2C шине");
    
    // Инициализация RFID менеджера  
    if (!rfidManager.initialize()) {
        DEBUG_PRINTLN("ОШИБКА: Не удалось инициализировать RFID менеджер");
        return false;
    }
    
    // Дополнительная проверка - пытаемся получить версию прошивки
    DEBUG_PRINTLN("Проверка связи с PN532...");
    delay(100);  // Небольшая пауза для стабильности
    
    // Проверяем что RFID менеджер действительно подключен
    if (!rfidManager.getConnected()) {
        DEBUG_PRINTLN("ОШИБКА: PN532 не отвечает на команды");
        return false;
    }
    
    DEBUG_PRINTLN("PN532 RFID модуль успешно инициализирован и готов к работе!");
    return true;
}

// =============================================
// ОБРАБОТКА КОМАНД SERIAL (для отладки)
// =============================================

void processSerialCommands() {
    // УБРАЛ ВСЕ КОМАНДЫ - система работает автономно
    if (Serial.available()) {
        Serial.readString(); // Просто очищаем буфер
    }
}

// =============================================
// МЕТРИКИ ПРОИЗВОДИТЕЛЬНОСТИ
// =============================================

void resetMetrics() {
    metrics.totalScans = 0;
    metrics.successfulReads = 0;
    metrics.errors = 0;
    metrics.lastCycleTime = millis();
    metrics.currentFPS = 0.0;
}

void updateMetrics() {
    // Простые метрики для событийного режима
    metrics.totalScans++;
    
    // Больше не рассчитываем FPS - используем событийные метрики
    // Статистика ведется в ScanMatrix
}

void printPerformanceMetrics() {
    // Эта функция теперь заменена DisplayManager для событийных метрик
    displayManager.printPerformanceReport();
}



/* ============================================================================
 * ПРОИЗВОДСТВЕННЫЙ КОД ДЛЯ ПОЛНОЙ МАТРИЦЫ 8×12 (96 АНТЕНН)
 * Раскомментируйте и адаптируйте после подключения антенн
 * ============================================================================

// Структура для хранения RFID данных
struct RFIDCard {
  uint8_t uid[7];
  uint8_t uidLength;
  uint32_t timestamp;
  bool present;
};

// Матрица состояний RFID карт  
RFIDCard rfidMatrix[MATRIX_ROWS][MATRIX_COLS];

// Производственный loop для сканирования матрицы
void productionLoop() {
  static int currentRow = 0;
  static int currentCol = 0;
  static unsigned long lastScan = 0;
  
  // Сканируем каждую ячейку с интервалом
  if (millis() - lastScan >= SCAN_DELAY_MS) {
    
    // Выбираем текущую ячейку
    selectCell(currentRow, currentCol);
    delayMicroseconds(MUX_SETTLE_TIME_US);
    
    // Пытаемся прочитать RFID карту
    uint8_t uid[7];
    uint8_t uidLength;
    
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, RFID_TIMEOUT_MS)) {
      // Карта найдена
      if (!rfidMatrix[currentRow][currentCol].present) {
        // Новая карта появилась
        memcpy(rfidMatrix[currentRow][currentCol].uid, uid, uidLength);
        rfidMatrix[currentRow][currentCol].uidLength = uidLength;
        rfidMatrix[currentRow][currentCol].timestamp = millis();
        rfidMatrix[currentRow][currentCol].present = true;
        
        // Обработка события "карта добавлена"
        onCardAdded(currentRow, currentCol, uid, uidLength);
      }
    } else {
      // Карта не найдена
      if (rfidMatrix[currentRow][currentCol].present) {
        // Карта была убрана
        rfidMatrix[currentRow][currentCol].present = false;
        
        // Обработка события "карта убрана"  
        onCardRemoved(currentRow, currentCol);
      }
    }
    
    // Переход к следующей ячейке
    currentCol++;
    if (currentCol >= MATRIX_COLS) {
      currentCol = 0;
      currentRow++;
      if (currentRow >= MATRIX_ROWS) {
        currentRow = 0;
        // Полный цикл завершен
        onScanCycleComplete();
      }
    }
    
    lastScan = millis();
  }
}

// Обработчики событий (реализовать под задачу)
void onCardAdded(int row, int col, uint8_t* uid, uint8_t uidLength) {
  // Логика при добавлении карты
  Serial.printf("Карта добавлена [%d,%d]: ", row, col);
  for (int i = 0; i < uidLength; i++) {
    Serial.printf("%02X ", uid[i]);
  }
  Serial.println();
}

void onCardRemoved(int row, int col) {
  // Логика при удалении карты
  Serial.printf("Карта убрана [%d,%d]\n", row, col);
}

void onScanCycleComplete() {
  // Полный скан матрицы завершен (каждые ~3.8 сек)
  // Можно обновить интерфейс, отправить данные и т.д.
}

// Печать текущего состояния матрицы
void printMatrixState() {
  Serial.println("\n=== СОСТОЯНИЕ МАТРИЦЫ 8×12 ===");
  Serial.println("   0 1 2 3 4 5 6 7 8 9 A B");
  
  for (int row = MATRIX_ROWS - 1; row >= 0; row--) {
    Serial.printf("%X  ", row);
    for (int col = 0; col < MATRIX_COLS; col++) {
      Serial.print(rfidMatrix[row][col].present ? "█ " : "· ");
    }
    Serial.printf(" %X\n", row);
  }
  Serial.println("   0 1 2 3 4 5 6 7 8 9 A B");
  Serial.println("===============================");
}

============================================================================ */ 