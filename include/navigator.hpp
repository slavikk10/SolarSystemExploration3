#include <iostream>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functionsupport.hpp>
#include <constants.hpp>
#include <glmextension.hpp>

#pragma once

const double epsilon = 0.00001;

struct ObjectState
{
    glm::dvec3 r;
    glm::dvec3 v;
};

struct ObjectOrbitalState
{
    double meanAnomaly;
    double trueAnomaly;

    double timeOfPeriapsisPassage;
};

struct Orbit
{
    double semiMajorAxis;
    double semiMinorAxis;

    double eccentricity;
    double inclination;
    double raan;
    double argumentOfPeriapsis;

    double meanMotion;
};

struct OrbitalObject
{
    ObjectState state;
    ObjectOrbitalState orbitalState;
    Orbit orbit;
};

struct LambertOutput 
{
    glm::dvec3 requiredVelocity;
    double velocity;

    double A;
    double z;
    double y;
    double f;
    double g;

    double r1l;
    double r2l;
    glm::dvec3 r1;
    glm::dvec3 r2;
};

struct Transfer
{
    double deltaVelocity;
    double departureTime;
    double timeOfFlight;

    glm::dvec3 burnDirection;
    glm::dvec3 velocityVector;

    LambertOutput out;
};

ObjectState findObjectStateAtTime(double time, Orbit rocketOrbit, double timeOfPeriapsisPassage, double gravitationalParameter)
{
    double futureMeanAnomaly = rocketOrbit.meanMotion * (time - timeOfPeriapsisPassage);

    double eccentricAnomaly = futureMeanAnomaly;
    for (unsigned int i = 0; i < 5; i++)
        eccentricAnomaly -= (eccentricAnomaly - rocketOrbit.eccentricity * sin(eccentricAnomaly) - futureMeanAnomaly) / (1 - rocketOrbit.eccentricity * cos(eccentricAnomaly));

    double trueAnomaly = 2 * atan2(std::sqrt(1 + rocketOrbit.eccentricity) * sin(eccentricAnomaly / 2), std::sqrt(1 - rocketOrbit.eccentricity ) * cos(eccentricAnomaly / 2));

    double radius       = rocketOrbit.semiMajorAxis * (1 - std::pow(rocketOrbit.eccentricity, 2)) / (1 + rocketOrbit.eccentricity * cos(trueAnomaly));
    glm::dvec3 position = glm::dvec3(radius * cos(trueAnomaly), 0.0, radius * sin(trueAnomaly));

    glm::mat3 rotationMatrix1 = glm::mat3(glm::vec3(cos(rocketOrbit.raan), 0.0, sin(rocketOrbit.raan)), glm::vec3(0.0, 1.0, 0.0), glm::vec3(-sin(rocketOrbit.raan), 0.0, cos(rocketOrbit.raan)));
    glm::mat3 rotationMatrix2 = glm::mat3(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, cos(rocketOrbit.inclination), -sin(rocketOrbit.inclination)), glm::vec3(0.0, sin(rocketOrbit.inclination), cos(rocketOrbit.inclination)));
    glm::mat3 rotationMatrix3 = glm::mat3(glm::vec3(cos(rocketOrbit.argumentOfPeriapsis), 0.0, sin(rocketOrbit.argumentOfPeriapsis)), glm::vec3(0.0, 1.0, 0.0), glm::vec3(-sin(rocketOrbit.argumentOfPeriapsis), 0.0, cos(rocketOrbit.argumentOfPeriapsis)));

    glm::dvec3 velocity = sqrt(gravitationalParameter / rocketOrbit.semiMajorAxis * (1 - pow(rocketOrbit.eccentricity, 2))) * glm::dvec3(-sin(trueAnomaly), 0.0, rocketOrbit.eccentricity + cos(trueAnomaly));

    ObjectState result;
    result.r = glm::dmat3(rotationMatrix1 * rotationMatrix2 * rotationMatrix3) * position;
    result.v = glm::dmat3(rotationMatrix1 * rotationMatrix2 * rotationMatrix3) * velocity;

    return result;
}

