#include "vector.hpp"
#include <cmath>

// Small epsilon for normalization safety
static constexpr float kEpsilon = 1e-8f;

// Vector2 implementations
float Vector2::length() const {
	return std::sqrt(x * x + y * y);
}

float Vector2::lengthSquared() const {
	return x * x + y * y;
}

Vector2 Vector2::normalized() const {
	float len = length();
	if (len <= kEpsilon) return Vector2(0.0f, 0.0f);
	return Vector2(x / len, y / len);
}

void Vector2::normalize() {
	float len = length();
	if (len <= kEpsilon) {
		x = 0.0f; y = 0.0f; return;
	}
	x /= len; y /= len;
}

float Vector2::dot(const Vector2& other) const {
	return x * other.x + y * other.y;
}

float Vector2::dot(const Vector2& a, const Vector2& b) {
	return a.x * b.x + a.y * b.y;
}

float Vector2::distance(const Vector2& other) const {
	float dx = x - other.x;
	float dy = y - other.y;
	return std::sqrt(dx * dx + dy * dy);
}

// Vector3 implementations
float Vector3::length() const {
	return std::sqrt(x * x + y * y + z * z);
}

float Vector3::lengthSquared() const {
	return x * x + y * y + z * z;
}

Vector3 Vector3::normalized() const {
	float len = length();
	if (len <= kEpsilon) return Vector3(0.0f, 0.0f, 0.0f);
	return Vector3(x / len, y / len, z / len);
}

void Vector3::normalize() {
	float len = length();
	if (len <= kEpsilon) {
		x = 0.0f; y = 0.0f; z = 0.0f; return;
	}
	x /= len; y /= len; z /= len;
}

float Vector3::dot(const Vector3& other) const {
	return x * other.x + y * other.y + z * other.z;
}

float Vector3::dot(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vector3 Vector3::cross(const Vector3& other) const {
	return Vector3(
		y * other.z - z * other.y,
		z * other.x - x * other.z,
		x * other.y - y * other.x
	);
}

Vector3 Vector3::cross(const Vector3& a, const Vector3& b) {
	return Vector3(
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	);
}

float Vector3::distance(const Vector3& other) const {
	float dx = x - other.x;
	float dy = y - other.y;
	float dz = z - other.z;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

