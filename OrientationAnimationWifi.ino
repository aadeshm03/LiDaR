#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Wire.h>
#include "LIDARLite_v4LED.h"
#include "ICM_20948.h"


// Wi-Fi Credentials
const char* ssid = "Kiarash1";
const char* password = "12345678";

JSONVar readings;
const char htmlPage[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>ESP32 IMU & LiDAR</title>
      <script>
          // Store data globally for CSV download
          var latestData = {};

          // WebSocket data handler
          var socket = new WebSocket("ws://" + location.host + "/ws");
          socket.onmessage = function(event) {
              var data = JSON.parse(event.data);

              // Update the UI
              document.getElementById("quat_w").innerText = data.quat_w.toFixed(3);
              document.getElementById("quat_x").innerText = data.quat_x.toFixed(3);
              document.getElementById("quat_y").innerText = data.quat_y.toFixed(3);
              document.getElementById("quat_z").innerText = data.quat_z.toFixed(3);
              document.getElementById("raw_distance").innerText = data.RawLidarDistance;
              document.getElementById("reoriented_distance").innerText = data.ReorientedLidarDistance.toFixed(3);

              // Store the data for CSV download
              latestData = data;
          };

          // Function to download data as CSV
          function downloadCSV() {
              if (Object.keys(latestData).length === 0) {
                  alert("No data to download.");
                  return;
              }

              // Create a CSV string
              let csvContent = "data:text/csv;charset=utf-8,";
              const headers = "quat_w,quat_x,quat_y,quat_z,RawLidarDistance,ReorientedLidarDistance\n";
              const values = `${latestData.quat_w.toFixed(3)},${latestData.quat_x.toFixed(3)},${latestData.quat_y.toFixed(3)},${latestData.quat_z.toFixed(3)},${latestData.RawLidarDistance},${latestData.ReorientedLidarDistance.toFixed(3)}\n`;

              csvContent += headers + values;

              // Create a link element to trigger the download
              const encodedUri = encodeURI(csvContent);
              const link = document.createElement('a');
              link.setAttribute('href', encodedUri);
              link.setAttribute('download', 'sensor_data.csv');
              document.body.appendChild(link);

              // Trigger the download
              link.click();
          }
      </script>
  </head>
  <body>
      <h1>ESP32 Sensor Dashboard</h1>
      <p>Quaternion (IMU):</p>
      <ul>
          <li>w: <span id="quat_w">0</span></li>
          <li>x: <span id="quat_x">0</span></li>
          <li>y: <span id="quat_y">0</span></li>
          <li>z: <span id="quat_z">0</span></li>
      </ul>
      <p>LiDAR:</p>
      <ul>
          <li>Raw Distance: <span id="raw_distance">0</span> cm</li>
          <li>Reoriented Distance: <span id="reoriented_distance">0</span> cm</li>
      </ul>
      <button onclick="downloadCSV()">Download CSV</button>
  </body>
  </html>
)rawliteral";

  

// Create WebSocket server
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Create LIDAR and IMU objects
LIDARLite_v4LED myLidar;
ICM_20948_I2C myIMU;

#define SDA_PIN_IMU 27
#define SCL_PIN_IMU 14
#define SDA_PIN_LID 33
#define SCL_PIN_LID 26


float angle = 35.0;  // 35 degrees, halved as per 3Blue1Brown's explanation
float angle_rad = radians(angle);

float lidarRay[3] = {
        cos(angle_rad),   // X position (scaled by distance from IMU)
        0,                // Y position (no offset)
        sin(angle_rad),   // Z position (scaled by distance from IMU)
    };

void setup() {
    Serial.begin(115200);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected to Wi-Fi!");
    
    // Start I2C sensors
    Wire.begin(SDA_PIN_IMU, SCL_PIN_IMU);
    Wire1.begin(SDA_PIN_LID, SCL_PIN_LID);
    
    if (!myLidar.begin(0x62, Wire1)) {
      Serial.println("Failed to connect to LIDAR-Lite v4.");
    }

// Initialize IMU
    myIMU.begin(Wire, 0x68);
    if (myIMU.status != ICM_20948_Stat_Ok) {
      Serial.println("IMU initialization failed!");
    return;
    }

    
    myIMU.initializeDMP();
    myIMU.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION);
    myIMU.enableFIFO();
    myIMU.enableDMP();
    myIMU.resetDMP();
    myIMU.resetFIFO();
    
    // WebSocket event handler
    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) Serial.println("WebSocket client connected.");
    });

    server.addHandler(&ws);
    
    // Serve HTML file
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", htmlPage);
    });

    server.begin();
}

void loop() {
    icm_20948_DMP_data_t data;
    myIMU.readDMPdataFromFIFO(&data);
    
    if (myIMU.status == ICM_20948_Stat_Ok) {
        if ((data.header & DMP_header_bitmap_Quat9) > 0) {
            
            double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0;
            double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;
            double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0;
            double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));


            float lidar_position[3] = {0,0,0};
              double R[3][3] = {
                  {1 - 2 * (q2 * q2 + q3 * q3), 2 * (q1 * q2 - q3 * q0), 2 * (q1 * q3 + q2 * q0)},
                  {2 * (q1 * q2 + q3 * q0), 1 - 2 * (q1 * q1 + q3 * q3), 2 * (q2 * q3 - q1 * q0)},
                  {2 * (q1 * q3 - q2 * q0), 2 * (q2 * q3 + q1 * q0), 1 - 2 * (q1 * q1 + q2 * q2)}
              }; 

              for (int i = 0; i < 3; i++) { 
                  for (int j = 0; j < 3; j++) {
                      lidar_position[i] += R[i][j] * lidarRay[j];
                  }
              }

            uint16_t distance = myLidar.getDistance();
            float vertical_distance = lidar_position[2] * distance;  

            // Send data as JSON
            readings["quat_w"] = q0;
            readings["quat_x"] = q1;
            readings["quat_y"] = q2;
            readings["quat_z"] = q3;
            readings["RawLidarDistance"] = distance;
            readings["ReorientedLidarDistance"] = vertical_distance;

            //Serial.println(JSON.stringify(readings));

            

            ws.textAll(JSON.stringify(readings));
        }
    }
    
    delay(10);
}