glm::dvec3 findMeanPosition(double time, OrbitalObject rocketOrbital, double gravitationalParameter, bool debug=false)
{
    double r             = glm::length(rocketOrbital.state.r);
    double v             = glm::length(rocketOrbital.state.v);

    double E             = pow(v, 2.0) / 2.0 - gravitationalParameter / r;
    double a             = -gravitationalParameter / (2.0 * E);

    glm::dvec3 h         = glm::cross(rocketOrbital.state.r, rocketOrbital.state.v);

    glm::dvec3 eVec      = glm::cross(rocketOrbital.state.v, h) / gravitationalParameter - glm::normalize(rocketOrbital.state.r);
    double eccentricity  = glm::length(eVec);
    
    double trueAnomaly   = acos(glm::dot(eVec, rocketOrbital.state.r) / (rocketOrbital.orbit.eccentricity * r));
    double eccentricAnom = 2.0 * atan(sqrt((1 - rocketOrbital.orbit.eccentricity) / (1 + rocketOrbital.orbit.eccentricity)) * tan(trueAnomaly / 2.0));
    double meanAnomaly   = eccentricAnom - rocketOrbital.orbit.eccentricity * sin(eccentricAnom);

    double orbitalSpeed  = sqrt(gravitationalParameter / pow(rocketOrbital.orbit.semiMajorAxis, 3.0));
    double mFuture       = meanAnomaly + orbitalSpeed * time;

    double eFuture = mFuture;
    double previousE = 0.0;
    double deltaE;

    while (deltaE > epsilon)
    {
        previousE = eFuture;
        eFuture -= (eFuture - rocketOrbital.orbit.eccentricity * sin(eFuture) - mFuture) / (1.0 - rocketOrbital.orbit.eccentricity * cos(eFuture));
        deltaE = eFuture - previousE;
    }

    glm::dvec3 eNorm        = glm::normalize(eVec);
    glm::dvec3 orbitalPlane = glm::normalize(glm::cross(h, eNorm));

    return rocketOrbital.orbit.semiMajorAxis * (cos(eFuture) - rocketOrbital.orbit.eccentricity) * eNorm + rocketOrbital.orbit.semiMinorAxis * sin(eFuture) * orbitalPlane;
}

glm::dvec3 findMeanPosition(double time, ObjectState rv, double gravitationalParameter, bool debug=false)
{
    double r             = glm::length(rv.r);
    double v             = glm::length(rv.v);

    double E             = pow(v, 2.0) / 2.0 - gravitationalParameter / r;
    double a             = -gravitationalParameter / (2.0 * E);

    glm::dvec3 h         = glm::cross(rv.r, rv.v);

    glm::dvec3 eVec      = glm::cross(rv.v, h) / gravitationalParameter - glm::normalize(rv.r);
    double eccentricity  = glm::length(eVec);
    
    double trueAnomaly;
    if (eccentricity > 0.0)
        trueAnomaly = acos(glm::dot(eVec, rv.r) / (eccentricity * r));
    else
        trueAnomaly = atan2(rv.r.z, rv.r.x);

    if (glm::dot(h, glm::cross(eVec, rv.r)) < 0.0)
        trueAnomaly = -trueAnomaly;
    
    double eccentricAnom = 2.0 * atan(sqrt((1 - eccentricity) / (1 + eccentricity)) * tan(trueAnomaly / 2.0));
    double meanAnomaly   = eccentricAnom - eccentricity * sin(eccentricAnom);

    double orbitalSpeed  = sqrt(gravitationalParameter / pow(a, 3.0));
    double mFuture       = meanAnomaly + orbitalSpeed * time;

    double eFuture = mFuture;
    double previousE = 0.0;
    double deltaE = 100 * epsilon;

    while (deltaE > epsilon)
    {
        previousE = eFuture;
        eFuture -= ((eFuture - eccentricity * sin(eFuture) - mFuture) / (1.0 - eccentricity * cos(eFuture)));
        deltaE = abs(eFuture - previousE);
    }

    glm::dvec3 eNorm;
    if (eccentricity > 0)
        eNorm = glm::normalize(eVec);
    else
        eNorm = glm::dvec3(1.0, 0.0, 0.0);
    
    glm::dvec3 orbitalPlane = glm::normalize(glm::cross(h, eNorm));

    double x = a * (cos(eFuture) - eccentricity);
    double y = a * sqrt(1.0 - pow(eccentricity, 2.0)) * sin(eFuture);
    
    return x * eNorm + y * orbitalPlane;
}

