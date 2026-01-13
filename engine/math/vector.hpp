#pragma once


namespace mathplease {

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
    mathplease::Vector3 normalized() const;
    void normalize();

    // Dot, cross and distance
    float dot(const mathplease::Vector3& other) const;
    static float dot(const mathplease::Vector3& a, const mathplease::Vector3& b);
    mathplease::Vector3 cross(const mathplease::Vector3& other) const;
    static mathplease::Vector3 cross(const mathplease::Vector3& a, const mathplease::Vector3& b);
    float distance(const mathplease::Vector3& other) const;
};

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;

    Vector4() : x(0), y(0), z(0), w(0) {}
    Vector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vector4(const mathplease::Vector3& v3, float w) : x(v3.x), y(v3.y), z(v3.z), w(w) {}

    Vector4 operator+(const Vector4& other) const {
        return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
    }

    Vector4 operator-(const Vector4& other) const {
        return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
    }

    Vector4 operator*(float scalar) const {
        return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    Vector4 operator/(float scalar) const {
        return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
    }

    // Length and normalization
    float length() const;
    float lengthSquared() const;
    Vector4 normalized() const;
    void normalize();

    // Dot and distance
    float dot(const Vector4& other) const;
    static float dot(const Vector4& a, const Vector4& b);
    float distance(const Vector4& other) const;

    // Conversion
    mathplease::Vector3 xyz() const { return mathplease::Vector3(x, y, z); }
};

class Matrix4 {
public:
    // Storage: column-major order (like OpenGL/Vulkan)
    float m[16];

    // Constructors
    Matrix4();
    Matrix4(float diagonal);
    Matrix4(const float* data);
    
    // Static factory methods
    static Matrix4 identity();
    static Matrix4 translate(const mathplease::Vector3& translation);
    static Matrix4 rotate(const mathplease::Vector3& axis, float angleRadians);
    static Matrix4 rotateX(float angleRadians);
    static Matrix4 rotateY(float angleRadians);
    static Matrix4 rotateZ(float angleRadians);
    static Matrix4 scale(const mathplease::Vector3& scale);
    static Matrix4 scale(float uniformScale);
    
    // Camera/projection matrices
    static Matrix4 perspective(float fovYRadians, float aspectRatio, float nearPlane, float farPlane);
    static Matrix4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
    static Matrix4 lookAt(const mathplease::Vector3& eye, const mathplease::Vector3& center, const mathplease::Vector3& up);

    // Matrix operations
    Matrix4 operator+(const Matrix4& other) const;
    Matrix4 operator-(const Matrix4& other) const;
    Matrix4 operator*(const Matrix4& other) const;
    Matrix4 operator*(float scalar) const;
    
    Vector4 operator*(const Vector4& vec) const;
    mathplease::Vector3 transformPoint(const mathplease::Vector3& point) const;
    mathplease::Vector3 transformVector(const mathplease::Vector3& vector) const;
    
    // Matrix properties
    Matrix4 transposed() const;
    Matrix4 inverse() const;
    float determinant() const;
    
    
    // Element access
    float& operator()(int row, int col) { return m[col * 4 + row]; }
    const float& operator()(int row, int col) const { return m[col * 4 + row]; }
    
    // Data access
    const float* data() const { return m; }
    float* data() { return m; }
};

} // namespace mathplease