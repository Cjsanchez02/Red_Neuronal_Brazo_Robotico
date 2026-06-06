#include <Wire.h>
#include "Adafruit_TCS34725.h"

/* * Inicializamos el sensor. 
 * 50MS de integración y Ganancia de 4X son excelentes parámetros 
 * para leer colores sólidos a corta distancia.
 */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

// =====================================================================
// ¡ATENCIÓN! Cambia este número antes de subir el código según el objeto:
// 0 = Objeto Rojo
// 1 = Objeto Verde
// 2 = Objeto Azul
// =====================================================================
int etiqueta_actual = 0; 

void setup() {
  // Iniciamos el monitor serie a alta velocidad
  Serial.begin(115200);
  
  // Iniciar pines I2C estándar del ESP32 (SDA = D21, SCL = D22)
  Wire.begin(); 

  // Verificamos que el sensor esté bien conectado
  if (tcs.begin()) {
    Serial.println("Sensor TCS34725 encontrado y listo.");
    delay(2000);
  } else {
    Serial.println("ERROR: No se encontró el sensor. Revisa los cables 3.3V, GND, SDA(21) y SCL(22).");
    while (1); // Congelar el programa si no hay sensor
  }
  
  // Imprimir el encabezado de las columnas para tu CSV
  Serial.println("R,G,B,Clear,Etiqueta");
}

void loop() {
  // Variables para guardar las lecturas
  uint16_t r, g, b, c;
  
  // Leer los valores puros del hardware
  tcs.getRawData(&r, &g, &b, &c);

  // Imprimir los valores separados estrictamente por comas
  Serial.print(r); Serial.print(",");
  Serial.print(g); Serial.print(",");
  Serial.print(b); Serial.print(",");
  Serial.print(c); Serial.print(",");
  Serial.println(etiqueta_actual);

  // Esperar medio segundo entre cada lectura (ajustable)
  delay(500);
}