glm::dvec3 findMeanPosition(double time, glm::dvec3 r, glm::dvec3 v, double gravitationalParameter, bool debug=false)
{
    double rl            = glm::length(r);
    double vl            = glm::length(v);

    double E             = pow(vl, 2.0) / 2.0 - gravitationalParameter / rl;
    double a             = -gravitationalParameter / (2.0 * E);

    glm::dvec3 h         = glm::cross(r, v);

    glm::dvec3 eVec      = glm::cross(v, h) / gravitationalParameter - glm::normalize(r);
    double eccentricity  = glm::length(eVec);
    
    double trueAnomaly;
    if (eccentricity > 0.0)
        trueAnomaly = acos(glm::dot(eVec, r) / (eccentricity * rl));
    else
        trueAnomaly = atan2(r.z, r.x);

    if (glm::dot(h, glm::cross(eVec, r)) < 0.0)
        trueAnomaly = -trueAnomaly;
    
    double eccentricAnom = 2.0 * atan(sqrt((1 - eccentricity) / (1 + eccentricity)) * tan(trueAnomaly / 2.0));
    double meanAnomaly   = eccentricAnom - eccentricity * sin(eccentricAnom);

    double orbitalSpeed  = sqrt(gravitationalParameter / pow(a, 3.0));
    double mFuture       = meanAnomaly + orbitalSpeed * time;

    double eFuture = mFuture;
    double previousE = 0.0;
    double deltaE;

    while (deltaE > epsilon)
    {
        previousE = eFuture;
        eFuture -= ((eFuture - eccentricity * sin(eFuture) - mFuture) / (1.0 - eccentricity * cos(eFuture)));
        deltaE = abs(eFuture - previousE);
    }

    glm::dvec3 eNorm;
    if (eccentricity > 0)
        eNorm = glm::normalize(eVec);
    else
        eNorm = glm::dvec3(1.0, 0.0, 0.0);
    
    glm::dvec3 orbitalPlane = glm::normalize(glm::cross(h, eNorm));

    double x = a * (cos(eFuture) - eccentricity);
    double y = a * sqrt(1.0 - pow(eccentricity, 2.0)) * sin(eFuture);

    return x * eNorm + y * orbitalPlane;
}

double bisection(double lowerBound, double higherBound, double timeOfFlight, double r1l, double r2l, double gravitationalParameter, double A)
{
    double z = 0.0;
    double S, C, T, y = 0.0;
    for (unsigned int i = 0; i < 1000; i++)
    {
        z = (lowerBound + higherBound) / 2.0;

        if (z > epsilon)
            C = (1.0 - cos(sqrt(z))) / z;
        else if (z < -epsilon)
            C = (cosh(sqrt(-z)) - 1.0) / -z;
        else
            C = 1.0 / 2.0;

        if (z > epsilon)
            S = (sqrt(z) - sin(sqrt(z))) / pow(sqrt(z), 3.0);
        else if (z < -epsilon)
            S = (sinh(sqrt(-z)) - sqrt(-z)) / pow(sqrt(-z), 3.0);
        else
            S = 1.0 / 6.0;

        y = r1l + r2l + A * ((z * S - 1.0) / sqrt(C));
        T = (1.0 / sqrt(gravitationalParameter)) * (pow(y / C, 3.0 / 2.0) * S + A * sqrt(y));

        if (T < timeOfFlight)
            lowerBound  = z;
        else
            higherBound = z;

        if (abs(T - timeOfFlight) < epsilon)
            break;
    }

    return z;
}

