#include <Wire.h>

struct {
  char Year;
  char Month;
  char Date;
  char Day;
  char Hour;
  char Min;
  char Sec;

  char TempYear;
  char TempMonth;
  char TempDate;
  char TempDay;
  char TempHour;
  char TempMin;
  char TempSec;
} DS3231_Data;

void DS3231_ReadData() {
  Wire.beginTransmission(0x68);
  Wire.write(0x00); //Set DS3231 REG Pointer
  Wire.endTransmission();
  Wire.requestFrom(0x68, 7, 0);
  if (Wire.available() == 7) {
    DS3231_Data.Sec = Wire.read();
    DS3231_Data.Min = Wire.read();
    DS3231_Data.Hour = Wire.read();
    DS3231_Data.Day = Wire.read();
    DS3231_Data.Date = Wire.read();
    DS3231_Data.Month = Wire.read();
    DS3231_Data.Year = Wire.read();
    DS3231_Data.Sec = (DS3231_Data.Sec / 16) * 10 + (DS3231_Data.Sec % 16);
    DS3231_Data.Min = (DS3231_Data.Min / 16) * 10 + (DS3231_Data.Min % 16);
    DS3231_Data.Hour = (DS3231_Data.Hour / 16) * 10 + (DS3231_Data.Hour % 16);
    DS3231_Data.Date = (DS3231_Data.Date / 16) * 10 + (DS3231_Data.Date % 16);
  }
  else { //Clear Buffer
    while (Wire.available()) Wire.read();
  }
}

void DS3231_WriteData() {
  Wire.beginTransmission(0x68);
  Wire.write(0x00); //Set DS3231 REG Pointer
  DS3231_Data.TempSec = (DS3231_Data.TempSec / 10) * 16 + (DS3231_Data.TempSec % 10);
  DS3231_Data.TempMin = (DS3231_Data.TempMin / 10) * 16 + (DS3231_Data.TempMin % 10);
  DS3231_Data.TempHour = (DS3231_Data.TempHour / 10) * 16 + (DS3231_Data.TempHour % 10);
  DS3231_Data.TempDate = (DS3231_Data.TempDate / 10) * 16 + (DS3231_Data.TempDate % 10);
  Wire.write(DS3231_Data.TempSec);
  Wire.write(DS3231_Data.TempMin);
  Wire.write(DS3231_Data.TempHour);
  Wire.write(DS3231_Data.TempDay);
  Wire.write(DS3231_Data.TempDate);
  Wire.write(DS3231_Data.TempMonth);
  Wire.write(DS3231_Data.TempYear);
  Wire.endTransmission();
}