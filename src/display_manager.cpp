#include "display_manager.h"

DisplayManager::DisplayManager(StateManager* state, RFIDManager* rfid, ScanMatrix* scan, MultiplexerManager* mux) {
    stateManager = state;
    rfidManager = rfid;
    scanMatrix = scan;
    muxManager = mux;
    
    lastUpdate = 0;
    updateInterval = DISPLAY_UPDATE_INTERVAL;  // 2000мс по умолчанию
    verboseMode = false;
}

void DisplayManager::initialize() {
    DEBUG_PRINTLN("DisplayManager: Инициализация менеджера отображения");
    
    lastUpdate = millis();
    
    // Печатаем информацию о запуске
    printStartupInfo();
}

void DisplayManager::update() {
    if (!isTimeForUpdate()) {
        return;
    }
    
    lastUpdate = millis();
    
    // Основной отчет производительности
    if (verboseMode) {
        printFullReport();
    } else {
        printSystemStatus();
    }
}

bool DisplayManager::isTimeForUpdate() const {
    return (millis() - lastUpdate) >= updateInterval;
}

void DisplayManager::printStartupInfo() const {
    DEBUG_PRINTLN("=== КОНФИГУРАЦИЯ ПИНОВ ===");
    DEBUG_PRINTF("- I2C PN532: SDA=%d, SCL=%d\n", PN532_SDA_PIN, PN532_SCL_PIN);
    DEBUG_PRINTF("- MUX1 (строки): S0=%d, S1=%d, S2=%d, S3=GND\n", MUX1_S0_PIN, MUX1_S1_PIN, MUX1_S2_PIN);
    DEBUG_PRINTF("- MUX2 (столбцы): S0=%d, S1=%d, S2=%d, S3=%d\n", MUX2_S0_PIN, MUX2_S1_PIN, MUX2_S2_PIN, MUX2_S3_PIN);
    DEBUG_PRINTF("- Общий EN: %d\n", MUX_COMMON_EN_PIN);
    DEBUG_PRINTLN("==========================");
}

void DisplayManager::printSystemStatus() const {
    // ФИЛЬТР: Уменьшаем спам - показываем только ключевые изменения
    static uint32_t lastCardsCount = 0;
    static uint32_t lastEvents = 0;
    static SystemState lastState = STATE_INIT;
    static unsigned long lastPrint = 0;
    
    // Проверяем есть ли значимые изменения
    int currentCards = scanMatrix->findCardsInMatrix();
    uint32_t currentEvents = scanMatrix->getCardsDetected() + scanMatrix->getCardsRemoved() + scanMatrix->getCardChanges();
    SystemState currentState = stateManager->getCurrentState();
    
    bool hasChanges = (currentCards != lastCardsCount) || 
                     (currentEvents != lastEvents) || 
                     (currentState != lastState) ||
                     (millis() - lastPrint > 30000); // Принудительно раз в 30 сек
    
    if (!hasChanges) {
        return; // Не выводим если нет изменений
    }
    
    // Сохраняем текущие значения
    lastCardsCount = currentCards;
    lastEvents = currentEvents;
    lastState = currentState;
    lastPrint = millis();
    
    DEBUG_PRINTLN("=== СТАТУС СИСТЕМЫ ===");
    
    char uptimeBuffer[32];
    formatUptime(millis(), uptimeBuffer, sizeof(uptimeBuffer));
    
    DEBUG_PRINTF("Время работы: %s\n", uptimeBuffer);
    DEBUG_PRINTF("Карт в матрице: %d\n", currentCards);
    
    DEBUG_PRINTLN("======================");
}

void DisplayManager::printPerformanceReport() const {
    printHeader("ПРОИЗВОДИТЕЛЬНОСТЬ");
    
    // Статистика сканирования
    DEBUG_PRINTF("Режим: СОБЫТИЙНОЕ СКАНИРОВАНИЕ\n");
    
    // События карт
    DEBUG_PRINTF("Карт обнаружено: %lu\n", scanMatrix->getCardsDetected());
    DEBUG_PRINTF("Карт удалено: %lu\n", scanMatrix->getCardsRemoved());
    DEBUG_PRINTF("Карт изменено: %lu\n", scanMatrix->getCardChanges());
    uint32_t totalEvents = scanMatrix->getCardsDetected() + scanMatrix->getCardsRemoved() + scanMatrix->getCardChanges();
    DEBUG_PRINTF("Всего событий: %lu\n", totalEvents);
    
    // Статистика RFID убрана
    
    // Статистика состояний
    DEBUG_PRINTF("Переходы состояний: %lu\n", stateManager->getStateTransitions());
    DEBUG_PRINTF("Ошибки состояний: %lu\n", stateManager->getErrorCount());
    
    printFooter();
}

