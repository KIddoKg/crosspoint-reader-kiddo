#include "LunarCalendar.h"

#include <cmath>

// Port of Ho Ngoc Duc's "Am lich Viet Nam" algorithm (https://www.informatik.uni-leipzig.de/~duc/amlich/),
// which derives new moons and solar terms astronomically. Time zone is fixed at UTC+7 (Vietnam).
namespace {
constexpr double TIME_ZONE = 7.0;
constexpr double PI = 3.14159265358979323846;

long jdFromDate(const int dd, const int mm, const int yy) {
  const long a = (14 - mm) / 12;
  const long y = yy + 4800 - a;
  const long m = mm + 12 * a - 3;
  long jd = dd + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
  if (jd < 2299161) {
    jd = dd + (153 * m + 2) / 5 + 365 * y + y / 4 - 32083;
  }
  return jd;
}

// Julian day number of the k-th new moon after 1900-01-01
long newMoonDay(const long k) {
  const double T = k / 1236.85;
  const double T2 = T * T;
  const double T3 = T2 * T;
  const double dr = PI / 180;
  double jd1 = 2415020.75933 + 29.53058868 * k + 0.0001178 * T2 - 0.000000155 * T3;
  jd1 += 0.00033 * std::sin((166.56 + 132.87 * T - 0.009173 * T2) * dr);
  const double M = 359.2242 + 29.10535608 * k - 0.0000333 * T2 - 0.00000347 * T3;
  const double Mpr = 306.0253 + 385.81691806 * k + 0.0107306 * T2 + 0.00001236 * T3;
  const double F = 21.2964 + 390.67050646 * k - 0.0016528 * T2 - 0.00000239 * T3;
  double C1 = (0.1734 - 0.000393 * T) * std::sin(M * dr) + 0.0021 * std::sin(2 * dr * M);
  C1 = C1 - 0.4068 * std::sin(Mpr * dr) + 0.0161 * std::sin(dr * 2 * Mpr);
  C1 = C1 - 0.0004 * std::sin(dr * 3 * Mpr);
  C1 = C1 + 0.0104 * std::sin(dr * 2 * F) - 0.0051 * std::sin(dr * (M + Mpr));
  C1 = C1 - 0.0074 * std::sin(dr * (M - Mpr)) + 0.0004 * std::sin(dr * (2 * F + M));
  C1 = C1 - 0.0004 * std::sin(dr * (2 * F - M)) - 0.0006 * std::sin(dr * (2 * F + Mpr));
  C1 = C1 + 0.0010 * std::sin(dr * (2 * F - Mpr)) + 0.0005 * std::sin(dr * (2 * Mpr + M));
  double deltat;
  if (T < -11) {
    deltat = 0.001 + 0.000839 * T + 0.0002261 * T2 - 0.00000845 * T3 - 0.000000081 * T * T3;
  } else {
    deltat = -0.000278 + 0.000265 * T + 0.000262 * T2;
  }
  const double jdNew = jd1 + C1 - deltat;
  return static_cast<long>(std::floor(jdNew + 0.5 + TIME_ZONE / 24));
}

// Sun longitude sector (0..11, each 30 degrees) at the start of the given Julian day
int sunLongitude(const long jdn) {
  const double T = (jdn - 2451545.5 - TIME_ZONE / 24) / 36525;
  const double T2 = T * T;
  const double dr = PI / 180;
  const double M = 357.52910 + 35999.05030 * T - 0.0001559 * T2 - 0.00000048 * T * T2;
  const double L0 = 280.46645 + 36000.76983 * T + 0.0003032 * T2;
  double DL = (1.914600 - 0.004817 * T - 0.000014 * T2) * std::sin(dr * M);
  DL = DL + (0.019993 - 0.000101 * T) * std::sin(dr * 2 * M) + 0.000290 * std::sin(dr * 3 * M);
  double L = (L0 + DL) * dr;
  L = L - PI * 2 * std::floor(L / (PI * 2));
  return static_cast<int>(std::floor(L / PI * 6));
}

long lunarMonth11(const int yy) {
  const long off = jdFromDate(31, 12, yy) - 2415021;
  const long k = static_cast<long>(std::floor(off / 29.530588853));
  long nm = newMoonDay(k);
  if (sunLongitude(nm) >= 9) {
    nm = newMoonDay(k - 1);
  }
  return nm;
}

int leapMonthOffset(const long a11) {
  const long k = static_cast<long>(std::floor((a11 - 2415021.076998695) / 29.530588853 + 0.5));
  int last;
  int i = 1;
  int arc = sunLongitude(newMoonDay(k + i));
  do {
    last = arc;
    i++;
    arc = sunLongitude(newMoonDay(k + i));
  } while (arc != last && i < 14);
  return i - 1;
}
}  // namespace

namespace LunarCalendar {

LunarDate fromSolar(const int dd, const int mm, const int yy) {
  const long dayNumber = jdFromDate(dd, mm, yy);
  const long k = static_cast<long>(std::floor((dayNumber - 2415021.076998695) / 29.530588853));
  long monthStart = newMoonDay(k + 1);
  if (monthStart > dayNumber) {
    monthStart = newMoonDay(k);
  }
  long a11 = lunarMonth11(yy);
  long b11 = a11;
  int lunarYear;
  if (a11 >= monthStart) {
    lunarYear = yy;
    a11 = lunarMonth11(yy - 1);
  } else {
    lunarYear = yy + 1;
    b11 = lunarMonth11(yy + 1);
  }
  const int lunarDay = static_cast<int>(dayNumber - monthStart + 1);
  const int diff = static_cast<int>((monthStart - a11) / 29);
  bool lunarLeap = false;
  int lunarMonth = diff + 11;
  if (b11 - a11 > 365) {
    const int leapMonthDiff = leapMonthOffset(a11);
    if (diff >= leapMonthDiff) {
      lunarMonth = diff + 10;
      if (diff == leapMonthDiff) {
        lunarLeap = true;
      }
    }
  }
  if (lunarMonth > 12) {
    lunarMonth -= 12;
  }
  if (lunarMonth >= 11 && diff < 4) {
    lunarYear -= 1;
  }
  return {lunarDay, lunarMonth, lunarYear, lunarLeap};
}

}  // namespace LunarCalendar
