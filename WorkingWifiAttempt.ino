
#include <WebSocketServer.h>


#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>
#include "LIDARLite_v4LED.h"


#define SDA_PIN_IMU 27  // Change to match your ESP32 setup
#define SCL_PIN_IMU 14
#define SDA_PIN_LID 33  // Change to match your ESP32 setup
#define SCL_PIN_LID 26

//WiFiServer server(80);

#define SERIAL_PORT Serial

// Collecting LIDAR sensor data
Ticker timer;
LIDARLite_v4LED myLidar;
bool get_data = false;

// Connecting to the Internet
const char* ssid = "KiarashiPhone";
const char* password = "123456789";

// Running a web server
WebServer server(80);

// Adding a websocket to the server
WebSocketsServer webSocket = WebSocketsServer(81);

// Serving a web page (from flash memory)
char webpage[] PROGMEM = R"=====( 
<html>
  <head>
    <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.5.0/Chart.min.js'></script>
  </head>
  <body onload="javascript:init()">
    <div>
      <input type="range" min="1" max="10" value="5" id="dataRateSlider" oninput="sendDataRate()" />
      <label for="dataRateSlider" id="dataRateLabel">Rate: 0.2Hz</label>
    </div>
    <hr />
    <div>
      <canvas id="line-chart" width="800" height="450"></canvas>
    </div>
    <script>
      var webSocket, dataPlot;
      var maxDataPoints = 20;
      function removeData() {
        dataPlot.data.labels.shift();
        dataPlot.data.datasets[0].data.shift();
      }
      function addData(label, data) {
        if(dataPlot.data.labels.length > maxDataPoints) removeData();
        dataPlot.data.labels.push(label);
        dataPlot.data.datasets[0].data.push(data);
        dataPlot.update();
      }
      function init() {
        webSocket = new WebSocket('ws://' + window.location.hostname + ':81/');
        dataPlot = new Chart(document.getElementById("line-chart"), {
          type: 'line',
          data: {
            labels: [],
            datasets: [{
              data: [],
              label: "LiDAR Distance (cm)",
              borderColor: "#3e95cd",
              fill: false
            }]
          }
        });
        webSocket.onmessage = function(event) {
          var data = JSON.parse(event.data);
          var today = new Date();
          var t = today.getHours() + ":" + today.getMinutes() + ":" + today.getSeconds();
          addData(t, data.value);
        }
      }
      function sendDataRate() {
        var dataRate = document.getElementById("dataRateSlider").value;
        webSocket.send(dataRate);
        dataRate = 1.0/dataRate;
        document.getElementById("dataRateLabel").innerHTML = "Rate: " + dataRate.toFixed(2) + "Hz";
      }
    </script>
  </body>
</html>
)=====";

void setup() {
  // put your setup code here, to run once:
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", [](){
    server.send_P(200, "text/html", webpage);
  });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  // Initialize LiDAR
  if (!myLidar.begin(0x62, Wire)) {
    Serial.println("LiDAR not found! Check wiring.");
    while (1); // Halt the program if LiDAR fails
  }
  
  timer.attach(5, getData);
}

void loop() {
  // put your main code here, to run repeatedly:
  webSocket.loop();
  server.handleClient();
  if(get_data){
    uint16_t distance = myLidar.getDistance();
    String json = "{\"value\":";
    json += distance;
    json += "}";
    webSocket.broadcastTXT(json.c_str(), json.length());
    get_data = false;
  }
}

void getData() {
  get_data = true;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  // Handle data from the client if necessary (e.g., change data rate)
  if (type == WStype_TEXT) {
    float dataRate = (float) atof((const char *) &payload[0]);
    timer.detach();
    timer.attach(dataRate, getData);
  }
}
