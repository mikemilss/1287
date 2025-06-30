#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <Arduino.h>
#include "config.h"

class Multiplexer {
private:
    int muxNumber;         // Номер мультиплексора (1 или 2)
    int s0Pin, s1Pin, s2Pin, s3Pin;  // Управляющие пины
    int currentAddress;    // Текущий адрес
    
public:
    Multiplexer(int muxNum);
    
    // Инициализация
    void initialize();
    
    // Управление адресом
    void setAddress(int address);
    int getCurrentAddress() const { return currentAddress; }
    
    // Валидация
    bool isValidAddress(int address) const;
    
    // Отладка
    void printStatus() const;
    
private:
    void updatePins(int address);
    void setPinMappings();
};

// Глобальные функции для управления обоими мультиплексорами
class MultiplexerManager {
private:
    Multiplexer mux1;  // Строки (0-7)
    Multiplexer mux2;  // Столбцы (0-11)
    
    int currentRow;
    int currentCol;
    bool isEnabled;    // Состояние общего EN пина
    
public:
    MultiplexerManager();
    
    // Инициализация
    void initialize();
    
    // Выбор ячейки матрицы
    void selectCell(int row, int col);
    void selectCellByIndex(int cellIndex);  // 0-95
    
    // Получение текущей позиции
    int getCurrentRow() const { return currentRow; }
    int getCurrentCol() const { return currentCol; }
    int getCurrentCellIndex() const { return currentRow * MATRIX_COLS + currentCol; }
    
    // Переключение на следующую ячейку
    int nextCell();  // Возвращает индекс следующей ячейки
    
    // Управление общим EN пином
    void enableAll();
    void disableAll();
    bool getEnabled() const { return isEnabled; }
    
    // Валидация
    bool isValidCell(int row, int col) const;
    bool isValidCellIndex(int cellIndex) const;
    
    // Конвертация координат
    void indexToRowCol(int cellIndex, int& row, int& col) const;
    int rowColToIndex(int row, int col) const;
    
    // Отладка
    void printCurrentSelection() const;
};

#endif // MULTIPLEXER_H 