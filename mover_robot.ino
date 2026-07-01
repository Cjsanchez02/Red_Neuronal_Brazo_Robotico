#include <ESP32Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h" 
#include "modelo_svm.h"        

// ==========================================
// 1. DECLARACIONES GLOBALES 
// ==========================================
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Eloquent::ML::Port::SVM svm;

Servo servoBase;
Servo servoHombro;
Servo servoCodo;
Servo servoPinza;

// ==========================================
// 2. CONFIGURACIÓN INICIAL
// ==========================================
void setup() {
  Serial.begin(115200);
  
  if (tcs.begin()) {
    Serial.println("Sensor TCS34725 encontrado!");
  } else {
    Serial.println("No se encontró el sensor TCS34725. Revisa tus conexiones.");
    while (1); 
  }

  // Conectar servos a los pines de tu ESP32
  servoBase.attach(12);
  servoHombro.attach(14);
  servoCodo.attach(27);
  servoPinza.attach(26);

  // --- AGREGA ESTO PARA DARLES UNA POSICIÓN INICIAL ---
  servoBase.write(90);
  servoHombro.write(60); 
  servoCodo.write(60);
  servoPinza.write(90);
  delay(1000);
}

// ==========================================
// 3. BUCLE PRINCIPAL (Modo Manual)
// ==========================================
void loop() {
  if (Serial.available() > 0) {
    char comando = Serial.read();
    
    if (comando == 'L' || comando == 'l') {
      Serial.println(">>> Leyendo color...");
      
      uint16_t r, g, b, c;
      tcs.getRawData(&r, &g, &b, &c); 
      float caracteristicas[3] = { (float)r, (float)g, (float)b };
      int prediccion = svm.predict(caracteristicas);

      // PASO 1: Asegurar y levantar
      Serial.println("1. Agarrando y levantando objeto...");
      agarrarObjeto();
      levantarBrazo();

      // PASO 2: Girar la base
      switch (prediccion) {
        case 0: // Verde
          Serial.println("Color detectado: VERDE -> Moviendo a caja Verde");
          moverServoLento(servoBase, 120, 25); 
          break;
          
        case 1: // Rojo
          Serial.println("Color detectado: ROJO -> Moviendo a caja Roja");
          moverServoLento(servoBase, 45, 25); 
          break;
          
        case 2: // Azul
          Serial.println("Color detectado: AZUL -> Moviendo a caja Azul");
          moverServoLento(servoBase, 150, 25); 
          break;
      }
      delay(500); 

      // PASO 3: Bajar y soltar
      Serial.println("3. Soltando objeto...");
      bajarBrazo();
      soltarObjeto();

      // PASO 4: Regresar
      Serial.println("4. Regresando a posición inicial...");
      volverAlInicio();
      
      Serial.println(">>> Ciclo terminado. Esperando nueva orden (L)...");
    }
  }
}

// ==========================================
// 4. FUNCIONES AUXILIARES 
// ==========================================

// Función para mover suavemente
void moverServoLento(Servo &motor, int anguloDestino, int lentitud) {
  int anguloActual = motor.read(); 
  
  if (anguloActual < anguloDestino) {
    for (int pos = anguloActual; pos <= anguloDestino; pos++) {
      motor.write(pos);
      delay(lentitud); 
    }
  } else {
    for (int pos = anguloActual; pos >= anguloDestino; pos--) {
      motor.write(pos);
      delay(lentitud);
    }
  }
}

void agarrarObjeto() {
  moverServoLento(servoCodo, 90, 15);
  moverServoLento(servoPinza, 0, 15); 
  delay(500);           
}

void levantarBrazo() {
  moverServoLento(servoHombro, 90, 20); 
  moverServoLento(servoCodo, 70, 20);
  delay(500); 
}

void bajarBrazo() {
  // Ajusta la altura de bajada aquí
  moverServoLento(servoHombro, 60, 20); 
  moverServoLento(servoCodo, 90, 20);
  delay(500);
}

void soltarObjeto() {
  moverServoLento(servoCodo, 60, 15);
  moverServoLento(servoPinza, 90, 15); 
  delay(500);
}

void volverAlInicio() {
  moverServoLento(servoHombro, 60, 20);
  delay(200);
  
  moverServoLento(servoBase, 90, 25); 
  delay(200);
  
  // Ajusta la altura de reposo aquí
  moverServoLento(servoHombro, 60, 20); 
  moverServoLento(servoCodo, 70, 20);   
  delay(500);
}