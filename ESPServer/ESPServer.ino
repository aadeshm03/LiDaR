#include <WiFi.h>

#define PORT 8080
#define BUFFER_SIZE 8192  // Increased buffer size
#define DATA_BUFFER_SIZE 2048  // Size of the count buffer

const char* ssid = "Brady";       // Replace with your WiFi SSID
const char* password = "password"; // Replace with your WiFi password

WiFiServer server(PORT);
int count = 0;  // Counter to keep track of occurrences
int data_buffer1[DATA_BUFFER_SIZE];  // Buffer to store count values
int data_buffer2[DATA_BUFFER_SIZE];  // Buffer to store count values
int data_index = 0;
int last_sent_index = 0;  // Track the last sent index to avoid duplicates

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
        // } else {
            // Handle wrap-around scenario when count_index resets
            String dataMessage = "{";
            if (last_sent_index <= data_index) {
                // Normal case: send counts from last_sent_index to count_index
                for (int i = last_sent_index; i < data_index; i++) {
                    dataMessage += "data1: " + String(data_buffer1[i]) + ",";
                    dataMessage += "data2: " + String(data_buffer2[i]) + ",";
                }
            } else {
                // Wrap-around case: send counts from last_sent_index to end, then from start to count_index
                for (int i = last_sent_index; i < DATA_BUFFER_SIZE; i++) {
                    dataMessage += "data1: " + String(data_buffer1[i]) + ",";
                    dataMessage += "data2: " + String(data_buffer2[i]) + ",";
                }
                for (int i = 0; i < data_index; i++) {
                    dataMessage += "data1: " + String(data_buffer1[i]) + ",";
                    dataMessage += "data2: " + String(data_buffer2[i]) + ",";
                }
            }
            dataMessage.remove(dataMessage.length() - 1);  // Remove the last comma
            dataMessage += "}\n";
            client.write((const uint8_t *)dataMessage.c_str(), dataMessage.length());
            last_sent_index = data_index;  // Update the last sent index
        }
    }

    client.stop();
    vTaskDelete(NULL);  // Delete the task when done
}

void setup() {
    Serial.begin(115200);
    delay(1000);

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
    WiFiClient client = server.available();
    if (client) {
        Serial.println("New client connected.");
        WiFiClient *newClient = new WiFiClient(client);  // Allocate memory for the client
        xTaskCreate(handle_client, "ClientHandler", BUFFER_SIZE*2, (void *)newClient, 2, NULL);  // Create FreeRTOS task with higher priority
    }

    // Count occurrences and store in the buffer if different from the last sent count
    count++;
    data_buffer1[count_index++] = count;
    data_buffer2[count_index++] = count >> 2 ^ 0xA192;

    // Reset the buffer index if it reaches the buffer size
    if (count_index >= COUNT_BUFFER_SIZE) {
        count_index = 0;
    }
    delay(25);  // Short delay for loop efficiency
}
