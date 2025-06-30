#include "scan_matrix.h"

ScanMatrix::ScanMatrix(MultiplexerManager* mux, RFIDManager* rfid) {
    muxManager = mux;
    rfidManager = rfid;
    
    currentCellIndex = 0;
    scanInProgress = false;
    
    cycleStartTime = 0;
    lastFullCycle = 0;
    totalCycles = 0;
    // currentFPS убран - используем событийное сканирование вместо FPS
    
    cardsDetected = 0;
    cardsRemoved = 0;
    cardChanges = 0;
    
    // Инициализация кэша карт
    clearCardCache();
}

void ScanMatrix::initialize() {
    DEBUG_PRINTLN("ScanMatrix: Инициализация матрицы сканирования");
    
    if (muxManager == nullptr || rfidManager == nullptr) {
        DEBUG_PRINTLN("ОШИБКА ScanMatrix: Null указатели на менеджеры");
        return;
    }
    
    clearCardCache();
    currentCellIndex = 0;
    scanInProgress = false;
    
    DEBUG_PRINTF("ScanMatrix: Инициализирована матрица %dx%d (%d ячеек)\n", 
                 MATRIX_ROWS, MATRIX_COLS, MATRIX_TOTAL_CELLS);
    DEBUG_PRINTF("ScanMatrix: Ожидаемое время полного цикла: %.1f сек (консервативный режим)\n", 
                 MATRIX_TOTAL_CELLS * SCAN_DELAY_MS / 1000.0);
}

void ScanMatrix::update() {
    // Автоматический запуск нового цикла если предыдущий завершен
    if (!scanInProgress) {
        startNewCycle();
    }
    
    // СОБЫТИЙНОЕ СКАНИРОВАНИЕ - НЕ ЦИКЛИЧЕСКОЕ!
    // Сканируем текущую ячейку и ОСТАЕМСЯ на ней если есть карта
    
    ScanResult result = scanCurrentCell();
    
    // Обновляем кэш
    updateCardCache(currentCellIndex, result);
    
    // ЛОГИКА СОБЫТИЙНОГО СКАНИРОВАНИЯ:
    switch (result) {
        case SCAN_CARD_FOUND:
        case SCAN_CARD_CHANGED:
            // КАРТА НАЙДЕНА - остаемся на этой ячейке еще немного для стабильности
            static unsigned long cardFoundTime = 0;
            static int lastCardCell = -1;
            
            if (lastCardCell != currentCellIndex) {
                // Новая ячейка с картой
                cardFoundTime = millis();
                lastCardCell = currentCellIndex;
                
                // Логируем событие обнаружения
                if (LOG_CARD_EVENTS) {
                    int row = currentCellIndex / MATRIX_COLS;
                    int col = currentCellIndex % MATRIX_COLS;
                    DEBUG_PRINTF(">>> КАРТА ОБНАРУЖЕНА в ячейке [%d,%d] (индекс %d) <<<\n", 
                                 row, col, currentCellIndex);
                    
                    uint8_t uid[UID_BUFFER_SIZE];
                    uint8_t uidLength;
                    if (rfidManager->getLastUID(uid, uidLength)) {
                        DEBUG_PRINTF(">>> UID (%d байт): ", uidLength);
                        for (uint8_t i = 0; i < uidLength; i++) {
                            DEBUG_PRINTF("%02X ", uid[i]);
                        }
                        DEBUG_PRINTLN(" <<<");
                    }
                }
            }
            
            // Задерживаемся на карте 1 секунду для стабильного чтения
            if (millis() - cardFoundTime < 1000) {
                return; // НЕ переходим к следующей ячейке
            }
            
            // После стабильного чтения переходим дальше
            moveToNextCell();
            break;
            
        case SCAN_NO_CARD:
            // Нет карты - быстро переходим к следующей ячейке
            moveToNextCell();
            break;
            
        case SCAN_ERROR:
            // Ошибка - пропускаем ячейку и идем дальше
            DEBUG_PRINTF("ОШИБКА сканирования ячейки %d\n", currentCellIndex);
            moveToNextCell();
            break;
    }
    
    // Проверяем завершение полного прохода матрицы
    if (isCycleComplete()) {
        totalCycles++;
        scanInProgress = false;
        
        // Печатаем сводку только при завершении полного прохода
        if (ENABLE_PERFORMANCE_LOG) {
            int cardsFound = findCardsInMatrix();
            DEBUG_PRINTF(">>> ПОЛНЫЙ ПРОХОД #%lu ЗАВЕРШЕН: найдено %d карт <<<\n", 
                         totalCycles, cardsFound);
        }
        
        // Небольшая пауза между полными проходами
        delay(100);
    }
}

