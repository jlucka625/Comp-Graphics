class Vector{
        double x, y, z;
    public:
        Vector(double a, double b, double c);
        void setX(double a);
        void setY(double b);
        void setZ(double c);
        double getX();
        double getY();
        double getZ();
};

Vector::Vector(double a, double b, double c)
{
    x = a;
    y = b;
    z = c;
}

void Vector::setX(double a)
{
    x = a;
}

void Vector::setY(double b)
{
    y = b;
}

void Vector::setZ(double c)
{
    z = c;
}

double Vector::getX()
{
    return x;
}

double Vector::getY()
{
    return y;
}

double Vector::getZ()
{
    return z;
}

double dotProduct(Vector v1, Vector v2)
{
    return (v1.getX() * v2.getX()) + (v1.getY() * v2.getY()) + (v1.getZ() * v2.getZ());
}
Vector sub(Vector v1, Vector v2)
{
    Vector v3 (v1.getX() - v2.getX(), v1.getY() - v2.getY(), v1.getZ() - v2.getZ());
    return v3;
}
Vector add(Vector v1, Vector v2)
{
    Vector v3 (v1.getX() + v2.getX(), v1.getY() + v2.getY(), v1.getZ() + v2.getZ());
    return v3;
}

double magnitude(Vector v1)
{
    return sqrt(pow(v1.getX(), 2) + pow(v1.getY(), 2) + pow(v1.getZ(), 2));
}

Vector mult(double val, Vector v)
{
    Vector newV (v.getX() * val, v.getY() * val, v.getZ() * val);
    return newV;
}

Vector crossProduct3D(Vector u, Vector v)
{
    double x = (u.getY() * v.getZ()) - (u.getZ() * v.getY());
    double y = (u.getZ() * v.getX()) - (u.getX() * v.getZ());
    double z = (u.getX() * v.getY()) - (u.getY() * v.getX());
    Vector crossed (x, y, z);
    return crossed;
}

double absv(double val)
{
    if(val < 0)
        val = -val;
    return val;
}




