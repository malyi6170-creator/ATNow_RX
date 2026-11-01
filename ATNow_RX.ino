/*
 * Проект: ATNow (Беспроводной протокол на 433 МГц для ATtiny13)
 * Роль: ПРИЕМНИК (RX)
 * Идея проекта: @nuclearlinuxuser[cite: 1]
 * Канал: Схема на колене
 */

#include <avr/io.h>
#include <util/delay.h>

// Назначение пинов
#define RX_PIN PB1  // Пин данных радиоприемника
#define LED_PIN PB2 // Светодиод индикации приема

#define BIT_DELAY 400 

/*
 * Функция чтения одного бита с защитой от зависания (таймаутом)
 */
uint8_t read_bit(void) {
    uint16_t timeout = 8000;
    
    // Ждем появления высокого уровня (поднятие фронта)
    while (!(PINB & (1 << RX_PIN))) {
        if (--timeout == 0) return 0xFF; // Если тишина слишком долго — выходим по таймауту
    }
    
    // Делаем паузу до середины импульса, чтобы считать значение
    _delay_us(BIT_DELAY + (BIT_DELAY / 2));
    uint8_t bit = (PINB & (1 << RX_PIN)) ? 1 : 0;
    
    // Ждем спада уровня обратно в 0
    timeout = 8000;
    while (PINB & (1 << RX_PIN)) {
        if (--timeout == 0) break;
    }
    
    return bit;
}

/*
 * Функция сборки целого байта из 8 бит
 */
uint8_t read_byte(void) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b = read_bit();
        if (b == 0xFF) return 0xFF; // Ошибка чтения
        result = (result << 1) | b;
    }
    return result;
}

int main(void) {
    // Настройка пинов: RX_PIN - вход, LED_PIN - выход
    DDRB &= ~(1 << RX_PIN); 
    DDRB |= (1 << LED_PIN);  
    
    PORTB &= ~(1 << LED_PIN); // Изначально выключаем светодиод
    
    while (1) {
        // Шаг 1: Ищем преамбулу 0xAA в эфирном шуме
        if (read_byte() == 0xAA) {          
            // Шаг 2: Проверяем, идет ли за ней синхробайт 0xD5
            if (read_byte() == 0xD5) {      
                
                // Шаг 3: Считываем данные и контрольную сумму
                uint8_t data = read_byte();
                uint8_t crc = read_byte();
                
                // Шаг 4: Проверяем контрольную сумму (XOR)
                if (crc == (0xAA ^ 0xD5 ^ data)) {
                    
                    // Шаг 5: Если данные совпали с нашей командой (0x42)
                    if (data == 0x42) {
                        PORTB ^= (1 << LED_PIN); // Инвертируем состояние светодиода (вкл/выкл)
                        _delay_ms(200);          // Антидребезг / защита от двойного срабатывания
                    }
                }
            }
        }
    }
    
    return 0;
}
