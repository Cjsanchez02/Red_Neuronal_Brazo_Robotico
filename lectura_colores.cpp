#include <Wire.h>
#include "Adafruit_TCS34725.h"

// Inicializamos el sensor con ganancia de 4X y tiempo de integración de 50ms
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);

void setup() {
  Serial.begin(115200);
  
  if (tcs.begin()) {
    Serial.println("=========================================");
    Serial.println("MODO RECOLECCIÓN EN FORMATO HSV ACTIVADO");
    Serial.println("=========================================");
    Serial.println("Escribe 'r', 'v' o 'a' y presiona Enter para capturar.");
    Serial.println("=========================================");
    
    // Cabecera del CSV optimizada con RGB y HSV
    Serial.println("R,G,B,C,H,S,V,Clase"); 
  } else {
    Serial.println("ERROR: No se encuentra el sensor.");
    while (1);
  }
}

void loop() {
  if (Serial.available() > 0) {
    char tecla = Serial.read();
    String etiquetaColor = "";

    if (tecla == 'r' || tecla == 'R') etiquetaColor = "Rojo";
    else if (tecla == 'v' || tecla == 'V') etiquetaColor = "Verde";
    else if (tecla == 'a' || tecla == 'A') etiquetaColor = "Azul";
    
    if (etiquetaColor != "") {
      uint16_t r, g, b, c;
      tcs.getRawData(&r, &g, &b, &c);
      
      // --- ALGORITMO DE CONVERSIÓN RGB A HSV ---
      
      // 1. Evitamos división por cero si el sensor se tapa por completo
      float clearChannel = (c == 0) ? 1.0 : (float)c;
      
      // 2. Normalización: Dividimos entre C para eliminar el brillo ambiental
      float r_norm = (float)r / clearChannel;
      float g_norm = (float)g / clearChannel;
      float b_norm = (float)b / clearChannel;
      
      // 3. Encontrar el valor Máximo y Mínimo de los canales normalizados
      float c_max = r_norm;
      if (g_norm > c_max) c_max = g_norm;
      if (b_norm > c_max) c_max = b_norm;
      
      float c_min = r_norm;
      if (g_norm < c_min) c_min = g_norm;
      if (b_norm < c_min) c_min = b_norm;
      
      float delta = c_max - c_min;
      
      // 4. Cálculo del Matiz (Hue - H) en grados (0 - 360)
      float h = 0;
      if (delta > 0) {
        if (c_max == r_norm) {
          h = 60.0 * ((g_norm - b_norm) / delta);
        } else if (c_max == g_norm) {
          h = 60.0 * (((b_norm - r_norm) / delta) + 2.0);
        } else if (c_max == b_norm) {
          h = 60.0 * (((r_norm - g_norm) / delta) + 4.0);
        }
        
        // Si el ángulo da negativo, le damos la vuelta al círculo cromático
        if (h < 0) h += 360.0;
      }
      
      // 5. Cálculo de la Saturación (S) de 0.0 a 1.0
      float s = (c_max > 0) ? (delta / c_max) : 0;
      
      // 6. Cálculo del Brillo/Valor (V) de 0.0 a 1.0
      float v = c_max;
      
      // --- ENVIAR DATOS POR CONSOLA EN FORMATO CSV ---
      Serial.print(r); Serial.print(",");
      Serial.print(g); Serial.print(",");
      Serial.print(b); Serial.print(",");
      Serial.print(c); Serial.print(",");
      Serial.print(h, 2); Serial.print(","); // 2 decimales para precisión
      Serial.print(s, 4); Serial.print(","); 
      Serial.print(v, 4); Serial.print(",");
      Serial.println(etiquetaColor);
    }
  }
}