glm::dvec3 keplerUniversal(double time, ObjectState rv, double gravitationalParameter)
{
    glm::dvec3 r0 = rv.r;
    glm::dvec3 v0 = rv.v;

    double r0l = glm::length(r0);
    double v0l = glm::length(v0);

    double a = (2.0 / r0l) - (pow(v0l, 2.0) / gravitationalParameter);

    double X  = sqrt(gravitationalParameter) * time * a;
    double Xp = 0.0;

    double C, S, r;
    while (abs(X - Xp) > epsilon)
    {
        double p = a * pow(X, 2);

        if (p > epsilon)
            C = (1.0 - cos(sqrt(p))) / p;
        else if (p < -epsilon)
            C = (cosh(sqrt(-p)) - 1.0) / -p;
        else
            C = 1.0 / 2.0;

        if (p > epsilon)
            S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
        else if (p < -epsilon)
            S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
        else
            S = 1.0 / 6.0;

        double r_1 = pow(X, 2.0) * C;
        double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
        r          = r_1 + r_2 + r0l * (1.0 - p * C);

        double temp1 = sqrt(gravitationalParameter) * time;
        double temp2 = pow(X, 3.0) * S;
        double temp3 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * pow(X, 2.0) * C;
        double temp4 = r0l * X * (1.0 - p * S);

        Xp = X;
        X  = Xp + (temp1 - temp2 - temp3 - temp4) / r;
    }

    double p = a * pow(X, 2);

    if (p > epsilon)
        C = (1.0 - cos(sqrt(p))) / p;
    else if (p < -epsilon)
        C = (cosh(sqrt(-p)) - 1.0) / -p;
    else
        C = 1.0 / 2.0;

    if (p > epsilon)
        S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
    else if (p < -epsilon)
        S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
    else
        S = 1.0 / 6.0;

    double r_1 = pow(X, 2.0) * C;
    double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
    r          = r_1 + r_2 + r0l * (1.0 - p * C);

    double f = 1.0 - (pow(X, 2.0) * C) / r0l;
    double g = time - (pow(X, 3.0) * S) / sqrt(gravitationalParameter);

    double fd = (sqrt(gravitationalParameter) / (r * r0l)) * X * (p * S - 1.0);
    double gd = 1.0 - (pow(X, 2.0) * C) / r;

    return f * r0 + g * v0;
}

glm::dvec3 keplerUniversal(double time, OrbitalObject orbital, double gravitationalParameter)
{
    glm::dvec3 r0 = orbital.state.r;
    glm::dvec3 v0 = orbital.state.v;

    double r0l = glm::length(r0);
    double v0l = glm::length(v0);

    double a = (2.0 / r0l) - (pow(v0l, 2.0) / gravitationalParameter);

    double X  = sqrt(gravitationalParameter) * time * a;
    double Xp = 0.0;

    double C, S, r;
    while (abs(X - Xp) > epsilon)
    {
        double p = a * pow(X, 2);

        if (p > epsilon)
            C = (1.0 - cos(sqrt(p))) / p;
        else if (p < -epsilon)
            C = (cosh(sqrt(-p)) - 1.0) / -p;
        else
            C = 1.0 / 2.0;

        if (p > epsilon)
            S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
        else if (p < -epsilon)
            S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
        else
            S = 1.0 / 6.0;

        double r_1 = pow(X, 2.0) * C;
        double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
        r          = r_1 + r_2 + r0l * (1.0 - p * C);

        double temp1 = sqrt(gravitationalParameter) * time;
        double temp2 = pow(X, 3.0) * S;
        double temp3 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * pow(X, 2.0) * C;
        double temp4 = r0l * X * (1.0 - p * S);

        Xp = X;
        X  = Xp + (temp1 - temp2 - temp3 - temp4) / r;
    }

    double p = a * pow(X, 2);

    if (p > epsilon)
        C = (1.0 - cos(sqrt(p))) / p;
    else if (p < -epsilon)
        C = (cosh(sqrt(-p)) - 1.0) / -p;
    else
        C = 1.0 / 2.0;

    if (p > epsilon)
        S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
    else if (p < -epsilon)
        S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
    else
        S = 1.0 / 6.0;

    double r_1 = pow(X, 2.0) * C;
    double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
    r          = r_1 + r_2 + r0l * (1.0 - p * C);

    double f = 1.0 - (pow(X, 2.0) * C) / r0l;
    double g = time - (pow(X, 3.0) * S) / sqrt(gravitationalParameter);

    double fd = (sqrt(gravitationalParameter) / (r * r0l)) * X * (p * S - 1.0);
    double gd = 1.0 - (pow(X, 2.0) * C) / r;

    return f * r0 + g * v0;
}

