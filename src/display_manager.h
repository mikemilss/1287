#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "state_manager.h"
#include "rfid_manager.h"
#include "scan_matrix.h"
#include "multiplexer.h"

class DisplayManager {
private:
    StateManager* stateManager;
    RFIDManager* rfidManager;
    ScanMatrix* scanMatrix;
    MultiplexerManager* muxManager;
    
    unsigned long lastUpdate;
    unsigned long updateInterval;
    bool verboseMode;
    
public:
    DisplayManager(StateManager* state, RFIDManager* rfid, ScanMatrix* scan, MultiplexerManager* mux);
    
    // Инициализация
    void initialize();
    
    // Основные методы отображения
    void update();
    bool isTimeForUpdate() const;
    
    // Различные виды отчетов
    void printSystemStatus() const;
    void printPerformanceReport() const;
    void printMatrixStatus() const;
    void printRFIDDiagnostics() const;
    void printFullReport() const;
    
    // Настройки отображения
    void setUpdateInterval(unsigned long interval) { updateInterval = interval; }
    void setVerboseMode(bool verbose) { verboseMode = verbose; }
    
    // Специальные отчеты
    void printStartupInfo() const;
    void printErrorReport() const;
    void printCardEvents() const;
    
    // Форматированный вывод
    void printSeparator() const;
    void printHeader(const char* title) const;
    void printFooter() const;
    
private:
    // Вспомогательные методы
    void formatUptime(unsigned long uptimeMs, char* buffer, size_t bufferSize) const;
    void formatPercentage(float percentage, char* buffer, size_t bufferSize) const;
    const char* getSystemStateDescription() const;
};

#endif // DISPLAY_MANAGER_H 