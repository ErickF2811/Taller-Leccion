#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <LiquidCrystal_I2C.h>
#include <Arduino.h>

enum class Estado {
  ESTADO_0,
  ESTADO_1,
  ESTADO_2
};

struct EstadoRetorno {
  int valor;
  const char* texto;
};

LiquidCrystal_I2C lcd(0x27, 16, 2);
volatile Estado estadoActual = Estado::ESTADO_0;
volatile uint8_t estado_led = 0;
float estadoAnterior = 0.0;
float estado = 0.0;

EstadoRetorno mostrarEstado() {
  switch (estadoActual) {
    case Estado::ESTADO_0: return {1, "cm"};
    case Estado::ESTADO_1: return {0.32, "ft"};
    case Estado::ESTADO_2: return {1.6, "pg"};
  }
  return {0, ""};
}

ISR(INT0_vect) {
  for (volatile uint32_t i = 0; i < 100000UL; i++) {}  // Retardo simple para rebote

  switch (estadoActual) {
    case Estado::ESTADO_0: estadoActual = Estado::ESTADO_1; break;
    case Estado::ESTADO_1: estadoActual = Estado::ESTADO_2; break;
    case Estado::ESTADO_2: estadoActual = Estado::ESTADO_0; break;
  }
  // mostrarEstado();
}

ISR(TIMER1_COMPA_vect) {
  if (!(PIND & (1 << PD3))) {
    PORTD |= (1 << PD6);
    estado_led = 1;
  } else {
    if (estado_led) {
      PORTD &= ~(1 << PD6);
      estado_led = 0;
    } else {
      PORTD |= (1 << PD6);
      estado_led = 1;
    }
  }
}

void init_pines() {
  // Salidas PB0, PB1, PB2
  DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2);

  // Salida PD6 (LED)
  DDRD |= (1 << PD6);

  // Entradas PD2 (INT0) y PD3 (botón), con pull-up
  DDRD &= ~((1 << PD2) | (1 << PD3));
  PORTD |= (1 << PD2) | (1 << PD3);
}


void ADC_Init() {
  ADMUX &= ~((1 << REFS1) | (1 << REFS0)); // AREF externa
  ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Habilitar ADC y configurar prescaler en 128
}

float ADC_ReadVoltage(uint8_t channel) {
  ADMUX = (ADMUX & 0xF0) | (channel & 0x0F); // Seleccionar el canal ADC
  DIDR0 |= (1 << channel);                   // Deshabilitar funcionalidad digital en el canal ADC

  ADCSRA |= (1 << ADSC);                     // Iniciar conversión ADC
  while (ADCSRA & (1 << ADSC));              // Esperar a que termine la conversión

  return (ADC * 3.3) / 1023.0;               // Convertir a voltaje (referencia AREF externa)
}

float funcion0(float v) {
  return 0.1839 * pow(v, 6) - 1.5651 * pow(v, 5) + 4.7103 * pow(v, 4)
       - 5.7343 * pow(v, 3) + 2.3552 * pow(v, 2) + 0.4217 * v - 0.0024;
}

// Aproximación polinómica para calcular la distancia cuando funcion = 1
float funcion1(float v) {
  return 3.8697 * pow(v, 6) - 42.279 * pow(v, 5) + 184.25 * pow(v, 4)
       - 410.28 * pow(v, 3) + 497.79 * pow(v, 2) - 325.53 * v + 105.37;
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

void init_INT0() {
  // Interrupción externa INT0 por flanco de bajada
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);
  EIMSK |= (1 << INT0);
}

void init_Timer1() {
  TCCR1A = 0;
  TCCR1B = (1 << WGM12) | (1 << CS12); // CTC, prescaler 256
  OCR1A = 31249; // 0.5 s
  TIMSK1 |= (1 << OCIE1A);
}

void mostrarDistancia(float distancia, const char* unidad) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(unidad);
  lcd.print(distancia, 2); // 2 decimales
  // lcd.print(" cm");
}

void setup() {
  init_pines();
  init_INT0();
  init_Timer1();
  sei();
  mostrarEstado();
  lcd.init();
  lcd.backlight();

}

void loop() {
//   float distancia;
  
  // distancia = measuring();
  if (!(PIND & (1 << PD3))) {
    // PORTD |= (1 << PD6);
    estado = estadoAnterior;
  } else {
    estado = measuring();
    estadoAnterior = estado;
  }
 
  EstadoRetorno res = mostrarEstado();
  int numero = res.valor;
  const char* unidad = res.texto;

  mostrarDistancia(estado*numero, unidad );
  delay(500);
 
  // No hace nada, interrupciones se encargan de todo
}