glm::dvec3 keplerUniversal(double time, glm::dvec3 r0, glm::dvec3 v0, double gravitationalParameter)
{
    double r0l = glm::length(r0);
    double v0l = glm::length(v0);

    double a = (2.0 / r0l) - (pow(v0l, 2.0) / gravitationalParameter);

    double X  = sqrt(gravitationalParameter) * time * a;
    double Xp = 0.0;

    double C, S, r;
    while (abs(X - Xp) > epsilon)
    {
        double p = a * pow(X, 2);

        if (p > epsilon)
            C = (1.0 - cos(sqrt(p))) / p;
        else if (p < -epsilon)
            C = (cosh(sqrt(-p)) - 1.0) / -p;
        else
            C = 1.0 / 2.0;

        if (p > epsilon)
            S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
        else if (p < -epsilon)
            S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
        else
            S = 1.0 / 6.0;

        double r_1 = pow(X, 2.0) * C;
        double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
        r          = r_1 + r_2 + r0l * (1.0 - p * C);

        double temp1 = sqrt(gravitationalParameter) * time;
        double temp2 = pow(X, 3.0) * S;
        double temp3 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * pow(X, 2.0) * C;
        double temp4 = r0l * X * (1.0 - p * S);

        Xp = X;
        X  = Xp + (temp1 - temp2 - temp3 - temp4) / r;
    }

    double p = a * pow(X, 2);

    if (p > epsilon)
        C = (1.0 - cos(sqrt(p))) / p;
    else if (p < -epsilon)
        C = (cosh(sqrt(-p)) - 1.0) / -p;
    else
        C = 1.0 / 2.0;

    if (p > epsilon)
        S = (sqrt(p) - sin(sqrt(p))) / pow(sqrt(p), 3.0);
    else if (p < -epsilon)
        S = (sinh(sqrt(-p)) - sqrt(-p)) / pow(sqrt(-p), 3.0);
    else
        S = 1.0 / 6.0;

    double r_1 = pow(X, 2.0) * C;
    double r_2 = (glm::dot(r0, v0) / sqrt(gravitationalParameter)) * X * (1.0 - p * S);
    r          = r_1 + r_2 + r0l * (1.0 - p * C);

    double f = 1.0 - (pow(X, 2.0) * C) / r0l;
    double g = time - (pow(X, 3.0) * S) / sqrt(gravitationalParameter);

    double fd = (sqrt(gravitationalParameter) / (r * r0l)) * X * (p * S - 1.0);
    double gd = 1.0 - (pow(X, 2.0) * C) / r;

    return f * r0 + g * v0;
}

