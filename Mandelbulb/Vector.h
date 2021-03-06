//
//  Vector.h
//  Mandelbulb
//
//  Created by Jonathan Lucka on 3/24/14.
//  Copyright (c) 2014 Jonathan Lucka. All rights reserved.
//

#ifndef __Mandelbulb__Vector__
#define __Mandelbulb__Vector__

#include <iostream>

class Vector
{
public:
    float x, y, z, w;
    Vector(float a, float b, float c);
    ~Vector();
    Vector add(float val);
    Vector v_add(Vector v);
    Vector mult(float val);
    float mag();
    float dot(Vector v);
};

Vector::Vector(float a, float b, float c)
{
    x = a;
    y = b;
    z = c;
    w = 0.0;
}

Vector::~Vector(){}

Vector Vector::add(float val)
{
    return Vector(x + val, y + val, z + val);
}

Vector Vector::v_add(Vector v)
{
    return Vector(x + v.x, y + v.y, z + v.z);
}

Vector Vector::mult(float val)
{
    return Vector(x * val, y * val, z * val);
}

float Vector::mag()
{
    return sqrt(x * x + y * y + z * z);
}

float Vector::dot(Vector v)
{
    return x * v.x + y * v.y + z * v.z;
}

#endif /* defined(__Mandelbulb__Vector__) */
