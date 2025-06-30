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
    printHeader("СТАТУС СИСТЕМЫ");
    
    char uptimeBuffer[32];
    formatUptime(millis(), uptimeBuffer, sizeof(uptimeBuffer));
    
    DEBUG_PRINTF("Время работы: %s\n", uptimeBuffer);
    DEBUG_PRINTF("Состояние: %s\n", stateManager->getStateName(stateManager->getCurrentState()));
    DEBUG_PRINTF("Режим: СОБЫТИЙНОЕ СКАНИРОВАНИЕ\n");
    DEBUG_PRINTF("Текущая ячейка: %d/%d\n", scanMatrix->getCurrentCellIndex(), MATRIX_TOTAL_CELLS - 1);
    DEBUG_PRINTF("Проходов матрицы: %lu\n", scanMatrix->getTotalCycles());
    
    // Статистика RFID
    DEBUG_PRINTF("RFID подключен: %s\n", rfidManager->getConnected() ? "ДА" : "НЕТ");
    if (rfidManager->getTotalReads() > 0) {
        DEBUG_PRINTF("Успешность чтения: %.1f%%\n", rfidManager->getSuccessRate());
    }
    
    // События карт (главная метрика)
    int cardsInMatrix = scanMatrix->findCardsInMatrix();
    DEBUG_PRINTF("Карт в матрице: %d\n", cardsInMatrix);
    DEBUG_PRINTF("События: +%lu -%lu изм:%lu\n", 
                 scanMatrix->getCardsDetected(),
                 scanMatrix->getCardsRemoved(), 
                 scanMatrix->getCardChanges());
    
    printFooter();
}

void DisplayManager::printPerformanceReport() const {
    printHeader("ПРОИЗВОДИТЕЛЬНОСТЬ");
    
    // Статистика сканирования (событийная)
    DEBUG_PRINTF("Проходов матрицы: %lu\n", scanMatrix->getTotalCycles());
    DEBUG_PRINTF("Время последнего прохода: %lu мс\n", scanMatrix->getLastCycleTime());
    DEBUG_PRINTF("Режим: СОБЫТИЙНОЕ СКАНИРОВАНИЕ\n");
    
    // События карт
    DEBUG_PRINTF("Карт обнаружено: %lu\n", scanMatrix->getCardsDetected());
    DEBUG_PRINTF("Карт удалено: %lu\n", scanMatrix->getCardsRemoved());
    DEBUG_PRINTF("Карт изменено: %lu\n", scanMatrix->getCardChanges());
    uint32_t totalEvents = scanMatrix->getCardsDetected() + scanMatrix->getCardsRemoved() + scanMatrix->getCardChanges();
    DEBUG_PRINTF("Всего событий: %lu\n", totalEvents);
    
    // Статистика RFID
    DEBUG_PRINTF("Всего попыток чтения: %lu\n", rfidManager->getTotalReads());
    DEBUG_PRINTF("Успешные чтения: %lu\n", rfidManager->getSuccessfulReads());
    DEBUG_PRINTF("Ошибки RFID: %lu\n", rfidManager->getErrors());
    DEBUG_PRINTF("Таймауты: %lu\n", rfidManager->getTimeouts());
    
    if (rfidManager->getTotalReads() > 0) {
        char successBuffer[16];
        formatPercentage(rfidManager->getSuccessRate(), successBuffer, sizeof(successBuffer));
        DEBUG_PRINTF("Успешность: %s\n", successBuffer);
    }
    
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
    DEBUG_PRINTLN("════════════════════════════════════════════════════════════════");
}

void DisplayManager::printHeader(const char* title) const {
    DEBUG_PRINTF("║ %-60s ║\n", title);
    DEBUG_PRINTLN("╠════════════════════════════════════════════════════════════════╣");
}

void DisplayManager::printFooter() const {
    DEBUG_PRINTLN("╚════════════════════════════════════════════════════════════════╝");
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
        case STATE_SCAN_CELL:
            return "Сканирование ячейки";
        case STATE_PROCESS_CARD:
            return "Обработка карты";
        case STATE_SWITCH_CELL:
            return "Переключение ячейки";
        case STATE_UPDATE_DISPLAY:
            return "Обновление дисплея";
        case STATE_ERROR:
            return "СОСТОЯНИЕ ОШИБКИ";
        default:
            return "Неизвестное состояние";
    }
} 