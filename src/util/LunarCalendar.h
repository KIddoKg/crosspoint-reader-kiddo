#pragma once

// Vietnamese lunar calendar conversion (Ho Ngoc Duc's astronomical algorithm, UTC+7), computed offline.
namespace LunarCalendar {

struct LunarDate {
  int day;
  int month;
  int year;
  bool leapMonth;
};

// Converts a Gregorian date (day 1-31, month 1-12) to the Vietnamese lunar date
LunarDate fromSolar(int day, int month, int year);

}  // namespace LunarCalendar
