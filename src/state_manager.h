#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "config.h"

class StateManager {
private:
    SystemState currentState;
    SystemState previousState;
    unsigned long stateStartTime;
    unsigned long lastStateUpdate;
    
    // Счетчики и статистика
    uint32_t stateTransitions;
    uint32_t errorCount;
    
public:
    StateManager();
    
    // Основные методы управления состояниями
    void initialize();
    void updateState();
    void setState(SystemState newState);
    
    // Геттеры состояния
    SystemState getCurrentState() const { return currentState; }
    SystemState getPreviousState() const { return previousState; }
    unsigned long getStateRunTime() const { return millis() - stateStartTime; }
    
    // Проверки состояний
    bool isState(SystemState state) const { return currentState == state; }
    bool hasStateChanged() const { return currentState != previousState; }
    bool isInErrorState() const { return currentState == STATE_ERROR; }
    
    // Управление таймингами
    unsigned long getStateDelay() const;
    bool isTimeForStateUpdate() const;
    
    // Статистика и диагностика
    uint32_t getStateTransitions() const { return stateTransitions; }
    uint32_t getErrorCount() const { return errorCount; }
    void incrementErrorCount() { errorCount++; }
    
    // Сброс и перезапуск
    void reset();
    void handleError(const char* errorMessage);
    
    // Отладка
    void printCurrentState() const;
    const char* getStateName(SystemState state) const;
};

#endif // STATE_MANAGER_H 