#include <avr/io.h>
#include <math.h>
#include <LiquidCrystal_I2C.h>
#include "estados.h"



/**
  *Esta funcion inicializa el ADC y lo habilita para su uso
  * por defecto utiliza un pre-escalar en 128
*/
void ADC_Init() {
  ADMUX &= ~((1 << REFS1) | (1 << REFS0)); // AREF externa
  ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Habilitar ADC y configurar prescaler en 128
}


// Lectura y normalización del valor ADC
float ADC_ReadVoltage(uint8_t channel) {
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // Seleccionar el canal ADC
  DIDR0 |= (1 << channel);                   // Deshabilitar funcionalidad digital en el canal ADC

  ADCSRA |= (1 << ADSC);                     // Iniciar conversión ADC
  while (ADCSRA & (1 << ADSC));              // Esperar a que termine la conversión

  return (ADC * 3.3) / 1023.0;               // Convertir a voltaje (referencia AREF externa)
}



// Aproximación polinómica para calcular la distancia cuando funcion = 0
float funcion0(float v) {
  return 0.1839 * pow(v, 6) - 1.5651 * pow(v, 5) + 4.7103 * pow(v, 4)
       - 5.7343 * pow(v, 3) + 2.3552 * pow(v, 2) + 0.4217 * v - 0.0024;
}

// Aproximación polinómica para calcular la distancia cuando funcion = 1
float funcion1(float v) {
  return 3.8697 * pow(v, 6) - 42.279 * pow(v, 5) + 184.25 * pow(v, 4)
       - 410.28 * pow(v, 3) + 497.79 * pow(v, 2) - 325.53 * v + 105.37;
}



// Actualiza LEDs según estado
void mostrarEstado(float distance, LiquidCrystal_I2C *lcd, Estado estadoActual) {
    PORTB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2)); // Apaga LEDs
    lcd->setCursor(0, 1);


    switch (estadoActual) {
        case Estado::ESTADO_0:
            PORTB |= (1 << PB0);
            //Realiza la conversion
            
            break;
        case Estado::ESTADO_1:
            PORTB |= (1 << PB1);
            //Realiza la conversion
            break;
        case Estado::ESTADO_2:
            PORTB |= (1 << PB2);
            //Realiza la conversion
            break;
    }
    lcd->print(distance);
    lcd->print(" Segundos");
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

 
float measuring(){
  static float last_voltage = 0;      // Ultimo valor de voltaje leido
  float voltage = ADC_ReadVoltage(0); // Leer del canal 0
  char funcion = 0;


  if (voltage >= 3 && last_voltage < 3) {
    funcion = !funcion; // Cambiar el estado de funcion
  }
  last_voltage = voltage;
  
  // Rango válido para cada función
  bool valid_voltage = false;
  if (funcion == 0) valid_voltage = (voltage >= 0.01 && voltage <= 3.1);
  else              valid_voltage = (voltage >= 0.4 && voltage <= 3.1);

  // Calcular distancia si el voltaje es válido, sino mostrar "fuera del rango"
  float distance = 0;
  if (valid_voltage) {
    distance = (funcion == 0 ? funcion0(voltage) : funcion1(voltage)) * 100;
    if (distance < 0) distance = 0; // evitar valores negativos

  }

  return distance;
}