#include "state_manager.h"

StateManager::StateManager() {
    currentState = STATE_INIT;
    previousState = STATE_INIT;
    stateStartTime = 0;
    lastStateUpdate = 0;
    stateTransitions = 0;
    errorCount = 0;
}

void StateManager::initialize() {
    DEBUG_PRINTLN("StateManager: Инициализация");
    
    currentState = STATE_INIT;
    previousState = STATE_INIT;
    stateStartTime = millis();
    lastStateUpdate = millis();
    stateTransitions = 0;
    errorCount = 0;
    
    DEBUG_PRINTF("StateManager: Состояние установлено в %s\n", getStateName(currentState));
}

void StateManager::updateState() {
    unsigned long currentTime = millis();
    lastStateUpdate = currentTime;
    
    // ПРИМЕЧАНИЕ: Основная логика переходов управляется из main.cpp
    // StateManager используется для трекинга и статистики состояний
    // Автоматические переходы минимальны
    
    switch (currentState) {
        case STATE_INIT:
            // Состояние управляется из main.cpp
            break;
            
        case STATE_SCANNING:
            // Основное рабочее состояние - управляется из main.cpp
            break;
            
        case STATE_SCAN_CELL:
            // Детальные переходы управляются ScanMatrix
            break;
            
        case STATE_PROCESS_CARD:
            // Обработка карт управляется RFIDManager
            break;
            
        case STATE_SWITCH_CELL:
            // Переключение управляется MultiplexerManager
            break;
            
        case STATE_UPDATE_DISPLAY:
            // Обновления управляются DisplayManager
            break;
            
        case STATE_IDLE:
            // Режим ожидания - можем автоматически вернуться к сканированию
            if (getStateRunTime() > 1000) {
                setState(STATE_SCANNING);
            }
            break;
            
        case STATE_ERROR:
            // Состояние ошибки - управляется из main.cpp через handleErrorRecovery()
            break;
    }
}

void StateManager::setState(SystemState newState) {
    if (newState != currentState) {
        previousState = currentState;
        currentState = newState;
        stateStartTime = millis();
        stateTransitions++;
        
        // ФИЛЬТРАЦИЯ DEBUG СООБЩЕНИЙ - печатаем только важные переходы
        static unsigned long lastDebugOutput = 0;
        static uint32_t lastTransitionCount = 0;
        unsigned long currentTime = millis();
        
        bool shouldPrintDebug = false;
        
        // Печатаем debug только в следующих случаях:
        // 1. Переход в состояние ошибки - всегда
        // 2. Переход из состояния ошибки - всегда  
        // 3. Инициализация - всегда
        // 4. Обычные переходы - максимум раз в 5 секунд
        if (currentState == STATE_ERROR || previousState == STATE_ERROR || 
            currentState == STATE_INIT || previousState == STATE_INIT) {
            shouldPrintDebug = true;
        } else if ((currentTime - lastDebugOutput) >= 5000) {  // Раз в 5 секунд
            // Убираем спам статистики
            lastDebugOutput = currentTime;
            lastTransitionCount = stateTransitions;
            shouldPrintDebug = false;
        }
        
        if (shouldPrintDebug && ENABLE_SERIAL_DEBUG) {
            DEBUG_PRINTF("StateManager: %s -> %s (переход #%lu)\n", 
                        getStateName(previousState), 
                        getStateName(currentState),
                        stateTransitions);
        }
    }
}

unsigned long StateManager::getStateDelay() const {
    // Возвращает задержку для текущего состояния
    switch (currentState) {
        case STATE_INIT:
            return 50;  // Быстрая инициализация
            
        case STATE_SCANNING:
            return 1;   // Максимальная скорость для основного сканирования
            
        case STATE_SCAN_CELL:
            return SCAN_DELAY_MS;  // 5мс оптимизированная задержка
            
        case STATE_PROCESS_CARD:
            return 1;   // Быстрая обработка
            
        case STATE_SWITCH_CELL:
            return 1;   // Быстрое переключение
            
        case STATE_UPDATE_DISPLAY:
            return 10;  // Умеренная скорость для дисплея
            
        case STATE_IDLE:
            return 100; // Медленнее в режиме ожидания
            
        case STATE_ERROR:
            return 100; // Медленнее в состоянии ошибки
            
        default:
            return 10;  // По умолчанию
    }
}

bool StateManager::isTimeForStateUpdate() const {
    return (millis() - lastStateUpdate) >= getStateDelay();
}

void StateManager::reset() {
    DEBUG_PRINTLN("StateManager: Сброс состояния");
    
    currentState = STATE_INIT;
    previousState = STATE_INIT;
    stateStartTime = millis();
    lastStateUpdate = millis();
    
    // Не сбрасываем счетчики для статистики
}

void StateManager::handleError(const char* errorMessage) {
    errorCount++;
    
    DEBUG_PRINTF("StateManager: ОШИБКА #%lu: %s\n", errorCount, errorMessage);
    
    setState(STATE_ERROR);
}

void StateManager::printCurrentState() const {
    DEBUG_PRINTF("StateManager: Текущее состояние: %s (время выполнения: %lums)\n", 
                getStateName(currentState), getStateRunTime());
}

const char* StateManager::getStateName(SystemState state) const {
    switch (state) {
        case STATE_INIT:           return "INIT";
        case STATE_SCANNING:       return "SCANNING";
        case STATE_SCAN_CELL:      return "SCAN_CELL";
        case STATE_PROCESS_CARD:   return "PROCESS_CARD";
        case STATE_SWITCH_CELL:    return "SWITCH_CELL";
        case STATE_UPDATE_DISPLAY: return "UPDATE_DISPLAY";
        case STATE_IDLE:           return "IDLE";
        case STATE_ERROR:          return "ERROR";
        default:                   return "UNKNOWN";
    }
} 