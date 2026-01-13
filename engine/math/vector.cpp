#include "vector.hpp"
#include <cmath>

namespace mathplease {

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

// Vector4 implementations
float Vector4::length() const {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

float Vector4::lengthSquared() const {
    return x * x + y * y + z * z + w * w;
}

Vector4 Vector4::normalized() const {
    float len = length();
    if (len <= kEpsilon) return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
    return Vector4(x / len, y / len, z / len, w / len);
}

void Vector4::normalize() {
    float len = length();
    if (len <= kEpsilon) {
        x = 0.0f; y = 0.0f; z = 0.0f; w = 0.0f; return;
    }
    x /= len; y /= len; z /= len; w /= len;
}

float Vector4::dot(const Vector4& other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

float Vector4::dot(const Vector4& a, const Vector4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float Vector4::distance(const Vector4& other) const {
    float dx = x - other.x;
    float dy = y - other.y;
    float dz = z - other.z;
    float dw = w - other.w;
    return std::sqrt(dx * dx + dy * dy + dz * dz + dw * dw);
}

// Matrix4 implementations
Matrix4::Matrix4() {
    for (int i = 0; i < 16; ++i) {
        m[i] = 0.0f;
    }
}

Matrix4::Matrix4(float diagonal) {
    for (int i = 0; i < 16; ++i) {
        m[i] = 0.0f;
    }
    m[0] = m[5] = m[10] = m[15] = diagonal;
}

Matrix4::Matrix4(const float* data) {
    for (int i = 0; i < 16; ++i) {
        m[i] = data[i];
    }
}

Matrix4 Matrix4::identity() {
    return Matrix4(1.0f);
}

Matrix4 Matrix4::translate(const Vector3& translation) {
    Matrix4 result = identity();
    result(0, 3) = translation.x;
    result(1, 3) = translation.y;
    result(2, 3) = translation.z;
    return result;
}

Matrix4 Matrix4::rotateX(float angleRadians) {
    Matrix4 result = identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    result(1, 1) = c;
    result(1, 2) = -s;
    result(2, 1) = s;
    result(2, 2) = c;
    return result;
}

Matrix4 Matrix4::rotateY(float angleRadians) {
    Matrix4 result = identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    result(0, 0) = c;
    result(0, 2) = s;
    result(2, 0) = -s;
    result(2, 2) = c;
    return result;
}

Matrix4 Matrix4::rotateZ(float angleRadians) {
    Matrix4 result = identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    result(0, 0) = c;
    result(0, 1) = -s;
    result(1, 0) = s;
    result(1, 1) = c;
    return result;
}

Matrix4 Matrix4::rotate(const Vector3& axis, float angleRadians) {
    Vector3 a = axis.normalized();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    float omc = 1.0f - c;
    
    Matrix4 result;
    result(0, 0) = c + a.x * a.x * omc;
    result(0, 1) = a.x * a.y * omc - a.z * s;
    result(0, 2) = a.x * a.z * omc + a.y * s;
    result(0, 3) = 0.0f;
    
    result(1, 0) = a.y * a.x * omc + a.z * s;
    result(1, 1) = c + a.y * a.y * omc;
    result(1, 2) = a.y * a.z * omc - a.x * s;
    result(1, 3) = 0.0f;
    
    result(2, 0) = a.z * a.x * omc - a.y * s;
    result(2, 1) = a.z * a.y * omc + a.x * s;
    result(2, 2) = c + a.z * a.z * omc;
    result(2, 3) = 0.0f;
    
    result(3, 0) = 0.0f;
    result(3, 1) = 0.0f;
    result(3, 2) = 0.0f;
    result(3, 3) = 1.0f;
    
    return result;
}

Matrix4 Matrix4::scale(const Vector3& scale) {
    Matrix4 result = identity();
    result(0, 0) = scale.x;
    result(1, 1) = scale.y;
    result(2, 2) = scale.z;
    return result;
}

Matrix4 Matrix4::scale(float uniformScale) {
    return scale(Vector3(uniformScale, uniformScale, uniformScale));
}

Matrix4 Matrix4::perspective(float fovYRadians, float aspectRatio, float nearPlane, float farPlane) {
    Matrix4 result;
    float tanHalfFovy = std::tan(fovYRadians * 0.5f);
    
    result(0, 0) = 1.0f / (aspectRatio * tanHalfFovy);
    result(1, 1) = 1.0f / tanHalfFovy;
    result(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    result(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    result(3, 2) = -1.0f;
    
    return result;
}

Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
    Matrix4 result = identity();
    
    result(0, 0) = 2.0f / (right - left);
    result(1, 1) = 2.0f / (top - bottom);
    result(2, 2) = -2.0f / (farPlane - nearPlane);
    result(0, 3) = -(right + left) / (right - left);
    result(1, 3) = -(top + bottom) / (top - bottom);
    result(2, 3) = -(farPlane + nearPlane) / (farPlane - nearPlane);
    
    return result;
}

Matrix4 Matrix4::lookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    Vector3 f = (center - eye).normalized();
    Vector3 u = up.normalized();
    Vector3 s = f.cross(u).normalized();
    u = s.cross(f);
    
    Matrix4 result = identity();
    result(0, 0) = s.x;
    result(0, 1) = s.y;
    result(0, 2) = s.z;
    result(1, 0) = u.x;
    result(1, 1) = u.y;
    result(1, 2) = u.z;
    result(2, 0) = -f.x;
    result(2, 1) = -f.y;
    result(2, 2) = -f.z;
    result(0, 3) = -s.dot(eye);
    result(1, 3) = -u.dot(eye);
    result(2, 3) = f.dot(eye);
    
    return result;
}

Matrix4 Matrix4::operator+(const Matrix4& other) const {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] + other.m[i];
    }
    return result;
}

Matrix4 Matrix4::operator-(const Matrix4& other) const {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] - other.m[i];
    }
    return result;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result(i, j) = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result(i, j) += (*this)(i, k) * other(k, j);
            }
        }
    }
    return result;
}