LambertOutput lambertSolver(OrbitalObject rocketOrbital, OrbitalObject moonOrbital, double departureTime, double timeOfFlight, double gravitationalParameter, glm::dvec3 rocketDeparturePosition, bool ubisection=false)
{
    double arrivalTime = departureTime + timeOfFlight;

    glm::dvec3 arrivalMoonState = keplerUniversal(arrivalTime, moonOrbital.state, gravitationalParameter);
    //arrivalMoonState += 616000000.0 * -glm::normalize(arrivalMoonState);

    glm::dvec3 r1 = rocketDeparturePosition;
    glm::dvec3 r2 = arrivalMoonState;

    glm::dvec3 h = glm::cross(rocketOrbital.state.r, rocketOrbital.state.v);

    double r1l = glm::length(r1);
    double r2l = glm::length(r2);

    double cosTheta = glm::dot(r1, r2) / (r1l * r2l);
    double A        = (h.y / abs(h.y)) * sqrt(r1l * r2l * (1 + cosTheta));

    if (A == 0.0)
        return LambertOutput();//glm::dvec3(INFINITY);

    double z = 0.0;
    double S, C, T, y = 0.0;
    while (abs(T - timeOfFlight) > epsilon)
    {
        if (z > epsilon)
            C = (1.0 - cos(sqrt(z))) / z;
        else if (z < -epsilon)
            C = (cosh(sqrt(-z)) - 1.0) / -z;
        else
            C = 1.0 / 2.0;

        if (z > epsilon)
            S = (sqrt(z) - sin(sqrt(z))) / pow(sqrt(z), 3.0);
        else if (z < -epsilon)
            S = (sinh(sqrt(-z)) - sqrt(-z)) / pow(sqrt(-z), 3.0);
        else
            S = 1.0 / 6.0;

        double Cs, Ss;
        if (z > epsilon || z < -epsilon)
            Cs = (1.0 - z * S - 2.0 * C) / (2.0 * z);
        else
            Cs = -1.0 / 24.0;

        if (z > epsilon || z < -epsilon)
            Ss = (C - 3.0 * S) / (2.0 * z);
        else
            Ss = -1.0 / 120.0;

        y = r1l + r2l + A * ((z * S - 1.0) / sqrt(C));
        T = (1.0 / sqrt(gravitationalParameter)) * (pow(y / C, 3.0 / 2.0) * S + A * sqrt(y));

        double ys1 = sqrt(C) * (S + z * Ss);
        double ys2 = (Cs * (z * S - 1.0)) / (2.0 * sqrt(C));
        double ys  = A * ((ys1 - ys2) / C);

        double temp1 = 3.0 / 2.0 * sqrt(y / C);
        double temp2 = ((ys * C - y * Cs) / pow(C, 2.0)) * S;
        double temp3 = sqrt(pow(y / C, 3.0)) * Ss;
        double temp4 = (A / 2.0) * (ys / sqrt(y));

        //std::cout << "temp " << temp1 << "; " << temp2 << "; " << temp3 << "; z " << z << "; y " << y << "; ys " << ys << "; C " << C << "; S " << S << "; Cs " << Cs << "; Ss " << Ss << std::endl;
        z -= (T - timeOfFlight) / ((1.0 / sqrt(gravitationalParameter)) * (temp1 * temp2 + temp3 + temp4));
    }

    if (ubisection)
        z = bisection(-4.0 * pow(PI, 2.0), 4.0 * pow(PI, 2.0), timeOfFlight, r1l, r2l, gravitationalParameter, A);

    if (z > 0.0)
        C = (1.0 - cos(sqrt(z))) / z;
    else if (z < 0.0)
        C = (cosh(sqrt(-z)) - 1.0) / -z;
    else
        C = 1.0 / 2.0;

    if (z > 0.0)
        S = (sqrt(z) - sin(sqrt(z))) / pow(sqrt(z), 3.0);
    else if (z < 0.0)
        S = (sinh(sqrt(-z)) - sqrt(-z)) / pow(sqrt(-z), 3.0);
    else
        S = 1.0 / 6.0;

    y = r1l + r2l + A * ((z * S - 1.0) / sqrt(C));

    double f  = 1.0 - (y / r1l);
    double g  = A * sqrt(y / gravitationalParameter);
    double gd = 1.0 - (y / r2l); // currently unused; for future purposes

    LambertOutput output;
    output.requiredVelocity = (r2 - f * r1) / g;
    output.velocity         = glm::length(output.requiredVelocity);
    output.A = A;
    output.z = z;
    output.y = y;
    output.f = f;
    output.g = g;
    output.r1l = r1l;
    output.r2l = r2l;
    output.r1 = r1;
    output.r2 = r2;
    return output;
}

