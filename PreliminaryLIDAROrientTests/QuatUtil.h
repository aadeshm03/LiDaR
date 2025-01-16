#ifndef QUATUTIL_H
#define QUATUTIL_H

#include <vector>
#include <cmath>


struct quaternion {
  double w,x,y,z; // w is a scalar, x,y,z are a vector

  quaternion(double nw, double nx, double ny, double nz) : w(nw), x(nx), y(ny), z(nz) {}
};

struct vec3 {
  double x,y,z; // w is a scalar, x,y,z are a vector

  vec3(double nx, double ny, double nz) : x(nx), y(ny), z(nz) {}
};


vec3 operator*(const vec3& rhs, const double lhs);

vec3 operator*(const double lhs, const vec3& rhs);

vec3 operator+(const vec3& lhs, const vec3& rhs);

std::vector<double> quat2Euler(double qw, double qx, double qy, double qz);

vec3 quatRotate(const vec3& v, const quaternion& q);

#endif