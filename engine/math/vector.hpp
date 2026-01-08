#pragma once


class Vector2 {
public:
    float x;
    float y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    Vector2 operator/(float scalar) const {
        return Vector2(x / scalar, y / scalar);
    }

    // Length and normalization
    float length() const;
    float lengthSquared() const;
    Vector2 normalized() const;
    void normalize();

    // Dot and distance
    float dot(const Vector2& other) const;
    static float dot(const Vector2& a, const Vector2& b);
    float distance(const Vector2& other) const;
};

class Vector3 {
public:
    float x;
    float y;
    float z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return Vector3(x + other.x, y + other.y, z + other.z);
    }

    Vector3 operator-(const Vector3& other) const {
        return Vector3(x - other.x, y - other.y, z - other.z);
    }

    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    Vector3 operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    // Length and normalization
    float length() const;
    float lengthSquared() const;
    Vector3 normalized() const;
    void normalize();

    // Dot, cross and distance
    float dot(const Vector3& other) const;
    static float dot(const Vector3& a, const Vector3& b);
    Vector3 cross(const Vector3& other) const;
    static Vector3 cross(const Vector3& a, const Vector3& b);
    float distance(const Vector3& other) const;
};