// HTML Page for Web Dashboard
//Simplest
/*
  
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IMU & LiDAR Dashboard</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f4f4f9;
            margin: 0;
            padding: 20px;
        }
        h1 {
            color: #333;
        }
        .container {
            max-width: 500px;
            margin: auto;
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
        }
        .data-section {
            margin-top: 20px;
        }
        ul {
            list-style: none;
            padding: 0;
        }
        li {
            background: #ddd;
            margin: 5px 0;
            padding: 10px;
            border-radius: 5px;
        }
        span {
            font-weight: bold;
            color: #007bff;
        }
        #three-container {
            width: 100%;
            height: 300px;
            margin-top: 20px;
        }
        button {
            margin-top: 15px;
            padding: 10px;
            font-size: 16px;
            border: none;
            border-radius: 5px;
            background: #007bff;
            color: white;
            cursor: pointer;
        }
        button:hover {
            background: #0056b3;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Sensor Dashboard</h1>
        <div class="data-section">
            <h2>Quaternion (IMU)</h2>
            <ul>
                <li>w: <span id="quat_w">0</span></li>
                <li>x: <span id="quat_x">0</span></li>
                <li>y: <span id="quat_y">0</span></li>
                <li>z: <span id="quat_z">0</span></li>
            </ul>
        </div>
        <div class="data-section">
            <h2>LiDAR</h2>
            <ul>
                <li>Raw Distance: <span id="raw_distance">0</span> cm</li>
                <li>Reoriented Distance: <span id="reoriented_distance">0</span> cm</li>
            </ul>
        </div>
        <button onclick="downloadCSV()">Download CSV</button>
        <div id="three-container"></div>
    </div>
    
    <script>
    let socket = new WebSocket("ws://" + location.host + "/ws");
    let dataLog = [];

    let scene = new THREE.Scene();
    
    // Set background color to very light grey (RGB: 240, 240, 240)
    scene.background = new THREE.Color(0xf0f0f0);

    // Create camera with PerspectiveCamera
    let camera = new THREE.PerspectiveCamera(75, 500 / 300, 0.1, 1000);

    // Position the camera
    camera.position.set(0, 0, 2); // Move camera a bit back

    // Rotate the camera to match the coordinate system (Y-right, X-down, Z-towards observer)
    camera.rotation.x = Math.PI / 2;  // Rotate to make +X downward
    camera.rotation.y = -Math.PI / 2;  // Rotate to make +Y to the right

    let renderer = new THREE.WebGLRenderer();
    renderer.setSize(400, 300);
    document.getElementById("three-container").appendChild(renderer.domElement);

    // Cube geometry with pastel colors
    let geometry = new THREE.BoxGeometry();
    let materials = [
        new THREE.MeshBasicMaterial({ color: 0xff7f7f }),  // Pastel Red
        new THREE.MeshBasicMaterial({ color: 0x80ff80 }),  // Pastel Green
        new THREE.MeshBasicMaterial({ color: 0x7f7fff }),  // Pastel Blue
        new THREE.MeshBasicMaterial({ color: 0xffff7f }),  // Pastel Yellow
        new THREE.MeshBasicMaterial({ color: 0xff7fff }),  // Pastel Magenta
        new THREE.MeshBasicMaterial({ color: 0x7fffd4 })   // Pastel Cyan
    ];

    let cube = new THREE.Mesh(geometry, materials);
    scene.add(cube);

    // Animation loop
    function animate() {
        requestAnimationFrame(animate);
        renderer.render(scene, camera);
        }
        animate();

        socket.onmessage = function(event) {
            let data = JSON.parse(event.data);
            document.getEleme ntById("quat_w").innerText = data.quat_w.toFixed(3);
            document.getElementById("quat_x").innerText = data.quat_x.toFixed(3);
            document.getElementById("quat_y").innerText = data.quat_y.toFixed(3);
            document.getElementById("quat_z").innerText = data.quat_z.toFixed(3);
            document.getElementById("raw_distance").innerText = data.RawLidarDistance;
            document.getElementById("reoriented_distance").innerText = data.ReorientedLidarDistance.toFixed(3);
            
            dataLog.push([Date.now(), data.quat_w, data.quat_x, data.quat_y, data.quat_z, data.RawLidarDistance, data.ReorientedLidarDistance]);
            
            let qx = data.quat_x;
            let qy = data.quat_y;
            let qz = data.quat_z;
            let qw = data.quat_w;
            cube.quaternion.set(qx, qy, qz, qw);
        };

        function downloadCSV() {
            let csvContent = "data:text/csv;charset=utf-8,Time,Quat_W,Quat_X,Quat_Y,Quat_Z,RawLidar,ReorientedLidar\n";
            dataLog.forEach(row => {
                csvContent += row.join(",") + "\n";
            });
            let encodedUri = encodeURI(csvContent);
            let link = document.createElement("a");
            link.setAttribute("href", encodedUri);
            link.setAttribute("download", "sensor_data.csv");
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }
    </script>
</body>
</html>
)rawliteral";

*/