OrbitalObject findOrbitalElements(glm::dvec3 rocketPosition, glm::dvec3 rocketVelocity, double gravitationalParameter)
{
    glm::dvec3 angularMomentum = glm::cross(rocketPosition, rocketVelocity);

    double rpm = glm::length(rocketPosition); // rpm = rocket position magnitude: not to confuse with RPM (rotations per minute or revolutions per minute)
    double rvm = glm::length(rocketVelocity);

    glm::dvec3 eccentricityVector = glm::cross(rocketVelocity, angularMomentum) / gravitationalParameter - glm::normalize(rocketPosition);
    double eccentricity           = glm::length(eccentricityVector);

    double mechanicalEnergy = pow(rvm, 2.0) / 2.0 - gravitationalParameter / rpm;
    double semiMajorAxis    = -(gravitationalParameter / (2.0 * mechanicalEnergy));
    double semiMinorAxis    = semiMajorAxis * sqrt(1.0 - pow(eccentricity, 2.0));

    double cosTrueAnomaly = glm::dot(eccentricityVector, rocketPosition) / (eccentricity * rpm);
    double trueAnomaly;
    if (glm::dot(rocketPosition, rocketVelocity) >= 0.0)
        trueAnomaly = acos(cosTrueAnomaly);
    else
        trueAnomaly = 2.0 * PI - acos(cosTrueAnomaly);

    double tanEccentricAnomalyBy2 = std::sqrt((1.0 - eccentricity) / (1.0 + eccentricity)) * tan(trueAnomaly / 2.0);
    double eccentricAnomaly       = 2.0 * atan2(std::sqrt(1.0 - eccentricity) * sin(trueAnomaly / 2.0), std::sqrt(1.0 + eccentricity) * cos(trueAnomaly / 2.0));

    double meanAnomaly = eccentricAnomaly - sin(eccentricAnomaly);
    double meanMotion  = sqrt(gravitationalParameter / pow(semiMajorAxis, 3.0));

    double timeOfPeriapsisPassage = -(meanAnomaly / meanMotion);

    glm::dvec3 nodeVector      = glm::cross(glm::dvec3(0.0, 1.0, 0.0), angularMomentum);
    double inclination         = acos(angularMomentum.y / glm::length(angularMomentum));
    double raan                = acos(nodeVector.x      / glm::length(nodeVector));
    double argumentOfPeriapsis = acos(glm::dot(nodeVector, eccentricityVector) / (glm::length(nodeVector) * eccentricity));

    if (nodeVector.z > 0.0)
        raan = 2.0 * PI - raan;

    Orbit result1;
    result1.semiMajorAxis       = semiMajorAxis;
    result1.semiMinorAxis       = semiMinorAxis;
    result1.eccentricity        = eccentricity;
    result1.inclination         = inclination;
    result1.raan                = raan;
    result1.argumentOfPeriapsis = argumentOfPeriapsis;
    result1.meanMotion          = meanMotion;

    ObjectOrbitalState result2;
    result2.meanAnomaly            = meanAnomaly;
    result2.trueAnomaly            = trueAnomaly;
    result2.timeOfPeriapsisPassage = timeOfPeriapsisPassage;

    OrbitalObject result;
    result.orbit        = result1;
    result.orbitalState = result2;
    result.state        = ObjectState(rocketPosition, rocketVelocity);

    return result;
}

