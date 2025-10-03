#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// varriables de connexion au reseau et e fire base, ici nous avons 
#define WIFI_SSID "Orange_Digital_Event"
#define WIFI_PASSWORD "ODC@event"
#define FIREBASE_HOST "firebase-adminsdk-tutaj@orange-9546e.iam.gserviceaccount.com"
#define FIREBASE_AUTH \
"-----BEGIN PRIVATE KEY-----\n" \
votre api \n" \
"-----END PRIVATE KEY-----\n"


#define DHTPIN 27         
#define DHTTYPE DHT11     
DHT dht(DHTPIN, DHTTYPE);
float humidite = 0;
float temperature = 0;

// LEDs
#define LED_RED 32    
#define LED_GREEN 33  

TaskHandle_t sendDataTask;
TaskHandle_t controlLEDsTask;

void setup() {
  Serial.begin(115200);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  dht.begin();
  connectToWiFi();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // Création des tâches FreeRTOS rrespectivement pour l'envoie des donnees a fire base et pour le controles des regles 
  xTaskCreatePinnedToCore(sendDataToFirebaseTask, "sendDataTask", 10000,  NULL, 1, &sendDataTask, 0);                         
  xTaskCreatePinnedToCore(controlLEDsTask,"controlLEDsTask", 10000,NULL,1,&controlLEDsTask,1); 
  // Fonction tâche. Nomtâche, Taille pile, Paramètres, Priorité,Gestionnaire/ Noyau  (0 ou 1)
                       
}


void loop() {
}


void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connection...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
}

void sendDataToFirebaseTask(void *pvParameters) {
  for (;;) {
    humidite = dht.readHumidity();
    temperature = dht.readTemperature();

    if (isnan(humidite) || isnan(temperature)) {
      Serial.println("humidité ?");
      vTaskDelay(2000 / portTICK_PERIOD_MS); 
      continue;
    }

    // gestion des chemin des noeud pour facilite l'identification des serveurs
    String nodePath = "/nodes/noeud1"; 

    // Envoie des donnees sur firebase 
    Firebase.setFloat(Firebase.RTDB, nodePath + "/humidite", humidite);
    Firebase.setFloat(Firebase.RTDB, nodePath + "/temperature", temperature);

    Serial.print("Humidite: ");
    Serial.print(humidite);
    Serial.print("% - Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");
    vTaskDelay(60000 / portTICK_PERIOD_MS); 
  }
}
//ici nous utilisons une boucle infinie pour recuperer les donnees jusqu'au respect des conditions
void controlLEDsTask(void *pvParameters) {
  for (;;) {
    if (humidite > 50.0) {
      digitalWrite(LED_RED, HIGH);    
      digitalWrite(LED_GREEN, LOW);    
    } else {
      digitalWrite(LED_RED, LOW);     
      digitalWrite(LED_GREEN, HIGH);  
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); 
  }
}