Matrix4 Matrix4::operator*(float scalar) const {
    Matrix4 result;
    for (int i = 0; i < 16; ++i) {
        result.m[i] = m[i] * scalar;
    }
    return result;
}

Vector4 Matrix4::operator*(const Vector4& vec) const {
    return Vector4(
        m[0] * vec.x + m[4] * vec.y + m[8]  * vec.z + m[12] * vec.w,
        m[1] * vec.x + m[5] * vec.y + m[9]  * vec.z + m[13] * vec.w,
        m[2] * vec.x + m[6] * vec.y + m[10] * vec.z + m[14] * vec.w,
        m[3] * vec.x + m[7] * vec.y + m[11] * vec.z + m[15] * vec.w
    );
}

Vector3 Matrix4::transformPoint(const Vector3& point) const {
    Vector4 result = (*this) * Vector4(point.x, point.y, point.z, 1.0f);
    if (result.w != 0.0f) {
        return Vector3(result.x / result.w, result.y / result.w, result.z / result.w);
    }
    return result.xyz();
}

Vector3 Matrix4::transformVector(const Vector3& vector) const {
    Vector4 result = (*this) * Vector4(vector.x, vector.y, vector.z, 0.0f);
    return result.xyz();
}

Matrix4 Matrix4::transposed() const {
    Matrix4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result(i, j) = (*this)(j, i);
        }
    }
    return result;
}

float Matrix4::determinant() const {
    float det = 
        m[0] * (m[5] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[9] * m[15] - m[11] * m[13]) + m[7] * (m[9] * m[14] - m[10] * m[13])) -
        m[1] * (m[4] * (m[10] * m[15] - m[11] * m[14]) - m[6] * (m[8] * m[15] - m[11] * m[12]) + m[7] * (m[8] * m[14] - m[10] * m[12])) +
        m[2] * (m[4] * (m[9]  * m[15] - m[11] * m[13]) - m[5] * (m[8] * m[15] - m[11] * m[12]) + m[7] * (m[8] * m[13] - m[9]  * m[12])) -
        m[3] * (m[4] * (m[9]  * m[14] - m[10] * m[13]) - m[5] * (m[8] * m[14] - m[10] * m[12]) + m[6] * (m[8] * m[13] - m[9]  * m[12]));
    return det;
}

Matrix4 Matrix4::inverse() const {
    Matrix4 inv = identity();

    // Extract rotation (upper-left 3x3)
    float r00 = m[0],  r01 = m[1],  r02 = m[2];
    float r10 = m[4],  r11 = m[5],  r12 = m[6];
    float r20 = m[8],  r21 = m[9],  r22 = m[10];

    // Transpose rotation (inverse if orthonormal)
    inv.m[0]  = r00; inv.m[1]  = r10; inv.m[2]  = r20;
    inv.m[4]  = r01; inv.m[5]  = r11; inv.m[6]  = r21;
    inv.m[8]  = r02; inv.m[9]  = r12; inv.m[10] = r22;

    // Inverse translation
    float tx = m[12];
    float ty = m[13];
    float tz = m[14];

    inv.m[12] = -(inv.m[0] * tx + inv.m[4] * ty + inv.m[8]  * tz);
    inv.m[13] = -(inv.m[1] * tx + inv.m[5] * ty + inv.m[9]  * tz);
    inv.m[14] = -(inv.m[2] * tx + inv.m[6] * ty + inv.m[10] * tz);

    return inv;
}




} // namespace mathplease

