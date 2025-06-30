#ifndef CONFIG_H
#define CONFIG_H

// =============================================
// RFID MATRIX 8×12 - КОНФИГУРАЦИЯ
// =============================================

// I2C настройки для PN532
#define PN532_SDA_PIN           21
#define PN532_SCL_PIN           22
// PN532_I2C_ADDRESS уже определен в библиотеке Adafruit как 0x24, используем тот
// #define PN532_I2C_ADDRESS       0x24
#define PN532_IRQ_DUMMY         255  // -1 в uint8_t = библиотека НЕ будет использовать
#define PN532_RESET_DUMMY       255  // -1 в uint8_t = библиотека НЕ будет использовать

// HP4067 Мультиплексор #1 (строки 0-7, S3=GND)
#define MUX1_S0_PIN             4
#define MUX1_S1_PIN             5
#define MUX1_S2_PIN             15
// S3 подключен к GND для режима 8 каналов

// HP4067 Мультиплексор #2 (столбцы 0-11)  
#define MUX2_S0_PIN             18
#define MUX2_S1_PIN             19
#define MUX2_S2_PIN             23
#define MUX2_S3_PIN             25

// Общий EN пин для обоих мультиплексоров (экономия GPIO)
#define MUX_COMMON_EN_PIN       26

// Размеры матрицы
#define MATRIX_ROWS             8
#define MATRIX_COLS             12
#define MATRIX_TOTAL_CELLS      (MATRIX_ROWS * MATRIX_COLS)  // 96 ячеек

// ОПТИМИЗИРОВАННЫЕ ТАЙМИНГИ (в 8 раз быстрее чем было)
#define SCAN_DELAY_MS           5     // Задержка между сканированиями ячеек (было 40мс)
#define PN532_TIMEOUT_MS        10    // Таймаут PN532 операций (было 50мс)
#define MUX_SETTLE_TIME_US      2     // Время стабилизации мультиплексора (было 10мкс)
#define DISPLAY_UPDATE_INTERVAL 2000  // Обновление дисплея каждые 2 сек

// I2C настройки
#define I2C_FREQUENCY           100000  // 100kHz для стабильной работы
#define I2C_TIMEOUT_MS          100

// Размеры буферов
#define UID_BUFFER_SIZE         7      // Максимальный размер UID
#define CARD_CACHE_SIZE         96     // Кэш для всех ячеек матрицы

// Отладка и мониторинг
#define ENABLE_SERIAL_DEBUG     true
#define ENABLE_PERFORMANCE_LOG  true
#define LOG_CARD_EVENTS         true
#define LOG_ERROR_EVENTS        true

// Состояния системы
enum SystemState {
    STATE_INIT,           // Инициализация системы
    STATE_SCANNING,       // Режим активного сканирования матрицы
    STATE_SCAN_CELL,      // Сканирование текущей ячейки  
    STATE_PROCESS_CARD,   // Обработка найденной карты
    STATE_SWITCH_CELL,    // Переключение на следующую ячейку
    STATE_UPDATE_DISPLAY, // Обновление дисплея
    STATE_IDLE,           // Режим ожидания
    STATE_ERROR           // Обработка ошибок
};

// Результаты сканирования
enum ScanResult {
    SCAN_NO_CARD,         // Карта не найдена
    SCAN_CARD_FOUND,      // Карта найдена
    SCAN_CARD_CHANGED,    // Карта изменилась
    SCAN_ERROR            // Ошибка сканирования
};

// Макросы для отладки
#if ENABLE_SERIAL_DEBUG
    #define DEBUG_PRINT(x)   Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)  
    #define DEBUG_PRINTF(fmt, ...)
#endif

// Структура для хранения информации о карте
struct CardInfo {
    bool present;                    // Карта присутствует
    uint8_t uid[UID_BUFFER_SIZE];   // UID карты
    uint8_t uidLength;              // Длина UID
    unsigned long lastSeen;         // Время последнего обнаружения
    bool changed;                   // Флаг изменения
};

// Структура для метрик производительности
struct PerformanceMetrics {
    unsigned long totalScans;       // Общее количество сканирований
    unsigned long successfulReads;  // Успешные чтения
    unsigned long errors;           // Количество ошибок
    unsigned long lastCycleTime;    // Время последнего полного цикла
    float currentFPS;               // Текущий FPS
};

#endif // CONFIG_H 