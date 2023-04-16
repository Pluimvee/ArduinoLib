#include <Clock.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////////////////
#define TZ_DST_AMSTERDAM "CET-1CEST,M3.5.0/2,M10.5.0/3"

////////////////////////////////////////////////////////////////////////////////////////////
// Sync the RTC_Millis using NTP and TZ and DST for Amsterdam
////////////////////////////////////////////////////////////////////////////////////////////
bool Clock::ntp_sync()
{
  configTime(TZ_DST_AMSTERDAM, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");

  struct tm tmstruct;
  int retry=0;
  while (getLocalTime(&tmstruct) == false) {
    if (retry++ > 10)
      return false;
    delay(500);
  }
  // localtime is based on 1970 while class DateTime is based on year 2000
  adjust(DateTime(tmstruct.tm_year-100, tmstruct.tm_mon+1, tmstruct.tm_mday, tmstruct.tm_hour, tmstruct.tm_min, tmstruct.tm_sec));
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
