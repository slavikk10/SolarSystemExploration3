#include <math.h>
#include <string>
#include <format>

#pragma once

const std::vector<std::string> monthNames = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// date class
class Date 
{
public:
    unsigned int year   = 2026;
    unsigned int month  = 6;
    unsigned int day    = 1;
    unsigned int hour   = 0;
    unsigned int minute = 0;
    float        second = 0.0f;
    std::string  date   = "00:00:00, Jun 1, 2026";

    Date() {}

    void increment(float seconds)
    {
        second += seconds;

        if (second >= 60.0f)
        {
            minute += floor(second / 60.0f);
            second = 0.0f;
        }

        if (minute >= 60)
        {
            hour += floor(minute / 60);
            minute = 0;
        }

        if (hour >= 24)
        {
            day += floor(hour / 24);
            hour = 0;
        }

        if (day >= 32)
        {
            month += floor(day / 32);
            day = 1;
        }

        if (month >= 13)
        {
            year += floor(month / 13);
            month = 1;
        }

        date = std::format("{:02}", hour) + ":" + std::format("{:02}", minute) + ":" + std::format("{:02}", floor(second)) + ", " + monthNames[month - 1] + " " + std::to_string(day) + ", " + std::to_string(year);
    }
};