OrbitalObject findOrbitalElements(ObjectState rocketState, double gravitationalParameter)
{
    glm::dvec3 rocketPosition = rocketState.r;
    glm::dvec3 rocketVelocity = rocketState.v;

    glm::dvec3 angularMomentum = glm::cross(rocketPosition, rocketVelocity);

    double rpm = glm::length(rocketPosition); // rpm = rocket position magnitude: not to confuse with RPM (rotations per minute or revolutions per minute)
    double rvm = glm::length(rocketVelocity);

    glm::dvec3 eccentricityVector = glm::cross(rocketVelocity, angularMomentum) / gravitationalParameter - glm::normalize(rocketPosition);
    double eccentricity           = glm::length(eccentricityVector);

    double mechanicalEnergy = std::pow(rvm, 2) / 2 - gravitationalParameter / rpm;
    double semiMajorAxis    = -(gravitationalParameter / (2 * mechanicalEnergy));
    double semiMinorAxis    = semiMajorAxis * sqrt(1 - pow(eccentricity, 2));

    double cosTrueAnomaly = glm::dot(eccentricityVector, rocketPosition) / (eccentricity * rpm);
    double trueAnomaly;
    if (glm::dot(rocketPosition, rocketVelocity) >= 0.0)
        trueAnomaly = acos(cosTrueAnomaly);
    else
        trueAnomaly = 2 * PI - acos(cosTrueAnomaly);

    double tanEccentricAnomalyBy2 = std::sqrt((1 - eccentricity) / (1 + eccentricity)) * tan(trueAnomaly / 2);
    double eccentricAnomaly       = 2 * atan2(std::sqrt(1 - eccentricity) * sin(trueAnomaly / 2), std::sqrt(1 + eccentricity) * cos(trueAnomaly / 2));

    double meanAnomaly = eccentricAnomaly - sin(eccentricAnomaly);
    double meanMotion  = sqrt(gravitationalParameter / pow(semiMajorAxis, 3));

    double timeOfPeriapsisPassage = -(meanAnomaly / meanMotion);

    glm::dvec3 nodeVector      = glm::cross(glm::dvec3(0.0, 1.0, 0.0), angularMomentum);
    double inclination         = acos(angularMomentum.y / glm::length(angularMomentum));
    double raan                = acos(nodeVector.x      / glm::length(nodeVector));
    double argumentOfPeriapsis = acos(glm::dot(nodeVector, eccentricityVector) / (glm::length(nodeVector) * eccentricity));

    if (nodeVector.z > 0.0)
        raan = 2 * PI - raan;

    Orbit result1;
    result1.semiMajorAxis       = semiMajorAxis;
    result1.semiMinorAxis       = semiMinorAxis;
    result1.eccentricity        = eccentricity;
    result1.inclination         = inclination;
    result1.raan                = raan;
    result1.argumentOfPeriapsis = argumentOfPeriapsis;
    result1.meanMotion          = meanMotion;

    ObjectOrbitalState result2;
    result2.meanAnomaly            = meanAnomaly;
    result2.trueAnomaly            = trueAnomaly;
    result2.timeOfPeriapsisPassage = timeOfPeriapsisPassage;

    OrbitalObject result;
    result.orbit        = result1;
    result.orbitalState = result2;
    result.state        = ObjectState(rocketPosition, rocketVelocity);

    return result;
}

Transfer findTransferWindow(double x_min, double x_max, double y_min, double y_max, double x_step, double y_step, OrbitalObject rocketOrbital, OrbitalObject targetOrbital, double gravitationalParameter)
{
    double deltaVelocity = INFINITY;
    double departureTime;
    double timeOfFlight;
    glm::dvec3 velVector;

    LambertOutput finalo;

    std::vector<glm::dvec3> rocketDeparturePositions(x_max);

    for (double i = x_min; i < 1.0/*x_max*/; i += 1.0)//x_step)
        rocketDeparturePositions[i] = findMeanPosition(i, rocketOrbital.state, gravitationalParameter);

    for (double i = y_min; i < y_max; i += y_step)
    {
        for (double j = x_min; j < 1.0/*x_max*/; j += 1.0)//x_step)
        {
            //std::cout << "x: " << j << "; y: " << i << std::endl;
            //std::cout << rocketDeparturePositions[j] << "; " << gravitationalParameter << std::endl;
            LambertOutput temp = lambertSolver(rocketOrbital, targetOrbital, 3600 * j, 3600 * i, gravitationalParameter, rocketDeparturePositions[j]);
            glm::dvec3 velocityVector = temp.requiredVelocity;//lambertSolver(rocketOrbital, targetOrbital, 3600 * j, 3600 * i, gravitationalParameter, rocketDeparturePositions[j]).requiredVelocity;
            double vel = glm::length(velocityVector - rocketOrbital.state.v);
            //std::cout << vel << "; " << velocityVector << "; " << rocketOrbital.state.v << std::endl;

            if (vel < deltaVelocity)
            {
                deltaVelocity = vel;

                departureTime = 3600 * j;
                timeOfFlight  = 3600 * i;
                velVector     = velocityVector - rocketOrbital.state.v;
                finalo = temp;
            }
        }
    }

    Transfer transfer;
    transfer.deltaVelocity  = deltaVelocity;
    transfer.departureTime  = departureTime;
    transfer.timeOfFlight   = timeOfFlight;
    transfer.velocityVector = velVector;
    transfer.burnDirection  = glm::normalize(velVector);
    transfer.out = finalo;

    return transfer;
}