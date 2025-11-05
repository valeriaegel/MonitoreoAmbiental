//LIBRERÍAS
// Instala la librería "DHT sensor library" para que ESP32 se comunique y extraiga datos válidos del sensor
#include <DHT.h> 

//CONEXION WIFI:
#include <WiFi.h>
#define SSID "Wokwi-GUEST" 
#define PASSWORD ""

//Conexion BD
#include <FirebaseESP32.h>

// CREDENCIALES DE FIREBASE
#define FIREBASE_HOST "https://sistema-de-control-ambiental-default-rtdb.firebaseio.com/" // Reemplaza con la URL de tu base de datos
#define FIREBASE_AUTH "QJZVbxZ5NX4lTRvIO9KSFKzfPuLKXZ8unelcb0OS"              // Reemplaza con tu clave de autenticación (Secret)


// Define el objeto FirebaseData
FirebaseData firebaseData; 

// ******* NUEVAS VARIABLES GLOBALES NECESARIAS *******
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

//DEFINICIÓN DE PINES Y CONFIGURACIÓN
// ...

//DEFINICIÓN DE PINES Y CONFIGURACIÓN
// Pin de datos para el sensor DHT
#define DHTPIN 21       
// Tipo de sensor utilizado
#define DHTTYPE DHT22   

// Pin para el Buzzer Activo
#define BUZZER_PIN 2    

// Pines del LED RGB 
#define LED_R 18        // Rojo
#define LED_G 19        // Verde
#define LED_B 23        // Azul

// Umbrales de Temperatura y humedad definidos por el proyecto
#define TEMP_ALERTA 30.0  // Umbral superior (>= 30°C: ROJO y ALERTA)
#define TEMP_NORMAL 20.0  // Umbral inferior (< 20°C: AZUL)

// Inicialización del sensor
DHT dht(DHTPIN, DHTTYPE);

// FUNCIONES AUXILIARES
//Función para cambiar el color del LED RGB
void setRgbColor(int red, int green, int blue) {
  digitalWrite(LED_R, red);
  digitalWrite(LED_G, green);
  digitalWrite(LED_B, blue);
}

//Funcion para manajera led RGB y buzzer
void actualizarEstadoAmbiental(float temp, float humedad) {
  // Lógica de Alerta 
  if (temp > TEMP_ALERTA) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("ALERTA!! --> Temperatura alta");
  } else if(humedad>60){
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("ALERTA!! --> Humedad alta");
  } else  {
    digitalWrite(BUZZER_PIN, LOW);
  }
  
  // Lógica del LED RGB 
  if (temp > TEMP_ALERTA) { 
    // ROJO: si la temperatura es superior a 30 °C
    setRgbColor(LOW, HIGH, HIGH); 
    Serial.println(">>> LED: ROJO (Peligro)");
  } else if (temp >= TEMP_NORMAL) { 
    // VERDE: si la temperatura se encuentra entre 20 y 30 °C.
    setRgbColor(HIGH, LOW, HIGH); 
    Serial.println(">>> LED: VERDE (Normal)");
  } else { 
    // AZUL: si la temperatura es inferior a 20 °C.
    setRgbColor(HIGH, HIGH, LOW); 
    Serial.println(">>> LED: AZUL (Frío)");
  }
}


// FUNCIÓN PARA EL ENVÍO DE DATOS A FIREBASE
// FUNCIÓN PARA EL ENVÍO DE DATOS A FIREBASE
void enviarDatos(float temp, float hum) {
 if (WiFi.status() == WL_CONNECTED) {
 
 FirebaseJson json;
json.set("temperatura", temp); // 2 decimales
 json.set("humedad", hum); 
 json.set("timestamp", String(millis())); 

// --- 1. Guardar el dato actual (sobrescribe) ---
if (Firebase.setJSON(firebaseData, "/lecturas/actual", json)) { 
   Serial.println("Firebase: Dato actual enviado correctamente (JSON set).");
  } else {
   Serial.print("Firebase: FALLÓ el envío del dato actual. Razón: ");
  Serial.println(firebaseData.errorReason());
}

  // --- 2. Guardar el dato histórico (agrega un nuevo nodo) ---
    // Usamos pushJSON para crear una clave única bajo el nodo "historial"
   if (Firebase.pushJSON(firebaseData, "/lecturas/historial", json)) {
   Serial.println("Firebase: Dato histórico guardado correctamente (JSON push).");
      Serial.print("Ruta: ");
      Serial.println(firebaseData.dataPath()); // Muestra la ruta generada (con el ID único)
 } else {
 Serial.print("Firebase: FALLÓ el guardado histórico. Razón: ");
 Serial.println(firebaseData.errorReason());
 }

 } else {
Serial.println("Firebase: WiFi desconectado. No se pudieron enviar los datos.");
 }
}


//SETUP Y LOOP
void setup() {
  Serial.begin(115200);
  Serial.println("--- Sistema de Monitoreo Ambiental Inteligente ---");

// 1. CONEXIÓN WI-FI
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  // Serial.println("Conectando a WiFi...");
}

Serial.println("Conectado a WiFi!");

 // Configura el Host y el Secret
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;

    // Inicializa la configuración de Firebase
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    
    // Opcional: para evitar reconexiones Wi-Fi constantes
    Firebase.reconnectWiFi(true);

  // Configurar pines de salida para LED y Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);

  // Inicializar sensor DHT
  dht.begin();

  // Asegurar que el LED y Buzzer estén apagados al inicio
  setRgbColor(HIGH, HIGH, HIGH);
  digitalWrite(BUZZER_PIN, LOW);
}

void loop() {
  // Esperar un momento entre lecturas para evitar errores del sensor
  delay(2000); 

  // Leer Humedad y Temperatura
  float humedad = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verificar si la lectura fue exitosa
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println("¡Error al leer del sensor DHT!");
    return;
  }

  //LOGICA PARA ENVIAR DATOS 
  // A. Reporte de Datos (Simulación de "OK GOOGLE, ¿Qué temperatura/humedad hace?")
  // En un entorno real, esta data se enviaría a una nube (Firebase/MQTT) que Google Assistant consultaría.
  
  Serial.println("\n--- [Reporte de Datos IoT] ---");
  Serial.print("Temperatura actual: ");
  Serial.print(temperatura);
  Serial.println(" °C");
  Serial.print("Humedad actual: ");
  Serial.print(humedad);
  Serial.println(" %");


  enviarDatos(temperatura, humedad);
  
  // B. Lógica de Alerta Visual y Sonora
  actualizarEstadoAmbiental(temperatura,humedad);

  // Esperar 5 segundos antes de la siguiente lectura
  delay(5000); 

  // Para que la espera sea de 20 min: delay(1200000);
}