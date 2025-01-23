/****************************************************************
* Example6_DMP_Quat9_Orientation.ino
* ICM 20948 Arduino Library Demo
* Initialize the DMP based on the TDK InvenSense ICM20948_eMD_nucleo_1.0 example-icm20948
* Paul Clark, April 25th, 2021
* Based on original code by:
* Owen Lyke @ SparkFun Electronics
* Original Creation Date: April 17 2019
* 
* ** This example is based on InvenSense's _confidential_ Application Note "Programming Sequence for DMP Hardware Functions".
* ** We are grateful to InvenSense for sharing this with us.
* 
* ** Important note: by default the DMP functionality is disabled in the library
* ** as the DMP firmware takes up 14301 Bytes of program memory.
* ** To use the DMP, you will need to:
* ** Edit ICM_20948_C.h
* ** Uncomment line 29: #define ICM_20948_USE_DMP
* ** Save changes
* ** If you are using Windows, you can find ICM_20948_C.h in:
* ** Documents\Arduino\libraries\SparkFun_ICM-20948_ArduinoLibrary\src\util
*
* Please see License.md for the license information.
*
* Distributed as-is; no warranty is given.
***************************************************************/

#include "ICM_20948.h" // Click here to get the library: http://librarymanager/All#SparkFun_ICM_20948_IMU
//#include <TFminiS.h>
#include <TFMini.h>

#define TFMINI Serial1



//#define USE_SPI       // Uncomment this to use SPI

#define SERIAL_PORT Serial
#define CS_PIN 2     // Which pin you connect CS to. Used only when "USE_SPI" is defined

#define WIRE_PORT Wire // Your desired Wire port.      Used when "USE_SPI" is not defined
// The value of the last bit of the I2C address.
// On the SparkFun 9DoF IMU breakout the default is 1, and when the ADR jumper is closed the value becomes 0
#define AD0_VAL 1

ICM_20948_I2C myICM; // Otherwise create an ICM_20948_I2C object
TFMini tfmini;

// Create the TFMiniS object for the LiDAR

void setup() {
  
  SERIAL_PORT.begin(115200); // Start the serial console
  delay(100);

  // Initialize ICM-20948 IMU


  WIRE_PORT.begin();
  WIRE_PORT.setClock(400000);
  TFMINI.begin(115200);

  bool initialized = false;
  while (!initialized) {
    // Initialize the ICM-20948

    myICM.begin(WIRE_PORT, AD0_VAL);
    //q0 = 1.0f;
    //q1 = 0.0f;
    //q2 = 0.0f;
    //q3 = 0.0f;
    tfmini.begin(&Serial);

    if (myICM.status != ICM_20948_Stat_Ok) {
      SERIAL_PORT.println(F("Initialization failed, retrying..."));
      delay(500);
    } else {
      initialized = true;
    }
  }

  bool success = true; // Use success to show if the DMP configuration was successful

  // Initialize the DMP
  success &= (myICM.initializeDMP() == ICM_20948_Stat_Ok);
  
  // Enable the DMP orientation sensor
  success &= (myICM.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);

  // Enable the FIFO
  success &= (myICM.enableFIFO() == ICM_20948_Stat_Ok);

  // Enable the DMP
  success &= (myICM.enableDMP() == ICM_20948_Stat_Ok);

  // Reset DMP and FIFO
  success &= (myICM.resetDMP() == ICM_20948_Stat_Ok);
  success &= (myICM.resetFIFO() == ICM_20948_Stat_Ok);

  if (success) {
    SERIAL_PORT.println(F("DMP enabled!"));
  } else {
    SERIAL_PORT.println(F("Enable DMP failed!"));
    while (1); // Do nothing more
  }
  tfmini.begin(&TFMINI);  
}

