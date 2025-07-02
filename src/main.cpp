/*
 * RFID Matrix 8×12 Scanner - ПРОИЗВОДСТВЕННАЯ ВЕРСИЯ v3.1 (КОНСЕРВАТИВНЫЙ РЕЖИМ)
 * ESP32 + PN532 (I2C) + 2x HP4067 мультиплексоры
 * 
 * ИСПРАВЛЕНИЯ v3.1:
 * - ✅ Правильная последовательность инициализации PN532
 * - ✅ StateManager интегрирован в основной цикл
 * - ✅ Обязательная проверка готовности перед сканированием
 * - ✅ Улучшенная обработка ошибок и переподключений
 * - ✅ Консервативные тайминги для максимальной стабильности
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
// RFID MATRIX 8×12 - ОСНОВНОЙ ФАЙЛ v3.1
// =============================================

// Глобальные объекты
StateManager stateManager;
RFIDManager rfidManager;
MultiplexerManager muxManager;
ScanMatrix scanMatrix(&muxManager, &rfidManager);
DisplayManager displayManager(&stateManager, &rfidManager, &scanMatrix, &muxManager);

// Счетчики попыток инициализации
int pn532InitAttempts = 0;
const int MAX_INIT_ATTEMPTS = 5;
unsigned long lastInitAttempt = 0;
const unsigned long INIT_RETRY_DELAY = 3000; // 3 секунды между попытками

// =============================================
// ОБЪЯВЛЕНИЯ ФУНКЦИЙ
// =============================================

bool initializeI2C();
bool initializePN532WithRetry();
bool initializePN532Single();
void initializeOtherComponents();
void handleInitializationError();
void handleErrorRecovery();
void handlePeriodicTasks();

// =============================================
// ИНИЦИАЛИЗАЦИЯ СИСТЕМЫ
// =============================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("RFID Matrix 8×12 - Запуск системы v3.1");
    DEBUG_PRINTLN("КОНСЕРВАТИВНЫЙ РЕЖИМ (стабильные тайминги)");
    DEBUG_PRINTLN("========================================");
    
    // Инициализация системы
    stateManager.initialize();
    stateManager.setState(STATE_INIT);
    
    if (!initializeI2C()) {
        stateManager.setState(STATE_ERROR);
        handleInitializationError();
        return;
    }
    
    if (!initializePN532WithRetry()) {
        stateManager.setState(STATE_ERROR);
        handleInitializationError();
        return;
    }
    
    initializeOtherComponents();
    stateManager.setState(STATE_SCANNING);
    
    DEBUG_PRINTLN("✅ СИСТЕМА ПОЛНОСТЬЮ ИНИЦИАЛИЗИРОВАНА!");
}

// =============================================
// ОСНОВНОЙ ЦИКЛ С STATE MACHINE
// =============================================

void loop() {
    // КРИТИЧНО: Обновляем StateManager в каждом цикле
    stateManager.updateState();
    
    // Действия в зависимости от текущего состояния
    switch (stateManager.getCurrentState()) {
        
        case STATE_INIT:
            // Система еще инициализируется, ждем
            delay(100);
            break;
            
        case STATE_SCANNING:
            // ОСНОВНОЕ СКАНИРОВАНИЕ - только если PN532 готов
            if (rfidManager.getConnected()) {
                scanMatrix.update();
            } else {
                // PN532 отключился, пытаемся переподключить
                stateManager.setState(STATE_ERROR);
            }
            break;
            
        case STATE_ERROR:
            // Режим ошибки - пытаемся восстановить подключение
            handleErrorRecovery();
            break;
            
        case STATE_IDLE:
            // Режим ожидания
            delay(100);
            break;
            
        default:
            stateManager.setState(STATE_ERROR);
            break;
    }
    
    // Периодические задачи (независимо от состояния)
    handlePeriodicTasks();
    
    delay(10); // Задержка для стабильности
}

// =============================================
// УЛУЧШЕННАЯ ИНИЦИАЛИЗАЦИЯ PN532
// =============================================

bool initializePN532WithRetry() {
    for (pn532InitAttempts = 1; pn532InitAttempts <= MAX_INIT_ATTEMPTS; pn532InitAttempts++) {
        if (initializePN532Single()) {
            DEBUG_PRINTF("✅ PN532 инициализирован с попытки #%d\n", pn532InitAttempts);
            return true;
        }
        
        if (pn532InitAttempts < MAX_INIT_ATTEMPTS) {
            delay(INIT_RETRY_DELAY);
        }
    }
    
    DEBUG_PRINTLN("🚨 ОШИБКА: PN532 не инициализирован!");
    return false;
}

bool initializePN532Single() {
    // Проверяем наличие PN532 на I2C шине
    Wire.beginTransmission(0x24);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // Инициализация RFID менеджера
    if (!rfidManager.initialize()) {
        return false;
    }
    
    // КРИТИЧЕСКАЯ ПРОВЕРКА: PN532 действительно отвечает
    delay(200); // Увеличенная пауза для стабильности
    
    if (!rfidManager.getConnected()) {
        return false;
    }
    
    return true;
}

// =============================================
// ИНИЦИАЛИЗАЦИЯ ОСТАЛЬНЫХ КОМПОНЕНТОВ
// =============================================

void initializeOtherComponents() {
    muxManager.initialize();
    scanMatrix.initialize();
    displayManager.initialize();
}

// =============================================
// ИНИЦИАЛИЗАЦИЯ I2C (БЕЗ ИЗМЕНЕНИЙ)
// =============================================

bool initializeI2C() {
    Wire.begin(PN532_SDA_PIN, PN532_SCL_PIN);
    Wire.setClock(I2C_FREQUENCY);  // 100kHz для стабильной работы
    
    // Быстрая проверка I2C шины
    Wire.beginTransmission(0x24);
    if (Wire.endTransmission() == 0) {
        DEBUG_PRINTLN("✅ PN532 найден на I2C адресе 0x24 - I2C работает!");
        return true;
    }
    
    return true;  // Продолжаем, PN532 попробуем инициализировать позже
}

// =============================================
// ОБРАБОТКА ОШИБОК И ВОССТАНОВЛЕНИЕ
// =============================================

void handleInitializationError() {
    DEBUG_PRINTLN("🚨 КРИТИЧЕСКАЯ ОШИБКА ИНИЦИАЛИЗАЦИИ!");
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("🔧 ПРОВЕРЬТЕ:");
    DEBUG_PRINTLN("1. Подключения I2C: SDA=GPIO21, SCL=GPIO22");
    DEBUG_PRINTLN("2. Подтягивающие резисторы 3.3kΩ на SDA/SCL к 3.3V");
    DEBUG_PRINTLN("3. Питание PN532: 3.3V или 5V");
    DEBUG_PRINTLN("4. Переключатели PN532: SW1=ON, SW2=OFF (I2C режим)");
    DEBUG_PRINTLN("5. Качество пайки и контактов");
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("⚠️ Система переходит в режим ошибки...");
}

void handleErrorRecovery() {
    static unsigned long lastRecoveryAttempt = 0;
    
    // Попытка восстановления каждые 5 секунд
    if (millis() - lastRecoveryAttempt >= 5000) {
        if (rfidManager.reconnect()) {
            DEBUG_PRINTLN("✅ Подключение восстановлено!");
            stateManager.setState(STATE_SCANNING);
        }
        
        lastRecoveryAttempt = millis();
    }
}

// =============================================
// ПЕРИОДИЧЕСКИЕ ЗАДАЧИ
// =============================================

void handlePeriodicTasks() {
    // Периодическое обновление дисплея (раз в 5 секунд)
    static unsigned long lastDisplay = 0;
    if (millis() - lastDisplay >= 5000) {
        if (stateManager.getCurrentState() != STATE_ERROR) {
            displayManager.printSystemStatus();
        }
        lastDisplay = millis();
    }
    
    // Проверка подключения RFID (раз в 10 секунд) 
    static unsigned long lastRFIDCheck = 0;
    if (millis() - lastRFIDCheck >= 10000) {
        rfidManager.checkConnection();
        
        // Если PN532 отключился во время работы
        if (!rfidManager.getConnected() && stateManager.getCurrentState() == STATE_SCANNING) {
            stateManager.setState(STATE_ERROR);
        }
        
        lastRFIDCheck = millis();
    }
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