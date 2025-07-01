#include "multiplexer.h"

// =============================================
// КЛАСС MULTIPLEXER
// =============================================

Multiplexer::Multiplexer(int muxNum) : muxNumber(muxNum) {
    currentAddress = 0;
    setPinMappings();
}

void Multiplexer::setPinMappings() {
    if (muxNumber == 1) {
        // Мультиплексор #1 (строки 0-7, S3=GND)
        s0Pin = MUX1_S0_PIN;
        s1Pin = MUX1_S1_PIN;
        s2Pin = MUX1_S2_PIN;
        s3Pin = -1; // S3 подключен к GND, не используется
    } else if (muxNumber == 2) {
        // Мультиплексор #2 (столбцы 0-11)
        s0Pin = MUX2_S0_PIN;
        s1Pin = MUX2_S1_PIN;
        s2Pin = MUX2_S2_PIN;
        s3Pin = MUX2_S3_PIN;
    } else {
        DEBUG_PRINTF("ОШИБКА: Неверный номер мультиплексора: %d\n", muxNumber);
    }
}

void Multiplexer::initialize() {
    // Настройка пинов как выходы
    pinMode(s0Pin, OUTPUT);
    pinMode(s1Pin, OUTPUT);
    pinMode(s2Pin, OUTPUT);
    
    // S3 пин только для мультиплексора #2 (столбцы)
    if (muxNumber == 2 && s3Pin >= 0) {
        pinMode(s3Pin, OUTPUT);
    }
    
    // Начальный адрес 0
    setAddress(0);
}

void Multiplexer::setAddress(int address) {
    if (!isValidAddress(address)) {
        DEBUG_PRINTF("ОШИБКА Multiplexer: Неверный адрес %d для мультиплексора #%d\n", address, muxNumber);
        return;
    }
    
    if (address != currentAddress) {
        updatePins(address);
        currentAddress = address;
    }
}

void Multiplexer::updatePins(int address) {
    // Оптимизированная установка битов адреса
    digitalWrite(s0Pin, (address & 0x01) ? HIGH : LOW);
    digitalWrite(s1Pin, (address & 0x02) ? HIGH : LOW);
    digitalWrite(s2Pin, (address & 0x04) ? HIGH : LOW);
    
    // S3 пин только для мультиплексора #2 (столбцы), для MUX1 S3=GND
    if (muxNumber == 2 && s3Pin >= 0) {
        digitalWrite(s3Pin, (address & 0x08) ? HIGH : LOW);
    }
    
    // Минимальная задержка стабилизации (оптимизированная до 2мкс)
    delayMicroseconds(MUX_SETTLE_TIME_US);
}

bool Multiplexer::isValidAddress(int address) const {
    // Мультиплексор #1 (строки): 0-7
    if (muxNumber == 1) {
        return (address >= 0 && address < MATRIX_ROWS);
    }
    // Мультиплексор #2 (столбцы): 0-11  
    else if (muxNumber == 2) {
        return (address >= 0 && address < MATRIX_COLS);
    }
    return false;
}

void Multiplexer::printStatus() const {
    // Убираем лишний вывод
}

// =============================================
// КЛАСС MULTIPLEXER MANAGER
// =============================================

MultiplexerManager::MultiplexerManager() : mux1(1), mux2(2) {
    currentRow = 0;
    currentCol = 0;
    isEnabled = false;
}

void MultiplexerManager::initialize() {
    mux1.initialize();
    mux2.initialize();
    
    // Настройка общего EN пина
    pinMode(MUX_COMMON_EN_PIN, OUTPUT);
    disableAll();  // Начальное состояние - выключен
    
    // Устанавливаем начальную позицию
    selectCell(0, 0);
}

void MultiplexerManager::selectCell(int row, int col) {
    if (!isValidCell(row, col)) {
        DEBUG_PRINTF("ОШИБКА MultiplexerManager: Неверная ячейка (%d, %d)\n", row, col);
        return;
    }
    
    // Оптимизированное переключение - обновляем только если изменилось
    if (row != currentRow) {
        mux1.setAddress(row);
        currentRow = row;
    }
    
    if (col != currentCol) {
        mux2.setAddress(col);
        currentCol = col;
    }
    
    // Автоматически включаем мультиплексоры после установки адреса
    enableAll();
}

void MultiplexerManager::enableAll() {
    digitalWrite(MUX_COMMON_EN_PIN, LOW);  // Активный LOW
    isEnabled = true;
}

void MultiplexerManager::disableAll() {
    digitalWrite(MUX_COMMON_EN_PIN, HIGH); // Отключение
    isEnabled = false;
}

void MultiplexerManager::selectCellByIndex(int cellIndex) {
    if (!isValidCellIndex(cellIndex)) {
        DEBUG_PRINTF("ОШИБКА MultiplexerManager: Неверный индекс ячейки %d\n", cellIndex);
        return;
    }
    
    int row, col;
    indexToRowCol(cellIndex, row, col);
    selectCell(row, col);
}

int MultiplexerManager::nextCell() {
    int nextIndex = getCurrentCellIndex() + 1;
    
    // Циклический переход к началу
    if (nextIndex >= MATRIX_TOTAL_CELLS) {
        nextIndex = 0;
    }
    
    selectCellByIndex(nextIndex);
    return nextIndex;
}

bool MultiplexerManager::isValidCell(int row, int col) const {
    return (row >= 0 && row < MATRIX_ROWS && col >= 0 && col < MATRIX_COLS);
}

bool MultiplexerManager::isValidCellIndex(int cellIndex) const {
    return (cellIndex >= 0 && cellIndex < MATRIX_TOTAL_CELLS);
}

void MultiplexerManager::indexToRowCol(int cellIndex, int& row, int& col) const {
    row = cellIndex / MATRIX_COLS;
    col = cellIndex % MATRIX_COLS;
}

int MultiplexerManager::rowColToIndex(int row, int col) const {
    return row * MATRIX_COLS + col;
}

void MultiplexerManager::printCurrentSelection() const {
    int cellIndex = getCurrentCellIndex();
    DEBUG_PRINTF("MultiplexerManager: Текущая ячейка [%d,%d] (индекс %d/%d), EN=%s\n", 
                 currentRow, currentCol, cellIndex, MATRIX_TOTAL_CELLS - 1, 
                 isEnabled ? "ВКЛ" : "ВЫКЛ");
} 