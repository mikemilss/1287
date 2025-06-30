#ifndef SCAN_MATRIX_H
#define SCAN_MATRIX_H

#include <Arduino.h>
#include "config.h"
#include "multiplexer.h"
#include "rfid_manager.h"

class ScanMatrix {
private:
    MultiplexerManager* muxManager;
    RFIDManager* rfidManager;
    
    // Кэш состояний карт для всех ячеек
    CardInfo cardCache[MATRIX_TOTAL_CELLS];
    
    // Текущее сканирование
    int currentCellIndex;
    bool scanInProgress;
    
    // Метрики проходов матрицы (не FPS!)
    unsigned long cycleStartTime;
    unsigned long lastFullCycle;
    uint32_t totalCycles;
    
    // События карт (основные метрики для событийной режима)
    uint32_t cardsDetected;
    uint32_t cardsRemoved;
    uint32_t cardChanges;
    
public:
    ScanMatrix(MultiplexerManager* mux, RFIDManager* rfid);
    
    // Инициализация
    void initialize();
    
    // Основной цикл сканирования (событийный, не циклический)
    void update();
    
    // Сканирование одной ячейки
    ScanResult scanCurrentCell();
    ScanResult scanCell(int cellIndex);
    
    // Управление проходом матрицы
    void startNewCycle();
    bool isCycleComplete() const;
    int getCurrentCellIndex() const { return currentCellIndex; }
    
    // Переход к следующей ячейке
    void moveToNextCell();
    
    // Работа с кэшем карт
    const CardInfo& getCardInfo(int cellIndex) const;
    bool isCardPresent(int cellIndex) const;
    bool hasCardChanged(int cellIndex) const;
    void clearCardCache();
    
    // Поиск карт
    int findCardsInMatrix() const;  // Возвращает количество найденных карт
    void printMatrixState() const;
    
    // Метрики событийной сканирования (не FPS!)
    uint32_t getTotalCycles() const { return totalCycles; }
    unsigned long getLastCycleTime() const;
    
    // События карт (основные метрики)
    uint32_t getCardsDetected() const { return cardsDetected; }
    uint32_t getCardsRemoved() const { return cardsRemoved; }
    uint32_t getCardChanges() const { return cardChanges; }
    
    // Сброс статистики
    void resetStatistics();
    
    // Отладка
    void printPerformanceMetrics() const;
    void printCardEvents() const;
    
private:
    // Внутренние методы
    void updateCardCache(int cellIndex, const ScanResult& result);
    void processCardEvent(int cellIndex, const CardInfo& oldInfo, const CardInfo& newInfo);
    void logCardEvent(int cellIndex, const char* event, const CardInfo& cardInfo) const;
    
    // Валидация
    bool isValidCellIndex(int cellIndex) const;
};

#endif // SCAN_MATRIX_H 