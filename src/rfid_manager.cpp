#include "rfid_manager.h"

RFIDManager::RFIDManager() {
    nfc = nullptr;
    isInitialized = false;
    isConnected = false;
    
    totalReads = 0;
    successfulReads = 0;
    errors = 0;
    timeouts = 0;
    
    lastReadAttempt = 0;
    lastInitAttempt = 0;
    
    resetLastRead();
}

RFIDManager::~RFIDManager() {
    if (nfc != nullptr) {
        delete nfc;
        nfc = nullptr;
    }
}

bool RFIDManager::initialize() {
    DEBUG_PRINTLN("RFIDManager: Инициализация PN532...");
    
    // Создаем объект PN532 для I2C
    if (nfc != nullptr) {
        delete nfc;
    }
    
    nfc = new Adafruit_PN532(PN532_IRQ_DUMMY, PN532_RESET_DUMMY);
    
    if (!initializeHardware()) {
        handleError("Не удалось инициализировать PN532 аппаратуру");
        return false;
    }
    
    if (!configurePN532()) {
        handleError("Не удалось сконфигурировать PN532");
        return false;
    }
    
    isInitialized = true;
    isConnected = true;
    lastInitAttempt = millis();
    
    DEBUG_PRINTLN("RFIDManager: PN532 успешно инициализирован");
    return true;
}

bool RFIDManager::initializeHardware() {
    // Инициализация с быстрым таймаутом для неблокирующей работы
    nfc->begin();
    
    // Проверяем версию прошивки для подтверждения связи
    return getFirmwareVersion();
}

bool RFIDManager::getFirmwareVersion() {
    // КРИТИЧЕСКАЯ ПРОВЕРКА: nfc должен существовать
    if (nfc == nullptr) {
        DEBUG_PRINTLN("RFIDManager: ОШИБКА - nfc объект не создан");
        return false;
    }
    
    uint32_t versiondata = nfc->getFirmwareVersion();
    
    if (!versiondata) {
        DEBUG_PRINTLN("RFIDManager: ОШИБКА - PN532 не найден");
        DEBUG_PRINTLN("Проверьте:");
        DEBUG_PRINTLN("- Подключения I2C (SDA=21, SCL=22)");
        DEBUG_PRINTLN("- Подтягивающие резисторы 3.3kΩ на SDA/SCL");
        DEBUG_PRINTLN("- Питание PN532 (3.3V или 5V)");
        DEBUG_PRINTLN("- Переключатели на PN532 (I2C режим: SW1=ON, SW2=OFF)");
        return false;
    }
    
    DEBUG_PRINTF("RFIDManager: PN532 найден! Версия прошивки: 0x%08lX\n", versiondata);
    DEBUG_PRINTF("- Чип: PN5%02X\n", (versiondata >> 24) & 0xFF);
    DEBUG_PRINTF("- Версия: %d.%d\n", (versiondata >> 16) & 0xFF, (versiondata >> 8) & 0xFF);
    
    return true;
}

bool RFIDManager::configurePN532() {
    // Быстрая конфигурация для максимальной производительности
    nfc->SAMConfig();
    
    DEBUG_PRINTLN("RFIDManager: PN532 сконфигурирован для ISO14443A карт");
    return true;
}

ScanResult RFIDManager::scanCard() {
    totalReads++;
    
    // Проверяем таймаут для неблокирующей работы
    if (!isTimeForRead()) {
        return SCAN_NO_CARD;
    }
    
    lastReadAttempt = millis();
    
    // Проверяем подключение
    if (!isConnected && !reconnect()) {
        incrementError();
        return SCAN_ERROR;
    }
    
    return scanCardFast();
}

ScanResult RFIDManager::scanCardFast() {
    totalReads++;
    
    // КРИТИЧЕСКАЯ ПРОВЕРКА: если PN532 не инициализирован
    if (nfc == nullptr) {
        incrementError();
        return SCAN_ERROR;
    }
    
    // Проверяем таймаут для неблокирующей работы
    if (!isTimeForRead()) {
        return SCAN_NO_CARD;
    }
    
    lastReadAttempt = millis();
    
    // Проверяем подключение
    if (!isConnected && !reconnect()) {
        incrementError();
        return SCAN_ERROR;
    }
    
    uint8_t uid[UID_BUFFER_SIZE];
    uint8_t uidLength;
    
    // Быстрое чтение с оптимизированным таймаутом
    bool cardFound = readPassiveTarget(uid, uidLength);
    
    if (!cardFound) {
        resetLastRead();
        return SCAN_NO_CARD;
    }
    
    successfulReads++;
    
    // Проверяем, изменилась ли карта
    bool cardChanged = false;
    
    if (!lastReadValid || uidLength != lastUIDLength) {
        cardChanged = true;
    } else {
        for (uint8_t i = 0; i < uidLength; i++) {
            if (uid[i] != lastUID[i]) {
                cardChanged = true;
                break;
            }
        }
    }
    
    // Сохраняем новые данные
    memcpy(lastUID, uid, uidLength);
    lastUIDLength = uidLength;
    lastReadValid = true;
    
    if (cardChanged && LOG_CARD_EVENTS) {
        DEBUG_PRINTF("RFIDManager: Карта найдена - UID: ");
        for (uint8_t i = 0; i < uidLength; i++) {
            DEBUG_PRINTF("%02X ", uid[i]);
        }
        DEBUG_PRINTLN("");
    }
    
    return cardChanged ? SCAN_CARD_CHANGED : SCAN_CARD_FOUND;
}

