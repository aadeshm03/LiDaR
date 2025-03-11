#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Wire.h>
#include "LIDARLite_v4LED.h"
#include "ICM_20948.h"


// Wi-Fi Credentials
const char* ssid = "Basement";
const char* password = "GnastyGnorc";

JSONVar readings;

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 IMU & LiDAR Dashboard</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>  <!-- Add Chart.js -->
    <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-annotation@1.0.2"></script>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f4f4f9;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 600px;
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
        #cube-container {
            width: 300px;
            height: 300px;
            margin: auto;
        }
        canvas {
            width: 100% !important;
            max-height: 300px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 Sensor Dashboard</h1>
        <!-- <div class="data-section">
            <h2>Quaternion (IMU)</h2>
            <ul>
                <li>w: <span id="quat_w">0</span></li>
                <li>x: <span id="quat_x">0</span></li>
                <li>y: <span id="quat_y">0</span></li>
                <li>z: <span id="quat_z">0</span></li>
            </ul>
        </div> -->
        <div class="data-section">
            <h2>LiDAR</h2>
            <ul>
                <li>Raw Distance: <span id="raw_distance">0</span> cm</li>
                <li>Reoriented Distance: <span id="reoriented_distance">0</span> cm</li>
            </ul>
        </div>
        
        <h2>LiDAR Distance Over Time</h2>
        <canvas id="lidarChart"></canvas> <!-- Chart for LiDAR distance -->

        <h2>3D Orientation</h2>
        <div id="cube-container"></div>
    </div>

    <script>
        let socket;
        let imuData = [];
        let lidarChart;

        document.addEventListener("DOMContentLoaded", () => {
            socket = new WebSocket("ws://" + location.host + "/ws");

            // Create the chart
            let ctx = document.getElementById("lidarChart").getContext("2d");

            lidarChart = new Chart(ctx, {
                type: "line",
                data: {
                    labels: [],  // Time labels
                    datasets: [{
                        label: "LiDAR Distance (cm)",
                        data: [],
                        borderColor: "blue",
                        borderWidth: 2,
                        fill: false,
                        pointRadius: 1
                    }]
                },
                options: {
                    scales: {
                        // x: { title: { display: true, text: "Time (s)" } },
                        y: {
                            title: { display: true, text: "Distance (cm)" },
                            min: 0,
                            max: 200,
                            stepSize: 20,
                        }
                    },
                    animation: false,
                    plugins: {
                        annotation: {
                            annotations: {
                                line1: {
                                    type: 'line',
                                    yMin: 70,
                                    yMax: 70,
                                    borderColor: 'red',
                                    borderWidth: 2,
                                    borderDash: [10, 5],
                                    label: {
                                        content: 'Threshold',
                                        enabled: true,
                                        position: 'end'
                                    }
                                }
                            }
                        }
                    }
                }
            });


            socket.onmessage = function (event) {
                console.log("Received WebSocket data:", event.data);
                var data = JSON.parse(event.data);

                let currentTime = (Date.now() / 1000).toFixed(1); // Convert to seconds
                let distance = parseFloat(data.ReorientedLidarDistance);

                // document.getElementById("quat_w").innerText = data.quat_w.toFixed(3);
                // document.getElementById("quat_x").innerText = data.quat_x.toFixed(3);
                // document.getElementById("quat_y").innerText = data.quat_y.toFixed(3);
                // document.getElementById("quat_z").innerText = data.quat_z.toFixed(3);
                document.getElementById("raw_distance").innerText = distance.toFixed(1);;
                document.getElementById("reoriented_distance").innerText = data.ReorientedLidarDistance.toFixed(1);

                // Add new data point to the chart
                if (lidarChart.data.labels.length > 50) {
                    lidarChart.data.labels.shift();
                    lidarChart.data.datasets[0].data.shift();
                }
                lidarChart.data.labels.push("");
                lidarChart.data.datasets[0].data.push(data.ReorientedLidarDistance.toFixed(1));
                lidarChart.update();

                updateCubeOrientation(data.quat_w, data.quat_x, data.quat_y, data.quat_z);
            };

            socket.onclose = function () {
                console.warn("WebSocket disconnected. Reconnecting...");
                setTimeout(() => location.reload(), 1000);
            };
        });

        // 3D Cube with Three.js
        const scene = new THREE.Scene();
        scene.background = new THREE.Color(0xffffff);
        const camera = new THREE.PerspectiveCamera(75, 1, 0.1, 1000);
        const renderer = new THREE.WebGLRenderer();
        renderer.setSize(300, 300);
        document.getElementById("cube-container").appendChild(renderer.domElement);

        const geometry = new THREE.BoxGeometry(1, 1, 1);
        const materials = [
            new THREE.MeshBasicMaterial({ color: 0xff0000 }),
            new THREE.MeshBasicMaterial({ color: 0x00ff00 }),
            new THREE.MeshBasicMaterial({ color: 0x0000ff }),
            new THREE.MeshBasicMaterial({ color: 0xffff00 }),
            new THREE.MeshBasicMaterial({ color: 0xff00ff }),
            new THREE.MeshBasicMaterial({ color: 0x00ffff })
        ];
        const cube = new THREE.Mesh(geometry, materials);
        scene.add(cube);
        
        camera.position.set(0, -2, 0);

        // Make the camera look at the origin (0, 0, 0)
        camera.lookAt(new THREE.Vector3(0, 0, 0));

        function updateCubeOrientation(w, x, y, z) {
            const quat = new THREE.Quaternion(x, y, z, w);
            cube.quaternion.copy(quat);
            renderer.render(scene, camera);
        }

        function animate() {
            requestAnimationFrame(animate);
            renderer.render(scene, camera);
        }
        animate();
    </script>
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
#define SPEAKER 15


float angle = 35.0;  // 35 degrees, halved as per 3Blue1Brown's explanation
float angle_rad = radians(angle);
float vertical_distance = 0;

float lidarRay[3] = {
        cos(angle_rad),   // X position (scaled by distance from IMU)
        0,                // Y position (no offset)
        -sin(angle_rad),   // Z position (scaled by distance from IMU)
    };


void setup() {
    Serial.begin(115200);

    pinMode(SPEAKER, OUTPUT);

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
    long sendDelay = 0;

    if (myIMU.status == ICM_20948_Stat_Ok) {
        //if ((data.header & DMP_header_bitmap_Quat9) > 0) {
            
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
            vertical_distance = lidar_position[2] * distance;
            
            // Serial.print(lidar_position[0]);
            // Serial.print(", ");
            // Serial.print(lidar_position[1]);
            // Serial.print(", ");
            // Serial.print(lidar_position[2]);
            // Serial.print(", ");
            // Serial.print(distance);
            // Serial.print(", ");
            // Serial.println(vertical_distance);

            // Send data as JSON
            readings["quat_w"] = q0;
            readings["quat_x"] = q1;
            readings["quat_y"] = q2;
            readings["quat_z"] = q3;
            readings["RawLidarDistance"] = distance;
            readings["ReorientedLidarDistance"] = vertical_distance;

            // Serial.println(JSON.stringify(readings));
            sendDelay = millis();
            ws.textAll(JSON.stringify(readings));
            sendDelay = millis() - sendDelay;
        //}
    }
    
    
    if (vertical_distance < 70) {
      tone(SPEAKER, 440, 10-sendDelay);
    // } else if (vertical_distance > 170) {
    //   tone(SPEAKER, 100, 10);
    } else {
      delay(10-sendDelay);
    }
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
      <title>ESP32 IMU & LiDAR</title>
      <script>
          var socket = new WebSocket("ws://" + location.host + "/ws");
          socket.onmessage = function(event) {
              var data = JSON.parse(event.data);
              document.getElementById("quat_w").innerText = data.quat_w.toFixed(3);
              document.getElementById("quat_x").innerText = data.quat_x.toFixed(3);
              document.getElementById("quat_y").innerText = data.quat_y.toFixed(3);
              document.getElementById("quat_z").innerText = data.quat_z.toFixed(3);
              document.getElementById("raw_distance").innerText = data.RawLidarDistance;
              document.getElementById("reoriented_distance").innerText = data.ReorientedLidarDistance.toFixed(3);
          };
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
  </body>
  </html>
  )rawliteral";
*/

