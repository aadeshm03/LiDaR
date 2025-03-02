#include <WiFi.h>

// ULTRASONIC SENSOR JUNK
#include <HCSR04.h>
byte triggerPin = 19;
byte echoCount = 1;
byte echoPin = 18;

#define PORT 8080
#define DATA_BUFFER_SIZE 512  // Size of the count buffer
#define BUFFER_SIZE DATA_BUFFER_SIZE * 16  // Increased buffer size

const char* ssid = "Aadesh ";       // Replace with your WiFi SSID
const char* password = "aadesh120"; // Replace with your WiFi password

WiFiServer server(PORT);
int count = 0;  // Counter to keep track of occurrences
int dist_buffer[DATA_BUFFER_SIZE];  // Buffer to store count values
int data_buffer2[DATA_BUFFER_SIZE];  // Buffer to store count values
int data_index = 0;
int last_sent_index = 0;  // Track the last sent index to avoid duplicates

bool ClientConnected = false;

void handle_client(void *param) {
    WiFiClient client = *((WiFiClient *)param);
    delete (WiFiClient *)param;  // Free the memory allocated for the parameter

    // THIS COMMENTED STUFF IS FOR RECIEVEING DATA
    // char buffer[BUFFER_SIZE];

    while (client.connected()) {
        // memset(buffer, 0, BUFFER_SIZE);  // Use C-style memset
        // int bytes_received = client.readBytes(buffer, BUFFER_SIZE);
        // if (bytes_received > 0) {
        //     String message = String(buffer);  // Use Arduino String class
        //     if (message == "/quit\n") {
        //         Serial.println("Client disconnected.");
        //         break;
        //     }
        //     Serial.print("Message received: ");
        //     Serial.println(message);
        //     client.write(buffer, bytes_received);
        // }

      // Handle wrap-around scenario when data_index resets
      int dataMessage[2*DATA_BUFFER_SIZE];
      int messageIndex = 0;
      if (last_sent_index < data_index) {
          // Normal case: send counts from last_sent_index to data_index
          for (int i = last_sent_index; i < data_index; i++) {
              // Bitwise junk to send data as raw bytes instead of human readable strings
              // Will need to be decoded manually client-side
              dataMessage[messageIndex++] = dist_buffer[i];
              dataMessage[messageIndex++] = data_buffer2[i];
          }
      } else if (last_sent_index > data_index) {
          // Wrap-around case: send counts from last_sent_index to end, then from start to data_index
          for (int i = last_sent_index; i < DATA_BUFFER_SIZE; i++) {
              dataMessage[messageIndex++] = dist_buffer[i];
              dataMessage[messageIndex++] = data_buffer2[i];
          }
          for (int i = 0; i < data_index; i++) {
              dataMessage[messageIndex++] = dist_buffer[i];
              dataMessage[messageIndex++] = data_buffer2[i];
          }
      } else continue;

      client.write((const uint8_t*) dataMessage, sizeof(int)*messageIndex);
      last_sent_index = data_index;  // Update the last sent index
  }

    client.stop();
    ClientConnected = false;
    vTaskDelete(NULL);  // Delete the task when done
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    HCSR04.begin(triggerPin, echoPin); // ULTRASONIC

    // Connect to Wi-Fi
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
    
    // Print the IP address
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Start the server
    server.begin();
    Serial.printf("Server is listening on port %d\n", PORT);
}

void loop() {
    // CONNECT CLIENTS
    if (!ClientConnected) {
        WiFiClient client = server.available();
        if (client) {
            ClientConnected = true;
            Serial.println("New client connected.");
            WiFiClient *newClient = new WiFiClient(client);  // Allocate memory for the client
            xTaskCreate(handle_client, "ClientHandler", BUFFER_SIZE, (void *)newClient, 1, NULL);  // Create FreeRTOS task with higher priority
        }
    }
    
    double* distance = HCSR04.measureDistanceCm();

    dist_buffer[data_index] = (int) distance[0]; // ULTRASONIC DISTANCE CM 
    data_buffer2[data_index++] = (int) (millis()); // RANDOM DATA

    // Reset the buffer index if it reaches the buffer size
    if (data_index >= DATA_BUFFER_SIZE) {
        data_index = 0;
    }
    
    delay(10);  // Short delay for loop efficiency
}