bool RFIDManager::readPassiveTarget(uint8_t* uid, uint8_t& uidLength) {
    // КРИТИЧЕСКАЯ ПРОВЕРКА: nfc должен существовать
    if (nfc == nullptr) {
        return false;
    }
    
    // Оптимизированное чтение с коротким таймаутом
    return nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, PN532_TIMEOUT_MS);
}

bool RFIDManager::reconnect() {
    if (!isTimeForReconnect()) {
        return false;
    }
    
    DEBUG_PRINTLN("RFIDManager: Попытка переподключения к PN532...");
    
    lastInitAttempt = millis();
    
    if (getFirmwareVersion()) {
        isConnected = true;
        DEBUG_PRINTLN("RFIDManager: Переподключение успешно");
        return true;
    }
    
    isConnected = false;
    incrementError();
    return false;
}

void RFIDManager::checkConnection() {
    // Периодическая проверка подключения
    static unsigned long lastCheck = 0;
    
    if (millis() - lastCheck > 10000) {  // Каждые 10 секунд
        if (!testConnection()) {
            isConnected = false;
            handleError("Потеряно соединение с PN532");
        }
        lastCheck = millis();
    }
}

bool RFIDManager::testConnection() {
    return getFirmwareVersion();
}

bool RFIDManager::getLastUID(uint8_t* uid, uint8_t& uidLength) const {
    if (!lastReadValid) {
        return false;
    }
    
    memcpy(uid, lastUID, lastUIDLength);
    uidLength = lastUIDLength;
    return true;
}

float RFIDManager::getSuccessRate() const {
    if (totalReads == 0) return 0.0;
    return (float)successfulReads / totalReads * 100.0;
}

void RFIDManager::resetStatistics() {
    totalReads = 0;
    successfulReads = 0;
    errors = 0;
    timeouts = 0;
    
    DEBUG_PRINTLN("RFIDManager: Статистика сброшена");
}

void RFIDManager::resetLastRead() {
    memset(lastUID, 0, sizeof(lastUID));
    lastUIDLength = 0;
    lastReadValid = false;
}

bool RFIDManager::isTimeForRead() const {
    return (millis() - lastReadAttempt) >= SCAN_DELAY_MS;
}

bool RFIDManager::isTimeForReconnect() const {
    return (millis() - lastInitAttempt) >= 5000;  // Попытка переподключения каждые 5 сек
}

void RFIDManager::handleError(const char* errorMessage) {
    errors++;
    
    if (LOG_ERROR_EVENTS) {
        DEBUG_PRINTF("RFIDManager: ОШИБКА #%lu: %s\n", errors, errorMessage);
    }
    
    isConnected = false;
}

void RFIDManager::printDiagnostics() const {
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTLN("RFID ДИАГНОСТИКА");
    DEBUG_PRINTLN("========================================");
    DEBUG_PRINTF("Инициализирован: %s\n", isInitialized ? "ДА" : "НЕТ");
    DEBUG_PRINTF("Подключен: %s\n", isConnected ? "ДА" : "НЕТ");
    DEBUG_PRINTF("Общее количество чтений: %lu\n", totalReads);
    DEBUG_PRINTF("Успешные чтения: %lu\n", successfulReads);
    DEBUG_PRINTF("Ошибки: %lu\n", errors);
    DEBUG_PRINTF("Таймауты: %lu\n", timeouts);
    DEBUG_PRINTF("Успешность: %.1f%%\n", getSuccessRate());
    DEBUG_PRINTF("Последнее чтение валидно: %s\n", lastReadValid ? "ДА" : "НЕТ");
    
    if (lastReadValid) {
        DEBUG_PRINTF("Последний UID (%d байт): ", lastUIDLength);
        for (uint8_t i = 0; i < lastUIDLength; i++) {
            DEBUG_PRINTF("%02X ", lastUID[i]);
        }
        DEBUG_PRINTLN("");
    }
    
    DEBUG_PRINTLN("========================================");
} 