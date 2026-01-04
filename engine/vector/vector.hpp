#ifndef VECTOR_HPP
#define VECTOR_HPP

class Vector {
public:
    double x, y, z;

    Vector(double x = 0, double y = 0, double z = 0);

    Vector operator+(const Vector& other) const;
    Vector operator-(const Vector& other) const;
    Vector operator*(double scalar) const;
    double dot(const Vector& other) const;
    Vector cross(const Vector& other) const;
    double length() const;
    Vector normalize() const;
};

#endif
