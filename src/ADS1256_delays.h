#ifndef ADS1256_DELAYS_H
#define ADS1256_DELAYS_H

#include <Arduino.h>

// Cicli di NOP per ESP32 a 240MHz
// Un ciclo = ~4.17ns
inline void T1Delay() {  // 100ns richiesti
    __asm__ __volatile__ ("nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t" ::); // 24 NOPs = ~100ns
}

inline void T2Delay() {  // 50ns richiesti
    __asm__ __volatile__ ("nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t" ::); // 12 NOPs = ~50ns
}

inline void T3Delay() {  // 50ns richiesti
    __asm__ __volatile__ ("nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t" ::); // 12 NOPs = ~50ns
}

inline void T4Delay() {  // 50ns richiesti
    __asm__ __volatile__ ("nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t" ::); // 12 NOPs = ~50ns
}

inline void T5Delay() {  // 3.125us richiesti (24 * 1/7.68MHz)
    // Per 3.125us a 240MHz necessitiamo circa 750 cicli
    for(uint16_t i = 0; i < 250; i++) {
        __asm__ __volatile__ ("nop\n\t"
                             "nop\n\t"
                             "nop\n\t" ::);
    }
}

inline void T6Delay() {  // 50ns richiesti
    __asm__ __volatile__ ("nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t"
                         "nop\n\t" ::); // 12 NOPs = ~50ns
}

inline void T11Delay() {  // 25ms richiesti
    delay(25);  // Usiamo delay standard per tempi lunghi
}

inline void TSettleDelay() {  // 400us richiesti per 30000 SPS
    delayMicroseconds(400);  // Usiamo delayMicroseconds per tempi medi
}

inline void delay500ns() {
    volatile uint8_t i = 40;
    while(i--) {
        NOP();
    }
}

#endif // ADS1256_DELAYS_H