void DisplayManager::printMatrixStatus() const {
    printHeader("СОСТОЯНИЕ МАТРИЦЫ");
    
    DEBUG_PRINTF("Размер: %dx%d (%d ячеек)\n", MATRIX_ROWS, MATRIX_COLS, MATRIX_TOTAL_CELLS);
    
    int cardsFound = scanMatrix->findCardsInMatrix();
    DEBUG_PRINTF("Карт найдено: %d\n", cardsFound);
    DEBUG_PRINTF("События: обнаружено=%lu, удалено=%lu, изменено=%lu\n", 
                 scanMatrix->getCardsDetected(), 
                 scanMatrix->getCardsRemoved(), 
                 scanMatrix->getCardChanges());
    
    // Текущая позиция мультиплексоров
    DEBUG_PRINTF("Текущая ячейка: [%d,%d] (индекс %d)\n", 
                 muxManager->getCurrentRow(), 
                 muxManager->getCurrentCol(),
                 muxManager->getCurrentCellIndex());
    
    printFooter();
}

void DisplayManager::printRFIDDiagnostics() const {
    rfidManager->printDiagnostics();
}

void DisplayManager::printFullReport() const {
    printSeparator();
    printSystemStatus();
    printPerformanceReport();
    printMatrixStatus();
    
    // Печатаем матрицу если есть карты
    if (scanMatrix->findCardsInMatrix() > 0) {
        scanMatrix->printMatrixState();
    }
    
    printSeparator();
}

void DisplayManager::printErrorReport() const {
    printHeader("ОТЧЕТ ОБ ОШИБКАХ");
    
    DEBUG_PRINTF("Ошибки RFID: %lu\n", rfidManager->getErrors());
    DEBUG_PRINTF("Таймауты RFID: %lu\n", rfidManager->getTimeouts());
    DEBUG_PRINTF("Ошибки состояний: %lu\n", stateManager->getErrorCount());
    DEBUG_PRINTF("RFID подключен: %s\n", rfidManager->getConnected() ? "ДА" : "НЕТ");
    DEBUG_PRINTF("Текущее состояние: %s\n", getSystemStateDescription());
    
    if (stateManager->isInErrorState()) {
        DEBUG_PRINTF("Время в состоянии ошибки: %lu мс\n", stateManager->getStateRunTime());
    }
    
    printFooter();
}

void DisplayManager::printCardEvents() const {
    scanMatrix->printCardEvents();
}

void DisplayManager::printSeparator() const {
    DEBUG_PRINTLN("================================================================");
}

void DisplayManager::printHeader(const char* title) const {
    DEBUG_PRINTF("=== %s ===\n", title);
}

void DisplayManager::printFooter() const {
    DEBUG_PRINTLN("================================================================");
}

void DisplayManager::formatUptime(unsigned long uptimeMs, char* buffer, size_t bufferSize) const {
    unsigned long seconds = uptimeMs / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds %= 60;
    minutes %= 60;
    
    if (hours > 0) {
        snprintf(buffer, bufferSize, "%luh %lum %lus", hours, minutes, seconds);
    } else if (minutes > 0) {
        snprintf(buffer, bufferSize, "%lum %lus", minutes, seconds);
    } else {
        snprintf(buffer, bufferSize, "%lus", seconds);
    }
}

void DisplayManager::formatPercentage(float percentage, char* buffer, size_t bufferSize) const {
    snprintf(buffer, bufferSize, "%.1f%%", percentage);
}

const char* DisplayManager::getSystemStateDescription() const {
    SystemState state = stateManager->getCurrentState();
    
    switch (state) {
        case STATE_INIT:
            return "Инициализация системы";
        case STATE_SCANNING:
            return "Активное сканирование матрицы";
        case STATE_SCAN_CELL:
            return "Сканирование ячейки";
        case STATE_PROCESS_CARD:
            return "Обработка карты";
        case STATE_SWITCH_CELL:
            return "Переключение ячейки";
        case STATE_UPDATE_DISPLAY:
            return "Обновление дисплея";
        case STATE_IDLE:
            return "Режим ожидания";
        case STATE_ERROR:
            return "СОСТОЯНИЕ ОШИБКИ";
        default:
            return "Неизвестное состояние";
    }
} 