void ScanMatrix::startNewCycle() {
    cycleStartTime = millis();
    currentCellIndex = 0;
    scanInProgress = true;
    
    // Выбираем первую ячейку
    muxManager->selectCellByIndex(currentCellIndex);
}

ScanResult ScanMatrix::scanCurrentCell() {
    return scanCell(currentCellIndex);
}

ScanResult ScanMatrix::scanCell(int cellIndex) {
    if (!isValidCellIndex(cellIndex)) {
        DEBUG_PRINTF("ОШИБКА ScanMatrix: Неверный индекс ячейки %d\n", cellIndex);
        return SCAN_ERROR;
    }
    
    // Убеждаемся, что выбрана правильная ячейка
    muxManager->selectCellByIndex(cellIndex);
    
    // Сканируем карту через RFID менеджер
    ScanResult result = rfidManager->scanCardFast();
    
    return result;
}

void ScanMatrix::moveToNextCell() {
    currentCellIndex++;
    
    if (currentCellIndex < MATRIX_TOTAL_CELLS) {
        // Переключаемся на следующую ячейку
        muxManager->selectCellByIndex(currentCellIndex);
    }
}

bool ScanMatrix::isCycleComplete() const {
    return currentCellIndex >= MATRIX_TOTAL_CELLS;
}

void ScanMatrix::updateCardCache(int cellIndex, const ScanResult& result) {
    if (!isValidCellIndex(cellIndex)) {
        return;
    }
    
    CardInfo& cache = cardCache[cellIndex];
    CardInfo oldInfo = cache;  // Сохраняем старое состояние
    
    unsigned long currentTime = millis();
    
    switch (result) {
        case SCAN_CARD_FOUND:
        case SCAN_CARD_CHANGED:
            // Получаем UID от RFID менеджера
            if (rfidManager->getLastUID(cache.uid, cache.uidLength)) {
                cache.present = true;
                cache.lastSeen = currentTime;
                cache.changed = (result == SCAN_CARD_CHANGED);
                
                if (result == SCAN_CARD_CHANGED || !oldInfo.present) {
                    processCardEvent(cellIndex, oldInfo, cache);
                }
            }
            break;
            
        case SCAN_NO_CARD:
            if (cache.present) {
                // Карта была удалена
                cache.present = false;
                cache.changed = true;
                memset(cache.uid, 0, sizeof(cache.uid));
                cache.uidLength = 0;
                
                processCardEvent(cellIndex, oldInfo, cache);
            }
            break;
            
        case SCAN_ERROR:
            // Не изменяем кэш при ошибке
            break;
    }
}

void ScanMatrix::processCardEvent(int cellIndex, const CardInfo& oldInfo, const CardInfo& newInfo) {
    if (!oldInfo.present && newInfo.present) {
        // Карта добавлена
        cardsDetected++;
        logCardEvent(cellIndex, "ДОБАВЛЕНА", newInfo);
        
    } else if (oldInfo.present && !newInfo.present) {
        // Карта удалена
        cardsRemoved++;
        logCardEvent(cellIndex, "УДАЛЕНА", oldInfo);
        
    } else if (oldInfo.present && newInfo.present) {
        // Карта изменилась
        bool uidChanged = (oldInfo.uidLength != newInfo.uidLength);
        if (!uidChanged) {
            for (uint8_t i = 0; i < oldInfo.uidLength; i++) {
                if (oldInfo.uid[i] != newInfo.uid[i]) {
                    uidChanged = true;
                    break;
                }
            }
        }
        
        if (uidChanged) {
            cardChanges++;
            logCardEvent(cellIndex, "ИЗМЕНЕНА", newInfo);
        }
    }
}

void ScanMatrix::logCardEvent(int cellIndex, const char* event, const CardInfo& cardInfo) const {
    if (!LOG_CARD_EVENTS) return;
    
    int row = cellIndex / MATRIX_COLS;
    int col = cellIndex % MATRIX_COLS;
    
    DEBUG_PRINTF("ScanMatrix: Карта %s в ячейке [%d,%d] (индекс %d)\n", 
                 event, row, col, cellIndex);
    
    if (cardInfo.present && cardInfo.uidLength > 0) {
        DEBUG_PRINTF("ScanMatrix: UID (%d байт): ", cardInfo.uidLength);
        for (uint8_t i = 0; i < cardInfo.uidLength; i++) {
            DEBUG_PRINTF("%02X ", cardInfo.uid[i]);
        }
        DEBUG_PRINTLN("");
    }
}

