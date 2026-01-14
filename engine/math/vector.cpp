#include "vector.hpp"
#include <cmath>
#include <cstring>

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
    std::memset(result.m, 0, sizeof(result.m));
    float tanHalfFovy = std::tan(fovYRadians * 0.5f);
    
    result(0, 0) = 1.0f / (aspectRatio * tanHalfFovy);
    result(1, 1) = -1.0f / tanHalfFovy; 
    result(2, 2) = farPlane / (nearPlane - farPlane);
    result(2, 3) = (farPlane * nearPlane) / (nearPlane - farPlane);
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
    Matrix4 result;
    float s0 = m[0] * m[5] - m[1] * m[4];
    float s1 = m[0] * m[6] - m[2] * m[4];
    float s2 = m[0] * m[7] - m[3] * m[4];
    float s3 = m[1] * m[6] - m[2] * m[5];
    float s4 = m[1] * m[7] - m[3] * m[5];
    float s5 = m[2] * m[7] - m[3] * m[6];

    float c5 = m[10] * m[15] - m[11] * m[14];
    float c4 = m[9] * m[15] - m[11] * m[13];
    float c3 = m[9] * m[14] - m[10] * m[13];
    float c2 = m[8] * m[15] - m[11] * m[12];
    float c1 = m[8] * m[14] - m[10] * m[12];
    float c0 = m[8] * m[13] - m[9] * m[12];

    float det = (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

    if (std::fabs(det) < kEpsilon) {
        // Non-invertible matrix, return identity as a fallback
        return identity();
    }
    float invDet = 1.0f / det;

    // Calculate the adjugate matrix multiplied by 1/det
    result.m[0] = ( m[5] * c5 - m[6] * c4 + m[7] * c3) * invDet;
    result.m[1] = (-m[1] * c5 + m[2] * c4 - m[3] * c3) * invDet;
    result.m[2] = ( m[13] * s5 - m[14] * s4 + m[15] * s3) * invDet;
    result.m[3] = (-m[9] * s5 + m[10] * s4 - m[11] * s3) * invDet;

    result.m[4] = (-m[4] * c5 + m[6] * c2 - m[7] * c1) * invDet;
    result.m[5] = ( m[0] * c5 - m[2] * c2 + m[3] * c1) * invDet;
    result.m[6] = (-m[12] * s5 + m[14] * s2 - m[15] * s1) * invDet;
    result.m[7] = ( m[8] * s5 - m[10] * s2 + m[11] * s1) * invDet;

    result.m[8] = ( m[4] * c4 - m[5] * c2 + m[7] * c0) * invDet;
    result.m[9] = (-m[0] * c4 + m[1] * c2 - m[3] * c0) * invDet;
    result.m[10] = ( m[12] * s4 - m[13] * s2 + m[15] * s0) * invDet;
    result.m[11] = (-m[8] * s4 + m[9] * s2 - m[11] * s0) * invDet;

    result.m[12] = (-m[4] * c3 + m[5] * c1 - m[6] * c0) * invDet;
    result.m[13] = ( m[0] * c3 - m[1] * c1 + m[2] * c0) * invDet;
    result.m[14] = (-m[12] * s3 + m[13] * s1 - m[14] * s0) * invDet;
    result.m[15] = ( m[8] * s3 - m[9] * s1 + m[10] * s0) * invDet;

    return result;


}




} // namespace mathplease

