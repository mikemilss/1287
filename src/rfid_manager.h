#ifndef RFID_MANAGER_H
#define RFID_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include "config.h"

class RFIDManager {
private:
    Adafruit_PN532* nfc;
    bool isInitialized;
    bool isConnected;
    
    // Статистика
    uint32_t totalReads;
    uint32_t successfulReads;
    uint32_t errors;
    uint32_t timeouts;
    
    // Тайминги для неблокирующей работы
    unsigned long lastReadAttempt;
    unsigned long lastInitAttempt;
    
    // Последние данные чтения
    uint8_t lastUID[UID_BUFFER_SIZE];
    uint8_t lastUIDLength;
    bool lastReadValid;
    
public:
    RFIDManager();
    ~RFIDManager();
    
    // Инициализация и подключение
    bool initialize();
    bool reconnect();
    void checkConnection();
    
    // Основные операции чтения (неблокирующие)
    ScanResult scanCard();
    ScanResult scanCardFast();  // Оптимизированная версия
    
    // Получение данных последнего чтения
    bool getLastUID(uint8_t* uid, uint8_t& uidLength) const;
    bool isLastReadValid() const { return lastReadValid; }
    
    // Состояние системы
    bool getInitialized() const { return isInitialized; }
    bool getConnected() const { return isConnected; }
    
    // Статистика и метрики
    uint32_t getTotalReads() const { return totalReads; }
    uint32_t getSuccessfulReads() const { return successfulReads; }
    uint32_t getErrors() const { return errors; }
    uint32_t getTimeouts() const { return timeouts; }
    float getSuccessRate() const;
    
    // Сброс статистики
    void resetStatistics();
    
    // Тестирование и диагностика
    bool testConnection();
    bool getFirmwareVersion();
    void printDiagnostics() const;
    
    // Обработка ошибок
    void handleError(const char* errorMessage);
    void incrementError() { errors++; }
    void incrementTimeout() { timeouts++; }
    
private:
    // Внутренние методы
    bool initializeHardware();
    bool configurePN532();
    void resetLastRead();
    bool readPassiveTarget(uint8_t* uid, uint8_t& uidLength);
    
    // Тайминги и таймауты
    bool isTimeForRead() const;
    bool isTimeForReconnect() const;
};

#endif // RFID_MANAGER_H 