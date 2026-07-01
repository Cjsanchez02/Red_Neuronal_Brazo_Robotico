#include <ESP32Servo.h>
#include <Wire.h>
#include "Adafruit_TCS34725.h" 
#include "modelo_svm.h" 
#include <WiFi.h>       
#include <WebServer.h>

// ==========================================
// 1. DECLARACIONES GLOBALES Y WIFI
// ==========================================
const char* ssid = "MERCUSYS_963A6D";
const char* password = "A122A1230000082321"; 

WebServer server(80); // El puerto 80 es el estándar para páginas web

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_50MS, TCS34725_GAIN_4X);
Eloquent::ML::Port::SVM svm;

Servo servoBase;
Servo servoHombro;
Servo servoCodo;
Servo servoPinza;

// Variables para contar los aciertos
int aciertos = 0;
int fallos = 0;

// ==========================================
// 2. PÁGINA WEB (HTML + JavaScript)
// ==========================================
const char pagina_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Control Brazo IA</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin-top: 30px; background-color: #f4f4f9; }
    button { padding: 15px 30px; font-size: 20px; margin: 10px; cursor: pointer; border-radius: 10px; border: none; font-weight: bold; }
    #btnAgarrar { background-color: #3498db; color: white; width: 80%; padding: 20px;}
    .btnSi { background-color: #2ecc71; color: white; }
    .btnNo { background-color: #e74c3c; color: white; }
    .oculto { display: none; }
    .caja { background: white; margin: 20px auto; padding: 20px; width: 80%; border-radius: 10px; box-shadow: 0px 4px 6px rgba(0,0,0,0.1); }
  </style>
</head>
<body>
  <h1>Brazo Robótico IA</h1>
  
  <button id="btnAgarrar" onclick="iniciarRobot()">AGARRAR Y LEER</button>
  <h2 id="estado">Esperando orden...</h2>

  <div id="zonaPregunta" class="caja oculto">
    <h3>¿El robot lo clasificó bien?</h3>
    <button class="btnSi" onclick="enviarRespuesta(1)">SÍ, Correcto</button>
    <button class="btnNo" onclick="enviarRespuesta(0)">NO, Falló</button>
  </div>

  <div class="caja">
    <h3>Estadísticas de la IA</h3>
    <p>Aciertos: <strong id="txtAciertos">0</strong></p>
    <p>Fallos: <strong id="txtFallos">0</strong></p>
    <p>Precisión: <strong id="txtPrecision">0</strong>%</p>
  </div>

  <script>
    function iniciarRobot() {
      document.getElementById("estado").innerText = "El robot se está moviendo...";
      document.getElementById("zonaPregunta").classList.add("oculto");
      document.getElementById("btnAgarrar").disabled = true;

      // Llama al ESP32 para que ejecute el movimiento
      fetch('/agarrar').then(response => response.text()).then(colorDetectado => {
        document.getElementById("estado").innerText = "Color detectado: " + colorDetectado;
        document.getElementById("zonaPregunta").classList.remove("oculto");
        document.getElementById("btnAgarrar").disabled = false;
      });
    }

    function enviarRespuesta(esCorrecto) {
      document.getElementById("zonaPregunta").classList.add("oculto");
      document.getElementById("estado").innerText = "Guardando estadística...";

      // Envía la respuesta al ESP32
      fetch('/feedback?acierto=' + esCorrecto).then(response => response.json()).then(datos => {
        document.getElementById("txtAciertos").innerText = datos.aciertos;
        document.getElementById("txtFallos").innerText = datos.fallos;
        document.getElementById("txtPrecision").innerText = datos.precision;
        document.getElementById("estado").innerText = "Esperando siguiente orden...";
      });
    }
  </script>
</body>
</html>
)rawliteral";

// ==========================================
// 3. FUNCIONES DEL SERVIDOR WEB
// ==========================================

void paginaPrincipal() {
  server.send(200, "text/html", pagina_html);
}

void accionAgarrar() {
  String nombreColor = "";
  
  // 1. Leer color y predecir
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c); 
  float caracteristicas[3] = { (float)r, (float)g, (float)b };
  int prediccion = svm.predict(caracteristicas);

  // 2. Rutina de movimiento usando TUS calibraciones
  agarrarObjeto();
  levantarBrazo();

  switch (prediccion) {
    case 0: // Verde
      nombreColor = "VERDE";
      moverServoLento(servoBase, 120, 25); 
      break;
    case 1: // Rojo
      nombreColor = "ROJO";
      moverServoLento(servoBase, 45, 25); 
      break;
    case 2: // Azul
      nombreColor = "AZUL";
      moverServoLento(servoBase, 150, 25); 
      break;
  }
  
  delay(500); 
  bajarBrazo();
  soltarObjeto();
  volverAlInicio();

  // 3. Responder al teléfono con el color que detectó
  server.send(200, "text/plain", nombreColor);
}

void accionFeedback() {
  String respuesta = server.arg("acierto");
  
  if (respuesta == "1") {
    aciertos++;
  } else if (respuesta == "0") {
    fallos++;
  }

  int total = aciertos + fallos;
  float precision = 0.0;
  if (total > 0) {
    precision = ((float)aciertos / total) * 100.0;
  }

  String json = "{\"aciertos\":" + String(aciertos) + ",\"fallos\":" + String(fallos) + ",\"precision\":" + String(precision, 1) + "}";
  server.send(200, "application/json", json);
}

// ==========================================
// 4. CONFIGURACIÓN INICIAL
// ==========================================
void setup() {
  Serial.begin(115200);
  
  if (!tcs.begin()) {
    Serial.println("No se encontró el sensor TCS34725. Revisa tus conexiones.");
    while (1); 
  }

  servoBase.attach(12);
  servoHombro.attach(14);
  servoCodo.attach(27);
  servoPinza.attach(26);

  // TUS POSICIONES INICIALES
  servoBase.write(90);
  servoHombro.write(60); 
  servoCodo.write(60);
  servoPinza.write(90);
  delay(1000);

  // Conectar a WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n¡WiFi Conectado!");
  
  Serial.print("Abre tu navegador web en el celular y escribe esta IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, paginaPrincipal); 
  server.on("/agarrar", HTTP_GET, accionAgarrar); 
  server.on("/feedback", HTTP_GET, accionFeedback); 
  
  server.begin();
}

// ==========================================
// 5. BUCLE PRINCIPAL
// ==========================================
void loop() {
  // El ESP32 solo escucha peticiones del celular
  server.handleClient();
}

// ==========================================
// 6. TUS FUNCIONES DE MOVIMIENTO (Calibradas)
// ==========================================
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
  
  moverServoLento(servoHombro, 60, 20); 
  moverServoLento(servoCodo, 70, 20);   
  delay(500);
}