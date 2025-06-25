#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2); // 0x27


//this file include all functions
#include "funciones.h"
#include "estados.h"

Estado estadoAnterior = Estado::ESTADO_0;
volatile Estado estadoActual = Estado::ESTADO_0;
volatile uint8_t estado_led = 0;


float distance; //se actualizara

ISR(INT0_vect) {
    retardo_software_ms(200);  // debounce simple

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

    // mostrarEstado();
}

int main(void) {
    cli(); // Deshabilitar interrupciones
    distance = 0;
    // Configurar PB0, PB1, PB2 como salidas (pines 8,9,10 Arduino)
    DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2));
    DDRD |= (1 << PD6);

    // Configurar PD2 (INT0) como entrada con pull-up
    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);
        // PD3 (pin 7) como entrada (botón)
    DDRD &= ~(1 << PD3);
    PORTD |= (1 << PD3); // Pull-up activado (botón entre pin 7 y GND)

    // Configura Timer1 en modo CTC
    TCCR1B |= (1 << WGM12);  // Modo CTC
    TCCR1B |= (1 << CS12);   // Preescaler 256
    OCR1A = 31249;           // ~500ms (para 16MHz)
    TIMSK1 |= (1 << OCIE1A); // Habilita interrupción de comparación
    // Configurar interrupción INT0 en flanco de bajada
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);

    

    sei(); // Habilitar interrupciones globales

    //Configuramos la lcd
    lcd.init();
  
    //Encender la luz de fondo.
    lcd.backlight();

    while (1) {

        /*
      if (estadoAnterior != estadoActual) {
        estadoAnterior = estadoActual;
        mostrarEstado();
      }*/

      if(PIND & (1 << 3)){
        //mide la distancia
        distance = measuring();
      }

      mostrarEstado(distance, &lcd, estadoActual);


    }

    return 0;
}


// ISR del Timer1
ISR(TIMER1_COMPA_vect) {
    // Si el botón (PD) está presionado (nivel bajo)
    if (!(PIND & (1 << PD3))) {
        // Mantener LED encendido
        PORTD |= (1 << PD6);
        estado_led = 1;
    } else {
        // Parpadeo normal
        if (estado_led) {
            PORTD &= ~(1 << PD6); // Apaga
            estado_led = 0;
        } else {
            PORTD |= (1 << PD6);  // Enciende
            estado_led = 1;
        }
    }
}