// FPS метрики удалены - используем событийное сканирование

unsigned long ScanMatrix::getLastCycleTime() const {
    if (lastFullCycle > 0 && cycleStartTime > 0) {
        return lastFullCycle - cycleStartTime;
    }
    return 0;
}

const CardInfo& ScanMatrix::getCardInfo(int cellIndex) const {
    static CardInfo emptyCard = {false, {0}, 0, 0, false};
    
    if (!isValidCellIndex(cellIndex)) {
        return emptyCard;
    }
    
    return cardCache[cellIndex];
}

bool ScanMatrix::isCardPresent(int cellIndex) const {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    return cardCache[cellIndex].present;
}

bool ScanMatrix::hasCardChanged(int cellIndex) const {
    if (!isValidCellIndex(cellIndex)) {
        return false;
    }
    
    return cardCache[cellIndex].changed;
}

int ScanMatrix::findCardsInMatrix() const {
    int cardCount = 0;
    
    for (int i = 0; i < MATRIX_TOTAL_CELLS; i++) {
        if (cardCache[i].present) {
            cardCount++;
        }
    }
    
    return cardCount;
}

void ScanMatrix::clearCardCache() {
    for (int i = 0; i < MATRIX_TOTAL_CELLS; i++) {
        cardCache[i].present = false;
        cardCache[i].changed = false;
        cardCache[i].uidLength = 0;
        cardCache[i].lastSeen = 0;
        memset(cardCache[i].uid, 0, sizeof(cardCache[i].uid));
    }
    
    DEBUG_PRINTLN("ScanMatrix: Кэш карт очищен");
}

void ScanMatrix::resetStatistics() {
    totalCycles = 0;
    cardsDetected = 0;
    cardsRemoved = 0;
    cardChanges = 0;
    // Убираем FPS метрики
    lastFullCycle = 0;
    
    DEBUG_PRINTLN("ScanMatrix: Статистика сброшена");
}

bool ScanMatrix::isValidCellIndex(int cellIndex) const {
    return (cellIndex >= 0 && cellIndex < MATRIX_TOTAL_CELLS);
}

void ScanMatrix::printPerformanceMetrics() const {
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("СТАТИСТИКА СКАНИРОВАНИЯ (СОБЫТИЙНЫЙ РЕЖИМ)");
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTF("Полных проходов матрицы: %lu\n", totalCycles);
    DEBUG_PRINTF("Текущая ячейка: %d/%d\n", currentCellIndex, MATRIX_TOTAL_CELLS - 1);
    DEBUG_PRINTF("Сканирование активно: %s\n", scanInProgress ? "ДА" : "НЕТ");
    
    int cardsInMatrix = findCardsInMatrix();
    DEBUG_PRINTF("Карт в матрице сейчас: %d\n", cardsInMatrix);
    
    // События карт
    DEBUG_PRINTF("События: обнаружено=%lu, удалено=%lu, изменено=%lu\n", 
                 cardsDetected, cardsRemoved, cardChanges);
    
    DEBUG_PRINTLN("========================================");
}

void ScanMatrix::printCardEvents() const {
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("СОБЫТИЯ КАРТ");
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTF("Карт обнаружено: %lu\n", cardsDetected);
    DEBUG_PRINTF("Карт удалено: %lu\n", cardsRemoved);
    DEBUG_PRINTF("Карт изменено: %lu\n", cardChanges);
    DEBUG_PRINTF("Общее количество событий: %lu\n", cardsDetected + cardsRemoved + cardChanges);
    DEBUG_PRINTLN("========================================");
}

void ScanMatrix::printMatrixState() const {
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("СОСТОЯНИЕ МАТРИЦЫ");
    DEBUG_PRINTLN("========================================");
    
    DEBUG_PRINTLN("   0  1  2  3  4  5  6  7  8  9 10 11");
    
    for (int row = 0; row < MATRIX_ROWS; row++) {
        DEBUG_PRINTF("%d: ", row);
        
        for (int col = 0; col < MATRIX_COLS; col++) {
            int cellIndex = row * MATRIX_COLS + col;
            const CardInfo& card = cardCache[cellIndex];
            
            if (card.present) {
                DEBUG_PRINTF("[X]");
            } else {
                DEBUG_PRINTF("[ ]");
            }
        }
        DEBUG_PRINTLN("");
    }
    
    DEBUG_PRINTLN("========================================");
} 