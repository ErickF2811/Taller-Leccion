#include <avr/io.h>
#include <avr/interrupt.h>

enum class Estado {
    ESTADO_0,
    ESTADO_1,
    ESTADO_2
};

volatile Estado estadoActual = Estado::ESTADO_0;

// Actualiza LEDs según estado
void mostrarEstado() {
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2)); // Apaga LEDs

    switch (estadoActual) {
        case Estado::ESTADO_0:
            PORTB |= (1 << PB0);
            
            break;
        case Estado::ESTADO_1:
            PORTB |= (1 << PB1);
            
            break;
        case Estado::ESTADO_2:
            PORTB |= (1 << PB2);
            
            break;
    }
}

// Función para retardos simples (aproximados)
void retardo_software_ms(uint16_t ms) {
    // Este retardo depende del reloj del micro y no es preciso.
    for (uint16_t i = 0; i < ms; i++) {
        for (volatile uint16_t j = 0; j < 1000; j++) {
            asm volatile ("nop");
        }
    }
}

ISR(INT0_vect) {
    retardo_software_ms(1000);  // debounce simple

    // Cambiar estado cíclicamente
    switch (estadoActual) {
        case Estado::ESTADO_0:
            estadoActual = Estado::ESTADO_1;
            break;
        case Estado::ESTADO_1:
            estadoActual = Estado::ESTADO_2;
            break;
        case Estado::ESTADO_2:
            estadoActual = Estado::ESTADO_0;
            break;
    }

    mostrarEstado();
}

int main(void) {
    cli(); // Deshabilitar interrupciones

    // Configurar PB0, PB1, PB2 como salidas (pines 8,9,10 Arduino)
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));

    // Configurar PD2 (INT0) como entrada con pull-up
    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);

    // Configurar interrupción INT0 en flanco de bajada
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);

    mostrarEstado();

    sei(); // Habilitar interrupciones globales

    while (1) {
        // Aquí nada: todo se maneja en ISR
    }

    return 0;
}
