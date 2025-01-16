#include "QuatUtil.h"


vec3 operator*(const double lhs, const vec3& rhs) {
  return rhs * lhs;
}

vec3 operator*(const vec3& rhs, const double lhs) {
  return vec3(rhs.x*lhs, rhs.y*lhs, rhs.z*lhs);
}

vec3 operator+(const vec3& lhs, const vec3& rhs) {
  return vec3(lhs.x+rhs.x,
              lhs.y+rhs.y,
              lhs.z+rhs.z);
}

double dot(const vec3& v1, const vec3& v2) {
    return v1.x*v2.x +
           v1.y*v2.y +
           v1.z*v2.z;
}

vec3 cross(const vec3& v1, const vec3& v2) { 
    return vec3( v1.y * v2.z - v1.z * v2.y,
                 v1.z * v2.x - v1.x * v2.z, 
                 v1.x * v2.y - v1.y * v2.x ); 
    }

std::vector<double> quat2Euler(double qw, double qx, double qy, double qz) {
    double yaw, pitch, roll;
    if (2*(qx*qz-qw*qy) >= 0.94) { // Preventing gimbal lock for north pole
        yaw = atan2(qx*qy-qw*qz,qx*qz+qw*qy);
        roll = 0;
    } else if (2*(qx*qz-qw*qy) <= -0.94) { // Preventing gimbal lock for south pole
        yaw = -atan2(qx*qy-qw*qz,qx*qz+qw*qy);
        roll = 0;
    } else {
        yaw = atan2(qy*qz + qw*qx,
            1/2 - (qx*qx + qy*qy));
        roll = atan2(qx*qy - qw*qz,
            1/2 - (qy*qy + qz*qz));
    }
    pitch = asin(-2*(qx * qz - qw * qy));

    return {yaw, pitch, roll};
}

vec3 quatRotate(const vec3& v, const quaternion& q) {
    // Extract the vector part of the quaternion
    vec3 u(q.x, q.y, q.z);

    // Extract the scalar part of the quaternion
    float s = q.w;

    // Do the math
    vec3 vprime = 2.0 * dot(u, v) * u
                + (s*s - dot(u, u)) * v
                + 2.0 * s * cross(u, v);

    return vprime;
}