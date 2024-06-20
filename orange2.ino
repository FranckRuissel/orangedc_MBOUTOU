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
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDY3MLGpYgjDSkW\n" \
"uHbEFs17qMumnIgKlCNuYnwW+q7Dcj9JkDkFn165+VoMAz/A6wBpec1Y7+fCgHA0\n" \
"ykwrC6BMN77bK8dTSd2vZ7URXZSjj7oU8U9GvYV29e8EF/DBPfw7T46zn3UU/5NR\n" \
"Qh1XqZBiPSFq/E59AcB5yvoAsIHFEzVA7FzHgu2/qXoKiFR2JoSG60myQJsIZuxc\n" \
"aeAkZAi5JaLcsbFTO35pj1SRRQO6moAYPGqxIeUXVGnJGlm60EkIKa2fFbz1zqtG\n" \
"njG6JllPzsCR05bOV8IJBwVAxiOR4uQ0K48jyDolfqAhN+4Epm33hW+jcOZ19kG6\n" \
"uAwVg7SrAgMBAAECggEADgrvSaMFJULE1Uy9nKYDW5/LDC9rHBBBhs+1p8prtIpo\n" \
"J3CvKt73sdcbhWl5Tixk8cr8kguNOFNOHnvbWKlfbQAO0uQCMnq3Ft3drRrXIvmy\n" \
"aYjnAsi+QGHdh98dSCmTvhjl0hDW8pATnR42qzRSNPSckN3ksTreKxKKqEDqUuMr\n" \
"XwnBhzLFt3xHNGRoW5x1O8537/w/INE5zwxo4Uvpoj2Sv4yaZ4zUV3uC6690uQlO\n" \
"L9A2W6NaMDwa48egSmmC4QoWlHo8EXsIk3y38gtVqp18Fj3x4we5cmMbCze/OGgR\n" \
"glR308pm+kHkkt9uVLc0YkXuVmQtxGqjPUaYH9lZWQKBgQDsn3YDYCn0ZnI3HFnV\n" \
"XiZfaQhPfGPlnZkggAlTzdsgyhuJZu8cBYjCYriRtREgFh7kcegLT40osRbWnFlE\n" \
"SQCS8Msivp5BFd9aWkf55q2ziSHCU/nPndgs8o0PDEN4mqcGLX02+00GIG947Qba\n" \
"KE7I+GMyUQ1UDMJkwcdyuzZNkwKBgQDqnwqsDuqukyyTuBlhjj7odEH3xj5zpLx2\n" \
"hEdawcSb4eb8z9c1QUQbksA6QjeYlm4ZbPjqh9kO9s0PTCA0ktDjJCoHKkVSd4e+\n" \
"8Gdd70WqchAcFFZsrPBknHOlFcD1u1Y1OKJzfrMptS50OJCb7cLCl0fhCIWX6oq6\n" \
"bNpVL7GriQKBgQCs7f05ilYfH5/pVvbwUNaKdCB2ttnrPbpjXi9y58d1tz9Ys7mc\n" \
"KQ9BwRRSqJnTcVcybWzAl2r6ijG+tpUmqpOY0BIlWloeBvYs+j4QDN51j4r0kprH\n" \
"ePw8HSeqD7Rft+bJRQV14ULxb1MIdXHEjiDGD4GvjDzJaGnJaTVyTA/rsQKBgFHj\n" \
"dLwc8LhB0fs09FbiaP6FJUkvvjhqtQTAoo8AtniZ3h87A6gowXbGP6WV2wW862yg\n" \
"ZjildZMNfQDr1wnCfHnq82hQzqU75vszuUBF9RDKgujqCEgiDD1xhcOG1jRuUGZ+\n" \
"/+RDwpC6Fh6mZ4Kk96cnWgUnDkm5qPXsbD/zR5XRAoGBAOw1W7diitZp1s0e2eKI\n" \
"UQNQkxZNvZEebt1jOMx4ABwoGK0ghql22fq/FZOnNmDpOjn6S/5bUfNzvmeXBfpS\n" \
"IJnXiKnAt9mHyYfvBQ5zmGLK0sNQDGFGeEAbBWbHlbZs+MmGq2CKxX88YAk1X8XN\n" \
"xGrUCkxR23kSHiK1R2drEf08\n" \
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
