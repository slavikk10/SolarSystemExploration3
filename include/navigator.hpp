#include <iostream>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <functionsupport.hpp>
#include <constants.hpp>

#pragma once

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

struct Transfer
{
    double deltaVelocity;
    double departureTime;
    double timeOfFlight;

    glm::dvec3 burnDirection;
    glm::dvec3 velocityVector;
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

glm::dvec3 findMeanPosition(double time, OrbitalObject rocketOrbital, double gravitationalParameter)
{
    double r             = glm::length(rocketOrbital.state.r);
    double v             = glm::length(rocketOrbital.state.v);

    double E             = pow(v, 2) / 2.0 - gravitationalParameter / r;
        
    double a             = -gravitationalParameter / (2.0 * E);
    double T             = 2.0 * PI * sqrt(pow(a, 3) / gravitationalParameter);
    double simulatedTime = 0.0;

    glm::dvec3 h         = glm::cross(rocketOrbital.state.r, rocketOrbital.state.v);

    glm::dvec3 eVec      = glm::cross(rocketOrbital.state.v, h) / gravitationalParameter - glm::normalize(rocketOrbital.state.r);
    double eccentricity  = glm::length(eVec);
    double b             = a * sqrt(1 - pow(eccentricity, 2));

    double M = rocketOrbital.orbit.meanMotion * (time - rocketOrbital.orbitalState.timeOfPeriapsisPassage);

    double E_anom = M;
    for (unsigned int i = 0; i < 5; i++)
        E_anom += ((M - E_anom + eccentricity * sin(E_anom)) / (1 - eccentricity * cos(E_anom)));

    glm::dvec3 eNorm        = glm::normalize(eVec);
    glm::dvec3 orbitalPlane = glm::normalize(glm::cross(h, eNorm));

    return (a * cos(E_anom) - a * eccentricity) * eNorm + b * sin(E_anom) * orbitalPlane;
}

glm::dvec3 lambertSolver(OrbitalObject rocketOrbital, OrbitalObject moonOrbital, double departureTime, double timeOfFlight, double gravitationalParameter, glm::dvec3 rocketDeparturePosition)
{
    double arrivalTime = departureTime + timeOfFlight;

    glm::dvec3 arrivalMoonState = findMeanPosition(arrivalTime, moonOrbital, gravitationalParameter);
    arrivalMoonState += 66100000.0 * -glm::normalize(arrivalMoonState);

    glm::dvec3 r1 = rocketDeparturePosition;
    glm::dvec3 r2 = arrivalMoonState;

    glm::dvec3 h = rocketOrbital.state.r * rocketOrbital.state.v;

    double r1l = glm::length(r1);
    double r2l = glm::length(r2);

    double cosTheta = glm::dot(r1, r2) / (r1l * r2l);
    double A        = (h.y / abs(h.y)) * sqrt(r1l * r2l * (1 + cosTheta));

    if (A == 0.0)
        return glm::dvec3(INFINITY);

    double z = 13.2;
    double S = 0.0;
    double C = 0.0;
    for (unsigned int i = 0; i < 5; i++)
    {
        if (z > 0.0)
            C = (1 - cos(sqrt(z))) / z;
        else if (z < 0.0)
            C = (cosh(sqrt(-z)) - 1) / -z;
        else
            C = 1/2;

        if (z > 0.0)
            S = (sqrt(z) - sin(sqrt(z))) / pow(sqrt(z), 3);
        else if (z < 0.0)
            S = (sinh(sqrt(-z)) - sqrt(-z)) / pow(sqrt(-z), 3);
        else
            S = 1/6;

        double Cs = (1 - z * S - 2 * C) / (2 * z);
        double Ss = (C - 3 * S)         / (2 * z);
        double ys = A * (((S + z * Ss) * sqrt(C) - (z * S - 1) * Cs / (2 * sqrt(C))) / C);

        double y = r1l + r2l + A * ((z * S - 1) / sqrt(C));
        double T = (1 / sqrt(gravitationalParameter)) * (pow(y/C, 3/2) * S + A * sqrt(y));

        double temp1 = (3/2 * sqrt(y/C)) * ((ys * C - y * Cs) / pow(C, 2)) * S;
        double temp2 = sqrt(pow(y/C, 3)) * Ss;
        double temp3 = (A/2) * (ys/sqrt(y));

        z -= (T - timeOfFlight) / ((1 / sqrt(gravitationalParameter)) * (temp1 + temp2 + temp3));
    }

    if (z > 0.0)
        C = (1 - cos(sqrt(z))) / z;
    else if (z < 0.0)
        C = (cosh(sqrt(-z)) - 1) / -z;
    else
        C = 1/2;

    if (z > 0.0)
        S = (sqrt(z) - sin(sqrt(z))) / pow(sqrt(z), 3);
    else if (z < 0.0)
        S = (sinh(sqrt(-z)) - sqrt(-z)) / pow(sqrt(-z), 3);
    else
        S = 1/6;

    double ya = r1l + r2l + A * ((z * S - 1) / sqrt(C));
    double f  = 1 - (ya / r1l);
    double g  = A * sqrt(ya / gravitationalParameter);
    double gd = 1 - (ya / r2l); // currently unused; for future purposes

    glm::dvec3 requiredVelocity = (r2 - f * r1) / g;
    return requiredVelocity;
}

OrbitalObject findOrbitalElements(glm::dvec3 rocketPosition, glm::dvec3 rocketVelocity, double gravitationalParameter)
{
    glm::dvec3 angularMomentum = glm::cross(rocketPosition, rocketVelocity);

    double rpm = glm::length(rocketPosition); // rpm = rocket position magnitude: not to confuse with RPM (rotations per minute or revolutions per minute)
    double rvm = glm::length(rocketVelocity);

    glm::dvec3 eccentricityVector = glm::cross(rocketVelocity, angularMomentum) / gravitationalParameter - glm::normalize(rocketPosition);
    double eccentricity           = glm::length(eccentricityVector);

    double mechanicalEnergy = std::pow(rvm, 2) / 2 - gravitationalParameter / rpm;
    double semiMajorAxis    = -(gravitationalParameter / (2 * mechanicalEnergy));

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
    result1.eccentricity        = eccentricity;
    result1.inclination         = inclination;
    result1.raan                = raan;
    result1.argumentOfPeriapsis = argumentOfPeriapsis;

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

    std::vector<glm::dvec3> rocketDeparturePositions(x_max);

    for (double i = x_min; i < x_max; i += x_step)
        rocketDeparturePositions[i] = findMeanPosition(i, rocketOrbital, gravitationalParameter);

    for (double i = y_min; i < y_max; i += y_step)
    {
        for (double j = x_min; j < x_max; j += x_step)
        {
            glm::dvec3 velocityVector = lambertSolver(rocketOrbital, targetOrbital, j, i, gravitationalParameter, rocketDeparturePositions[j]);
            double vel = glm::length(velocityVector - rocketOrbital.state.v);

            if (vel < deltaVelocity)
            {
                deltaVelocity = vel;

                departureTime = j;
                timeOfFlight  = i;
                velVector     = velocityVector - rocketOrbital.state.v;
            }
        }
    }

    Transfer transfer;
    transfer.deltaVelocity  = deltaVelocity;
    transfer.departureTime  = departureTime;
    transfer.timeOfFlight   = timeOfFlight;
    transfer.velocityVector = velVector;
    transfer.burnDirection  = glm::normalize(velVector);

    return transfer;
}