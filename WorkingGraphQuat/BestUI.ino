#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <Wire.h>
#include "LIDARLite_v4LED.h"
#include "ICM_20948.h"


// Wi-Fi Credentials
//const char* ssid = "Aadesh ";
//const char* password = "aadesh120";


const char *ssid = "ubcvisitor";  // Example: "eduroam"
//const char* identity = "your_email@university.edu";  // University login
const char *password = "";

JSONVar readings;

const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title> Athletic Tracker </title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>  <!-- Add Chart.js -->
    <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-annotation@1.0.2"></script>
    <style>
        body {
            font-family: 'Arial', sans-serif;
            text-align: center;
            background-color: #B5E2FA;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 800px;
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
            width: 50%;
            margin: 0 auto;
        }
        li {
          background: #F9F7F3;
          margin: 2px 0;  /* Reduce margin for shorter spacing */
          padding: 5px;   /* Reduce padding for a more compact look */
          border-radius: 5px;
          font: bold 16px Arial;  /* Smaller font size */
          text-align: center;
          
        }

        span {
            font-weight: bold;
            font-size: 20px;
            color: #0FA3B1; 
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
        <h1>Athletic Tracker</h1>
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
            <h2>Torso Height</h2>
            <ul>
                <li>Raw Distance: <span id="raw_distance">0</span> cm</li>
                <li>Torso Height: <span id="reoriented_distance">0</span> cm</li>
            </ul>
        </div>
        
        <h2>Torso Height Over Time</h2>
        <canvas id="lidarChart"></canvas> <!-- Chart for LiDAR distance -->

        <h2>Torso pose</h2>
        <div id="cube-container"></div>
        <h2> Percentage of time in Range </h2>
        <canvas id="percentRange"> </canvas>
        <div class="data-section">
          <ul>
              <li>Percent in Range: <span id="percentDisplay">0% </span> </li>
              <li id="percentMessage">Waiting for data...</li>
          </ul>
        </div>
        <button id="downloadCSV" style="margin-top: 20px; padding: 15px; background-color: #0FA3B1; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 20px; font-weight: bold;">
            Download CSV
        </button>

    </div>


<script>
    let socket;
    let imuData = [];
    let lidarChart, percentChart;
    let inRangeCount = 0;
    let tooHigh = 0;
    let totalCount = 0;
    let lowerBound = 40;
    let upperBound = 80; 
    let collectedData = []; // Store received data for CSV
    let messageTimer;
    let messages = [
        "AMAZING. KEEP IT GOING!",
        "WOW. SO GOOD.",
        "great start. Don't be afraid to take your time!"
        ];

    document.addEventListener("DOMContentLoaded", () => {
        socket = new WebSocket("ws://" + location.host + "/ws");

        // Create the chart
        let ctx = document.getElementById("lidarChart").getContext("2d");
        let percentCtx = document.getElementById("percentRange").getContext("2d");

        lidarChart = new Chart(ctx, {
          type: "line",
          data: {
              labels: [],  // Time labels
              datasets: [{
                  label: "Torso Height (cm)",
                  data: [],
                  borderColor: "#0FA3B1",
                  backgroundColor: "#0FA3B1",
                  borderWidth: 3,
                  fill: false,
                  pointRadius: 1
              }]
          },
          options: {
              responsive: true,
              animation: false,  // Disable animation
              plugins: {
                  





                  legend: {
                      position: 'top', // You can change 'top', 'left', 'bottom', or 'right'
                      labels: {
                          font: {
                              size: 25, // Adjust the font size of the legend
                              family: 'Arial', // Set the font family
                              style: 'italic bold', // Set the font style (italic, normal, bold)
                          },
                      },
                  },
              },
                scales: {
                  y: {
                      title: {
                          display: true,
                          text: "Distance (cm)", // Y-axis title text
                          font: {
                              size: 25, // Increase Y-axis title font size
                              family: 'Arial', // Y-axis title font family
                              weight: 'bold', // Y-axis title font weight
                          },
                          color: '#333', // Y-axis title text color
                      },
                      ticks: {
                          font: {
                              size: 18, // Increase tick mark font size
                              family: 'Arial',
                          },
                          color: '#000', // Change tick color for better visibility
                      },
                      min: 0,
                      max: 200,
                      stepSize: 20,
                  },
              }
          },
      });


        percentChart = new Chart(percentCtx, {
            type: "doughnut",
            data: {
                labels: ["In Range", "Too Low", "Too High"],
                datasets: [{
                    data: [0, 0, 0], // Initial values
                    backgroundColor: ["#8fd19e", "#EDDEA4", "#F7A072"]
                }]
            },
            options: {
                responsive: true,
                plugins: {
                    legend: { 
                        display: true,
                        labels: {
                            font: {
                                size: 24 // Change legend font size
                            },
                            color: "#000" // Change legend text color if needed
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
           

            document.getElementById("raw_distance").innerText = data.RawLidarDistance.toFixed(1);
            document.getElementById("reoriented_distance").innerText = data.ReorientedLidarDistance.toFixed(1);

            // Store data for CSV
            collectedData.push([currentTime, data.RawLidarDistance, data.ReorientedLidarDistance]);

            // Update the chart
            if (lidarChart.data.labels.length > 50) {
                lidarChart.data.labels.shift();
                lidarChart.data.datasets[0].data.shift();
            }
            lidarChart.data.datasets[0].data.push(data.ReorientedLidarDistance.toFixed(1));
            lidarChart.update();

            updateCubeOrientation(data.quat_w, data.quat_x, data.quat_y, data.quat_z);

            totalCount++;
            if (distance < upperBound && distance > lowerBound) {
                inRangeCount++;
            }
            else if (distance > upperBound) {
              tooHigh++;
            }


            let inRangePercent = totalCount > 0 ? ((inRangeCount / totalCount) * 100).toFixed(1) : 0;
            let tooHighRangePercent = totalCount > 0 ? ((tooHigh / totalCount) * 100).toFixed(1) : 0;
            let tooLowRangePercent = totalCount > 0 ? (100 - (parseFloat(inRangePercent) + parseFloat(tooHighRangePercent))) : 0;

            percentChart.data.datasets[0].data = [inRangePercent, tooHighRangePercent, tooLowRangePercent];
            percentChart.update();
            updateMessage(inRangePercent);
            document.getElementById("percentDisplay").innerText = `${inRangePercent}%`
            };

        socket.onclose = function () {
            console.warn("WebSocket disconnected. Reconnecting...");
            setTimeout(() => location.reload(), 1000);
        };

        document.getElementById("downloadCSV").addEventListener("click", function () {
            let csvContent = "data:text/csv;charset=utf-8,Time (s),Raw Distance (cm),Reoriented Distance (cm)\n";

            collectedData.forEach(row => {
                csvContent += row.join(",") + "\n";
            });

            let encodedUri = encodeURI(csvContent);
            let link = document.createElement("a");
            link.setAttribute("href", encodedUri);
            link.setAttribute("download", "lidar_imu_data.csv");
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        });
    });

    function updateMessage(percent) {
      clearTimeout(messageTimer);

      let message = "";
      if (percent >= 80) {
          message = messages[0]; // "AMAZING. KEEP IT GOING!"
      } else if (percent <= 50 && percent < 80) {
          message = messages[1]; // "WOW. SO GOOD."
      } else {
          message = messages[2]; // "great start. Don't be afraid to take your time!"
      }

      document.getElementById("percentMessage").innerText = message;

      messageTimer = setTimeout(() => {
          updateMessage(percent); // Recursively update the message every 2 seconds
      }, 2000);
    }

    // 3D Cube with Three.js
    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xffffff);
    const camera = new THREE.PerspectiveCamera(75, 1, 0.1, 1000);
    const renderer = new THREE.WebGLRenderer();
    renderer.setSize(300, 300);
    document.getElementById("cube-container").appendChild(renderer.domElement);

    const geometry = new THREE.BoxGeometry(1, 1, 1);
    const materials = [
        new THREE.MeshBasicMaterial({ color: 0x0FA3B1   }),
        new THREE.MeshBasicMaterial({ color: 0xB5E2FA   }),
        new THREE.MeshBasicMaterial({ color: 0xF76D3F   }),
        new THREE.MeshBasicMaterial({ color: 0xF8A97F   }),
        new THREE.MeshBasicMaterial({ color: 0xF4C7AB   }),
        new THREE.MeshBasicMaterial({ color: 0xA3D9A5   })
    ];
    const cube = new THREE.Mesh(geometry, materials);
    scene.add(cube);

    camera.position.set(0, -2, 0);
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


float angle = 20.0;  // 35 degrees, halved as per 3Blue1Brown's explanation
float angle_rad = radians(angle);
float vertical_distance = 0;

float lidarRay[3] = {
  cos(angle_rad),   // X position (scaled by distance from IMU)
  0,                // Y position (no offset)
  -sin(angle_rad),  // Z position (scaled by distance from IMU)
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
  Serial.println(WiFi.localIP());
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
  int lowerBOUND = 50;
  int upperBOUND = 300;
  icm_20948_DMP_data_t data;
  myIMU.readDMPdataFromFIFO(&data);
  long sendDelay = 0;
  int inRangeCounter = 0;
  int outRangeCounter = 0;

  if (myIMU.status == ICM_20948_Stat_Ok) {
    //if ((data.header & DMP_header_bitmap_Quat9) > 0) {

    double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0;
    double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;
    double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0;
    double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));


    float lidar_position[3] = { 0, 0, 0 };
    double R[3][3] = {
      { 1 - 2 * (q2 * q2 + q3 * q3), 2 * (q1 * q2 - q3 * q0), 2 * (q1 * q3 + q2 * q0) },
      { 2 * (q1 * q2 + q3 * q0), 1 - 2 * (q1 * q1 + q3 * q3), 2 * (q2 * q3 - q1 * q0) },
      { 2 * (q1 * q3 - q2 * q0), 2 * (q2 * q3 + q1 * q0), 1 - 2 * (q1 * q1 + q2 * q2) }
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




    long currentTime = millis();
    // Serial.println(JSON.stringify(readings));
    //Serial.print("{ ");
    //Serial.print(q0, 6); Serial.print(", ");
    //Serial.print(q1, 6); Serial.print(", ");
    //Serial.print(q2, 6); Serial.print(", ");
    //Serial.print(q3, 6); Serial.print(", ");
    //Serial.print(vertical_distance, 2); Serial.print(", ");
    //Serial.print(currentTime);
    //Serial.println(" }");
    ws.textAll(JSON.stringify(readings));

    //}
  }


  if (vertical_distance < 70) {
    tone(SPEAKER, 440, 10 - sendDelay);
    // } else if (vertical_distance > 170) {
    //   tone(SPEAKER, 100, 10);
  } else {
    //delay(10-sendDelay);
  }
}