void loop() {
  // Read DMP data from the FIFO
  icm_20948_DMP_data_t data;
  //StaticJsonDocument<20000> doc;
  float angle = 35.0;  // 35 degrees, halved as per 3Blue1Brown's explanation
  float angle_rad = radians(angle);  // Convert to radians

  // LiDAR local position relative to IMU
  float lidarRay[3] = {
    cos(angle_rad),   // X position (scaled by distance from IMU)
    0,                // Y position (no offset)
    sin(angle_rad),   // Z position (scaled by distance from IMU)
  };

  myICM.readDMPdataFromFIFO(&data);

  if ((myICM.status == ICM_20948_Stat_Ok) || (myICM.status == ICM_20948_Stat_FIFOMoreDataAvail)) {
    if ((data.header & DMP_header_bitmap_Quat9) > 0) {
      // Process Quaternion 
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; 
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0;
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; 
      double q0 = sqrt(1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));


      //double norm = sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
      //q0 /= norm;
      //q1 /= norm;
      //q2 /= norm;
      //q3 /= norm;
      // Rotate the LiDAR's position vector using the quaternion
      float lidar_position[3] = {0,0,0};
      //lidar_position[0] = q0 * lidarRay[0] - q1 * lidarRay[1] - q2 * lidarRay[2];
      //lidar_position[1] = q1 * lidarRay[0] + q0 * lidarRay[1] + q3 * lidarRay[2];
      //lidar_position[2] = q2 * lidarRay[0] - q3 * lidarRay[1] + q0 * lidarRay[2];

      //Addison Sucks Tried
      //double R[3][3] = {
      //  {-1 + 2 * (q0*q0 +q1*q1), 2 * (q1 * q2 - q3 * q0), 2 * (q1 * q3 + q2 * q0)},
      //  {2 * (q1 * q2 + q3 * q0), -1 + 2 * (q0*q0+q2*q2), 2 * (q2 * q3 - q1 * q0)},
      //  {2 * (q1 * q3 - q2 * q0), 2 * (q2 * q3 + q1 * q0), -1 + 2 * (q0 * q0 + q3 * q3)}
      //  }; // Define the lidarRay array std::array<double, 3> 

      double R[3][3] = {
      {1 - 2 * (q2 * q2 + q3 * q3), 2 * (q1 * q2 - q3 * q0), 2 * (q1 * q3 + q2 * q0)},
      {2 * (q1 * q2 + q3 * q0), 1 - 2 * (q1 * q1 + q3 * q3), 2 * (q2 * q3 - q1 * q0)},
      {2 * (q1 * q3 - q2 * q0), 2 * (q2 * q3 + q1 * q0), 1 - 2 * (q1 * q1 + q2 * q2)}
      }; // Define the lidarRay array std::array<double, 3> 
        
      
      // SERIAL_PORT.print("2");
      // Calculate the new lidar position
      //double lidar_position[3];
      
      //
      for (int i = 0; i < 3; i++) { 
        for (int j = 0; j < 3; j++) {
          lidar_position[i] += R[i][j] * lidarRay[j];
        }
      }
      
      // Get LiDAR distance
      uint16_t distance  = tfmini.getDistance();
      // Convert disncetance to meters (if needed)
      //float lidar_distance_m = distance * 0.01;

      // Calculate the vertical distance from the ground
      float vertical_distance = lidar_position[2] * distance;



      // Print out the quaternion and ground distance
      SERIAL_PORT.print(F("{\"quat_w\":"));
      SERIAL_PORT.print(q0, 3);
      SERIAL_PORT.print(F(", \"quat_x\":"));
      SERIAL_PORT.print(q1, 3);
      SERIAL_PORT.print(F(", \"quat_y\":"));
      SERIAL_PORT.print(q2, 3);
      SERIAL_PORT.print(F(", \"quat_z\":"));
      SERIAL_PORT.print(q3, 3);
      SERIAL_PORT.print(F(", \"RawLidarDistance\":"));
      SERIAL_PORT.print(distance);
      SERIAL_PORT.print(F(", \"ReorientedLidarDistance\":"));
      SERIAL_PORT.print(vertical_distance, 3);
      SERIAL_PORT.println(F("}"));
    }
  }  
  // Small delay to avoid flooding the serial output
  delay(10);
}
