//BlackCat32 20170208 Ver_1.0
#include <avr/pgmspace.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>

//HardSet
#define TimePlus_Pin 13 //沒用到
#define WaterDetector_Pin A0 //加水位感測器
#define Servo_Pin 7
#define Pump_Pin 10 //(5)
#define Alarm_Pin 11 //(6)
#define EncoderA_Pin 4 //(2)
#define EncoderB_Pin 5 //(3)
#define EncoderSW_Pin 6 //(4)
#define FanA_Pin 8
#define FanB_Pin 9
#define Speaker_Pin 11
#define TempDamp_Pin 10
//(藍芽)

#define ServoBlindAngle 170 //170度角為實際180角 //(change)

LiquidCrystal_I2C Lcd(0x27, 20, 4); //須注意PCF8574 I2C Address有兩種
Servo MotorArm;

#define ServoPostionSet(Degree) MotorArm.write(map(Degree, 0, 180, 0, ServoBlindAngle)) //每台RCServo角度不一定成比例 //(change)
#define WaterLowAlarm(EN); digitalWrite(Alarm_Pin, !EN) //(change)

//Index
#define ShowState 0

#define SelectSetSystemTime 1
#define SelectSetYear 10
#define SelectSetMonth 11
#define SelectSetDate 12
#define SelectSetHour 13
#define SelectSetMin 14
#define SelectSetSec 15
#define SaveSystemTime 16
#define ExitSystemTime 17

#define SelectSetSchedule 2
#define SetSchedule 20
#define SetTaskDegree 30
#define SetTaskTime 31
#define SetTaskWaterTime 32
#define SetTaskActive 33
#define SetTaskTimeHour 34
#define SetTaskTimeMin 35
#define SaveTask 36
#define ExitTask 37

//List
#define ExitSchedule 0
#define Task01 1
#define Task02 2
#define Task03 3
#define Task04 4
#define Task05 5
#define Task06 6
#define Task07 7
#define Task08 8
#define Task09 9

#define TaskSearch 0
#define TaskMoveArm 1
#define TaskWatering 2
#define TaskReturn 3

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

struct {
  unsigned char WaterTime;
  unsigned char ServoDegree;
  char EnableTimeHour;
  char EnableTimeMin;
  unsigned char Active;
} TaskData;

char* ListStrings[] = {
  "Exit  ",
  "Task01",
  "Task02",
  "Task03",
  "Task04",
  "Task05",
  "Task06",
  "Task07",
  "Task08",
  "Task09"
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

char Encoder_Read (char a_PIN, char b_PIN) { //(int?)
  static char O_A, N_A, N_B;
  O_A = N_A;
  N_A = digitalRead(a_PIN); //(?)
  N_B = digitalRead(b_PIN);
  if (O_A && !N_A) { //(O_A==1 && N_A==0)
    if (N_B) { //N_B==1
      return 1;
    }
    else {
      return -1;
    }
  }
  if (!O_A && N_A) { //(O_A==0 && N_A==1)
    if (!N_B) { //N_B==0
      return 1;
    }
    else {
      return -1;
    }
  }
  return 0;
}

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  char i;
  if (EEPROM.read(0) == 0xFF) { //表示第一次開機 記憶初始化
    for (i = 0; i <= 100; i++) {
      EEPROM.update(i, 0x00);
    }
  }
  Wire.begin();
  MotorArm.attach(Servo_Pin);
  Lcd.begin(); //( Lcd.init() )
  Lcd.backlight();
  Lcd.clear();
  pinMode(EncoderA_Pin, INPUT_PULLUP);
  pinMode(EncoderB_Pin, INPUT_PULLUP);
  pinMode(EncoderSW_Pin, INPUT_PULLUP);
  pinMode(Alarm_Pin, OUTPUT);
  pinMode(Pump_Pin, OUTPUT);
  pinMode(TimePlus_Pin, OUTPUT); //(?)
  pinMode(FanA_Pin, OUTPUT);
  pinMode(FanB_Pin, OUTPUT);
  pinMode(Speaker_Pin, OUTPUT);
  pinMode(TempDamp_Pin, OUTPUT);
  //(藍芽)

  digitalWrite(A0, 1); //(change)
  // (A4 A5?)
  digitalWrite(Pump_Pin, 1);
  ServoPostionSet(0); //(?)
  WaterLowAlarm(0); //(?)
}
void loop() {
  static unsigned long SystemTime, ScanTime, IndexReflashTime, BlinkTime, ArmMoveTime, WateringTime, WateringStart, WateringStop;
  static char EC_Flag, EnterFlag, FlashFlag, BlinkFlag, DoubleClickFlag, AlarmFlag, List, Task, NextTask;
  static char TimeFlashFlag, SystemRun, TaskNum;
  static char Old_Sec, Old_Min, Old_Index, Index, TaskOperation;
  static char TaskOrder[10];
  static int TimeValue[10], NowTime;
  char i;
  SystemTime = millis(); //(IMPOTTANT)
  EC_Flag += Encoder_Read(EncoderA_Pin, EncoderB_Pin); //持續刷新Encoder累計量 //(IMPOTTANT)
  if (SystemTime - ScanTime > 10) { //每10毫秒掃描按鍵一次 Debounce
    static char Old_SW, New_SW;
    static unsigned long DoubleClickTime, Old_DoubleClickTime;
    ScanTime += 10;
    Old_SW = New_SW;
    New_SW = !digitalRead(EncoderSW_Pin);
    if (!Old_SW && New_SW) {
      EnterFlag = 1; //正源觸發
      Old_DoubleClickTime = DoubleClickTime;
      DoubleClickTime = SystemTime;
      if (DoubleClickTime - Old_DoubleClickTime < 500) DoubleClickFlag = 1; //新舊正源觸發時距小於0.5秒 表示有雙擊 //(雙擊可以?)
    }
  }
  if (SystemTime - IndexReflashTime > 100) { //每0.1秒更新畫面 與更新系統時間
    IndexReflashTime += 100;
    Old_Sec = DS3231_Data.Sec;
    Old_Min = DS3231_Data.Min;
    DS3231_ReadData();
    if ((analogRead(WaterDetector_Pin) > 512) && SystemRun) { //偵測水位臨界 //(change)
      AlarmFlag = 1;
    }
    else {
      AlarmFlag = 0;
    }
    if (AlarmFlag) {
      static char EN;
      if (DS3231_Data.Sec != Old_Sec) {
        EN = !EN
             WaterLowAlarm(EN);
      }
    }
    else {
      WaterLowAlarm(0);
    }
    Old_Index = Index;
    if (EnterFlag) { //有按鍵反應 選單進出 //(?)
      EC_Flag = 0;
      switch (Index) { //主頁 停用或啟用
        case ShowState:
          SystemRun = !SystemRun;
          if (SystemRun) {
            TaskNum = 0; //清除 重新計算任務數
            for (i = 1; i <= 9; i++) { //重新計算從當天從00:00開始的分鐘指標
              TimeValue[i] = int(EEPROM.read(i * 10 + 1)) * 60 + int(EEPROM.read(i * 10 + 2));
              if (EEPROM.read(i * 10)) TaskNum++; //有啟動則增加任物顯示數
            }
          }
          break;
      }
      switch (Index) { //時間選單進出
        case SelectSetSystemTime:
          Index = SelectSetYear; //修改Index  進入系統時間 跳到年設定
          break;
        case SelectSetYear: //任一個選項接翻轉旗標
        case SelectSetMonth:
        case SelectSetDate:
        case SelectSetHour:
        case SelectSetMin:
        case SelectSetSec:
          FlashFlag = !FlashFlag;
          if (FlashFlag) BlinkTime = SystemTime;
          break;
        case SaveSystemTime:
          DS3231_WriteData();
        case ExitSystemTime:
          Index = SelectSetSystemTime; //什麼都不做 進入(返回)選單
          break;
      }
      switch (Index) { //排程選單進出
        case SelectSetSchedule:
          Index = SetSchedule;
          BlinkTime = SystemTime;
          FlashFlag = !FlashFlag;
          break;
        case SetSchedule:
          if (List == 0) {
            Index = SelectSetSchedule;
            FlashFlag = !FlashFlag;
          }
          if ((List <= Task09) && (List >= Task01)) { //清單指向
            Index = SetTaskDegree;
            TaskData.Active = EEPROM.read(List * 10);
            TaskData.EnableTimeHour = EEPROM.read(List * 10 + 1);
            TaskData.EnableTimeMin = EEPROM.read(List * 10 + 2);
            TaskData.WaterTime = EEPROM.read(List * 10 + 3);
            TaskData.ServoDegree = EEPROM.read(List * 10 + 4);
            ServoPostionSet(TaskData.ServoDegree);
          }
          break;
      }
      switch (Index) { //任務選單進出
        case SetTaskDegree:
        case SetTaskWaterTime:
        case SetTaskActive:
          FlashFlag = !FlashFlag;
          if (FlashFlag) BlinkTime = SystemTime;
          break;
        case SetTaskTime: //點極就進入調時
          Index = SetTaskTimeHour;
          FlashFlag = 1;
          BlinkTime = SystemTime;
          break;
        case SetTaskTimeHour://在點擊進入調分
          Index = SetTaskTimeMin;
          BlinkTime = SystemTime;
          break;
        case SetTaskTimeMin: //在點擊返回選項
          Index = SetTaskTime;
          FlashFlag = 0;
          break;
        case SaveTask:
          EEPROM.update(List * 10 + 0, TaskData.Active);
          EEPROM.update(List * 10 + 1, TaskData.EnableTimeHour);
          EEPROM.update(List * 10 + 2, TaskData.EnableTimeMin);
          EEPROM.update(List * 10 + 3, TaskData.WaterTime);
          EEPROM.update(List * 10 + 4, TaskData.ServoDegree);
        case ExitTask:
          TaskNum = 0; //清除 重新計算任務數
          for (i = 1; i <= 9; i++) { //重新計算從當天從00:00開始的分鐘指標
            TimeValue[i] = int(EEPROM.read(i * 10 + 1)) * 60 + int(EEPROM.read(i * 10 + 2));
            if (EEPROM.read(i * 10)) TaskNum++; //有啟動則增加任物顯示數
          }
          Index = SetSchedule;
          ServoPostionSet(0);
          FlashFlag = 1; //回到上一頁排程單 恢復閃爍
          BlinkTime = SystemTime;
          break;
      }
    }
    if (EC_Flag) { //有旋轉反應 移動Index或增減數字
      EnterFlag = 0; //同一時間 只能有一個動作
      if (!FlashFlag) {//移動Index
        if (EC_Flag > 0) { //上移
          switch (Index) { //主選單
            case ShowState:
              if (!SystemRun) Index = SelectSetSchedule;
              break;
            case SelectSetSchedule:
              Index = SelectSetSystemTime;
              break;
            case SelectSetSystemTime:
              Index = ShowState;
              break;
          }
          switch (Index) { //系統時鐘
            case SelectSetYear:
              Index = SelectSetMonth;
              break;
            case SelectSetMonth:
              Index = SelectSetDate;
              break;
            case SelectSetDate:
              Index = SelectSetHour;
              break;
            case SelectSetHour:
              Index = SelectSetMin;
              break;
            case SelectSetMin:
              Index = SelectSetSec;
              break;
            case SelectSetSec:
              Index = SaveSystemTime;
              break;
            case SaveSystemTime:
              Index = ExitSystemTime;
              break;
            case ExitSystemTime:
              Index = SelectSetYear;
              break;
          }
          switch (Index) { //任務選單
            case SetTaskDegree:
              Index = ExitTask;
              break;
            case ExitTask:
              Index = SaveTask;
              break;
            case SaveTask:
              Index = SetTaskActive;
              break;
            case SetTaskActive:
              Index = SetTaskWaterTime;
              break;
            case SetTaskWaterTime:
              Index = SetTaskTime;
              break;
            case SetTaskTime:
              Index = SetTaskDegree;
              break;

          }
        }
        if (EC_Flag < 0) { //後退
          switch (Index) { //主選單
            case ShowState:
              if (!SystemRun) Index = SelectSetSystemTime;
              break;
            case SelectSetSystemTime:
              Index = SelectSetSchedule;
              break;
            case SelectSetSchedule:
              Index = ShowState;
              break;
          }
          switch (Index) { //系統時鐘
            case SelectSetYear:
              Index = ExitSystemTime;
              break;
            case ExitSystemTime:
              Index = SaveSystemTime;
              break;
            case SaveSystemTime:
              Index = SelectSetSec;
              break;
            case SelectSetSec:
              Index = SelectSetMin;
              break;
            case SelectSetMin:
              Index = SelectSetHour;
              break;
            case SelectSetHour:
              Index = SelectSetDate;
              break;
            case SelectSetDate:
              Index = SelectSetMonth;
              break;
            case SelectSetMonth:
              Index = SelectSetYear;
              break;
          }
          switch (Index) { //任務選單
            case SetTaskDegree:
              Index = SetTaskTime;
              break;
            case SetTaskTime:
              Index = SetTaskWaterTime;
              break;
            case SetTaskWaterTime:
              Index = SetTaskActive;
              break;
            case SetTaskActive:
              Index = SaveTask;
              break;
            case SaveTask:
              Index = ExitTask;
              break;
            case ExitTask:
              Index = SetTaskDegree;
              break;
          }
        }
      }
      else { //增減數據
        switch (Index) { //調整時鐘數據增減
          case SelectSetYear:
            DS3231_Data.TempYear = (DS3231_Data.TempYear - EC_Flag) % 101; //旋轉方向 正轉為加 但生物直覺則為正轉下拉
            if (DS3231_Data.TempYear < 0) DS3231_Data.TempYear += 101;
            break;
          case SelectSetMonth:
            DS3231_Data.TempMonth -= EC_Flag;
            if (DS3231_Data.TempMonth > 12) DS3231_Data.TempMonth = 1;
            if (DS3231_Data.TempMonth < 1) DS3231_Data.TempMonth = 12;
            break;
          case SelectSetDate:
            DS3231_Data.TempDate -= EC_Flag;
            if (DS3231_Data.TempDate > 31) DS3231_Data.TempDate = 1;
            if (DS3231_Data.TempDate < 1) DS3231_Data.TempDate = 31;
            break;
          case SelectSetHour:
            DS3231_Data.TempHour -= EC_Flag;
            if (DS3231_Data.TempHour < 0) DS3231_Data.TempHour = 23;
            if (DS3231_Data.TempHour > 23) DS3231_Data.TempHour = 0;
            break;
          case SelectSetMin:
            DS3231_Data.TempMin -= EC_Flag;
            if (DS3231_Data.TempMin < 0) DS3231_Data.TempMin = 59;
            if (DS3231_Data.TempMin > 59) DS3231_Data.TempMin = 0;
            break;
          case SelectSetSec:
            DS3231_Data.TempSec -= EC_Flag;
            if (DS3231_Data.TempSec < 0) DS3231_Data.TempSec = 59;
            if (DS3231_Data.TempSec > 59) DS3231_Data.TempSec = 0;
            break;
        }
        switch (Index) { //調整排程選單上下翻轉
          case SetSchedule:
            List += EC_Flag;
            if (List > 0) List = List % 10;
            if (List < 0) List = (List % 10) + 10;
            break;
        }
        switch (Index) { //調整任務數據增減
          case SetTaskDegree:
            if (((int(TaskData.ServoDegree) + EC_Flag) < 180) && ((int(TaskData.ServoDegree) + EC_Flag) > 0)) TaskData.ServoDegree += EC_Flag;
            if ((int(TaskData.ServoDegree) + EC_Flag) > 180)TaskData.ServoDegree = 180;
            if ((int(TaskData.ServoDegree) + EC_Flag) < 0)TaskData.ServoDegree = 0;
            ServoPostionSet(TaskData.ServoDegree);
            break;
          case SetTaskTimeHour:
            TaskData.EnableTimeHour = (TaskData.EnableTimeHour + EC_Flag) % 24;
            if (TaskData.EnableTimeHour < 0) TaskData.EnableTimeHour += 24;
            break;
          case SetTaskTimeMin:
            TaskData.EnableTimeMin = (TaskData.EnableTimeMin + EC_Flag) % 60;
            if (TaskData.EnableTimeMin < 0) TaskData.EnableTimeMin += 60;
            break;
          case SetTaskWaterTime:
            TaskData.WaterTime += EC_Flag;
            break;
          case SetTaskActive:
            TaskData.Active = !TaskData.Active;
            break;
        }
      }
    }
    if (EC_Flag || EnterFlag) { //按鍵更新Index或數據 刷新顯示
      switch (Index) { //主畫面處理及更新
        case ShowState:
          if (Old_Index != ShowState) {
            Lcd.clear();
            Lcd.setCursor(0, 1);
            Lcd.print(2000 + DS3231_Data.Year);
            Lcd.print("/");
            if (DS3231_Data.Month <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Month));
            Lcd.print("/");
            if (DS3231_Data.Date <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Date));
            Lcd.print(" ");
            Lcd.print(" ");
            if (DS3231_Data.Hour <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Hour));
            Lcd.print(":");
            if (DS3231_Data.Min <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Min));
            Lcd.print(":");
            if (DS3231_Data.Sec <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Sec));
          }
          Lcd.setCursor(4, 0);
          if (SystemRun) {
            Lcd.print("Auto Watering");
            //這裡放入下一個排成列表
          }
          else {
            Lcd.print("STOP Watering");

          }
          break;
        case SelectSetSystemTime:
        case SelectSetSchedule:
          if ((Old_Index == ShowState) || (Old_Index == SetSchedule) || (Old_Index == ExitSystemTime)) {
            Lcd.clear();
            Lcd.setCursor(4, 1);
            Lcd.print("SetSystemTime");
            Lcd.setCursor(4, 2);
            Lcd.print("SetSchedule");
            break;
          }
      }
      switch (Index) { //時間主畫面處理
        case SelectSetYear: //進入時間設定 預設從年開始調整
          if (Old_Index == SelectSetSystemTime) { //從設定選單切入
            Lcd.clear();
            Lcd.setCursor(4, 0);
            Lcd.print("SetSystemTime");
            Lcd.setCursor(0, 1);
            Lcd.print(2000 + DS3231_Data.Year);
            Lcd.print("/");
            if (DS3231_Data.Month <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Month));
            Lcd.print("/");
            if (DS3231_Data.Date <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Date));
            Lcd.print(" ");
            Lcd.print(" ");
            if (DS3231_Data.Hour <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Hour));
            Lcd.print(":");
            if (DS3231_Data.Min <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Min));
            Lcd.print(":");
            if (DS3231_Data.Sec <= 9) Lcd.print("0");
            Lcd.print(int(DS3231_Data.Sec));
            Lcd.setCursor(3, 3);
            Lcd.print("Save");
            Lcd.setCursor(13, 3);
            Lcd.print("Exit");
            DS3231_Data.TempYear = DS3231_Data.Year;
            DS3231_Data.TempMonth = DS3231_Data.Month;
            DS3231_Data.TempDate = DS3231_Data.Date;
            DS3231_Data.TempHour = DS3231_Data.Hour;
            DS3231_Data.TempMin = DS3231_Data.Min;
            DS3231_Data.TempSec = DS3231_Data.Sec;
          }
          break;
      }
      switch (Index) { //排程畫面處理
        case SetSchedule:
          if ((Old_Index == SelectSetSchedule) || (Old_Index == ExitTask) || (Old_Index == SaveTask)) {
            Lcd.clear();
            Lcd.setCursor(4, 0);
            Lcd.print("SetSchedule");
            Lcd.setCursor(1, 2);
            Lcd.print("Select");
          }
          Lcd.setCursor(8, 1);
          Lcd.print(ListStrings[(List + 9) % 10]);
          Lcd.setCursor(8, 2);
          Lcd.print(ListStrings[List]);
          Lcd.setCursor(8, 3);
          Lcd.print(ListStrings[(List + 1) % 10]);
          break;
      }
      switch (Index) { //任務畫面 換頁畫面處理
        case SetTaskDegree:
        case SetTaskWaterTime:
          if (Old_Index == SetSchedule) { //首次進入
            Lcd.clear();
            Lcd.setCursor(6, 0);
            Lcd.print(ListStrings[List]);
          }
          if ((Old_Index == ExitTask) || (Old_Index == SetSchedule) || (Old_Index == SetTaskActive)) {
            Lcd.setCursor(0, 1);
            Lcd.print("                    ");
            Lcd.setCursor(0, 2);
            Lcd.print("                    ");
            Lcd.setCursor(0, 3);
            Lcd.print("                    ");
            Lcd.setCursor(1, 1);
            Lcd.print("ArmDegree: ");
            if (int(TaskData.ServoDegree) < 100) Lcd.print(" ");
            if (int(TaskData.ServoDegree) < 10) Lcd.print(" ");
            Lcd.print(int(TaskData.ServoDegree));
            Lcd.setCursor(1, 2);
            Lcd.print("EnableTime: ");
            if (int(TaskData.EnableTimeHour) < 10) Lcd.print("0");
            Lcd.print(int(TaskData.EnableTimeHour));
            Lcd.setCursor(15, 2);
            Lcd.print(":");
            if (int(TaskData.EnableTimeMin) < 10) Lcd.print("0");
            Lcd.print(int(TaskData.EnableTimeMin));
            Lcd.setCursor(1, 3);
            Lcd.print("WaterTime: ");
            if (int(TaskData.WaterTime) < 100) Lcd.print(" ");
            if (int(TaskData.WaterTime) < 10) Lcd.print(" ");
            Lcd.print(int(TaskData.WaterTime));
            Lcd.print(" Sec");
          }
          break;
        case SetTaskActive:
        case ExitTask:
          if ((Old_Index == SetTaskWaterTime) || (Old_Index == SetTaskDegree)) {
            Lcd.setCursor(0, 1);
            Lcd.print("                    ");
            Lcd.setCursor(0, 2);
            Lcd.print("                    ");
            Lcd.setCursor(0, 3);
            Lcd.print("                    ");
            Lcd.setCursor(1, 1);
            Lcd.print("State: ");
            if (TaskData.Active) {
              Lcd.print("Active  ");
            }
            else {
              Lcd.print("DeActive");
            }
            Lcd.setCursor(1, 2);
            Lcd.print("SaveTask");
            Lcd.setCursor(1, 3);
            Lcd.print("ExitTask");
          }
          break;
      }
      if (FlashFlag) { //刷新畫面數字
        switch (Index) { //刷新調時數據
          case SelectSetYear:
            Lcd.setCursor(0, 1);
            Lcd.print(DS3231_Data.TempYear + 2000);
            break;
          case SelectSetMonth:
            Lcd.setCursor(5, 1);
            if (DS3231_Data.TempMonth < 10) Lcd.print("0");
            Lcd.print(int(DS3231_Data.TempMonth));
            break;
          case SelectSetDate:
            Lcd.setCursor(8, 1);
            if (DS3231_Data.TempDate < 10) Lcd.print("0");
            Lcd.print(int(DS3231_Data.TempDate));
            break;
          case SelectSetHour:
            Lcd.setCursor(12, 1);
            if (DS3231_Data.TempHour < 10) Lcd.print("0");
            Lcd.print(int(DS3231_Data.TempHour));
            break;
          case SelectSetMin:
            Lcd.setCursor(15, 1);
            if (DS3231_Data.TempMin < 10) Lcd.print("0");
            Lcd.print(int(DS3231_Data.TempMin));
            break;
          case SelectSetSec:
            Lcd.setCursor(18, 1);
            if (DS3231_Data.TempSec < 10) Lcd.print("0");
            Lcd.print(int(DS3231_Data.TempSec));
            break;
        }
        switch (Index) { //刷新任務數據
          case SetTaskDegree:
            Lcd.setCursor(12, 1);
            if (int(TaskData.ServoDegree) < 100) Lcd.print(" ");
            if (int(TaskData.ServoDegree) < 10) Lcd.print(" ");
            Lcd.print(int(TaskData.ServoDegree));
            break;
          case SetTaskTimeHour:
            Lcd.setCursor(13, 2);
            if (int(TaskData.EnableTimeHour) < 10) Lcd.print("0");
            Lcd.print(int(TaskData.EnableTimeHour));
            break;
          case SetTaskTimeMin:
            Lcd.setCursor(12, 2);
            Lcd.print(" ");
            Lcd.setCursor(16, 2);
            if (int(TaskData.EnableTimeMin) < 10) Lcd.print("0");
            Lcd.print(int(TaskData.EnableTimeMin));
            break;
          case SetTaskWaterTime:
            Lcd.setCursor(12, 3);
            if (int(TaskData.WaterTime) < 100) Lcd.print(" ");
            if (int(TaskData.WaterTime) < 10) Lcd.print(" ");
            Lcd.print(int(TaskData.WaterTime));
            break;
          case SetTaskActive:
            Lcd.setCursor(8, 1);
            if (TaskData.Active) Lcd.print("Active  ");
            if (!TaskData.Active)Lcd.print("DeActive");
            break;
        }
      }
      EnterFlag = EC_Flag = 0; //用完就要清除
    }
    if (FlashFlag) { //處理閃爍刷新
      if (SystemTime - BlinkTime > 300) { //每0.3秒亮滅
        BlinkTime += 300;
        BlinkFlag = !BlinkFlag;
        if (BlinkFlag) { //點亮
          switch (Index) { //處理調時指示ON
            case SelectSetYear:
              Lcd.setCursor(3, 2);
              Lcd.print("^");
              break;
            case SelectSetMonth:
              Lcd.setCursor(6, 2);
              Lcd.print("^");
              break;
            case SelectSetDate:
              Lcd.setCursor(9, 2);
              Lcd.print("^");
              break;
            case SelectSetHour:
              Lcd.setCursor(13, 2);
              Lcd.print("^");
              break;
            case SelectSetMin:
              Lcd.setCursor(16, 2);
              Lcd.print("^");
              break;
            case SelectSetSec:
              Lcd.setCursor(19, 2);
              Lcd.print("^");
              break;
          }
          switch (Index) { //處理排程指示ON
            case SetSchedule:
              Lcd.setCursor(7, 2);
              Lcd.print(">");
              Lcd.setCursor(14, 2);
              Lcd.print("<");
              break;
          }
          switch (Index) { //處理任務指示ON
            case SetTaskDegree:
            case SetTaskActive:
              Lcd.setCursor(0, 1);
              Lcd.print("*");
              break;
            case SetTaskTime:
            case SaveTask:
              Lcd.setCursor(0, 2);
              Lcd.print("*");
              break;
            case SetTaskWaterTime:
            case ExitTask:
              Lcd.setCursor(0, 3);
              Lcd.print("*");
              break;
            case SetTaskTimeHour:
              Lcd.setCursor(12, 2);
              Lcd.print(">");
              break;
            case SetTaskTimeMin:
              Lcd.setCursor(18, 2);
              Lcd.print("<");
              break;
          }
        }
        else { //熄滅
          switch (Index) { //處理時鐘指示OFF
            case SelectSetYear:
              Lcd.setCursor(3, 2);
              Lcd.print(" ");
              break;
            case SelectSetMonth:
              Lcd.setCursor(6, 2);
              Lcd.print(" ");
              break;
            case SelectSetDate:
              Lcd.setCursor(9, 2);
              Lcd.print(" ");
              break;
            case SelectSetHour:
              Lcd.setCursor(13, 2);
              Lcd.print(" ");
              break;
            case SelectSetMin:
              Lcd.setCursor(16, 2);
              Lcd.print(" ");
              break;
            case SelectSetSec:
              Lcd.setCursor(19, 2);
              Lcd.print(" ");
              break;
          }
          switch (Index) { //處理排程指示OFF
            case SetSchedule:
              Lcd.setCursor(7, 2);
              Lcd.print(" ");
              Lcd.setCursor(14, 2);
              Lcd.print(" ");
              break;
          }
          switch (Index) { //處理任務指示OFF
            case SetTaskDegree:
            case SetTaskActive:
              Lcd.setCursor(0, 1);
              Lcd.print(" ");
              break;
            case SetTaskTime:
            case SaveTask:
              Lcd.setCursor(0, 2);
              Lcd.print(" ");
              break;
            case SetTaskWaterTime:
            case ExitTask:
              Lcd.setCursor(0, 3);
              Lcd.print(" ");
              break;
            case SetTaskTimeHour:
              Lcd.setCursor(12, 2);
              Lcd.print(" ");
              break;
            case SetTaskTimeMin:
              Lcd.setCursor(18, 2);
              Lcd.print(" ");
              break;
          }
        }
      }
    }
    else { //游標無閃爍刷新
      switch (Index) {//處理主選單游標
        case SelectSetSystemTime:
          Lcd.setCursor(3, 1);
          Lcd.print("*");
          Lcd.setCursor(3, 2);
          Lcd.print(" ");
          break;
        case SelectSetSchedule:
          Lcd.setCursor(3, 1);
          Lcd.print(" ");
          Lcd.setCursor(3, 2);
          Lcd.print("*");
          break;
      }
      switch (Index) {//處理時鐘選單游標
        case SelectSetYear:
          Lcd.setCursor(12, 3);
          Lcd.print(" ");
          Lcd.setCursor(6, 2);
          Lcd.print(" ");
          Lcd.setCursor(3, 2);
          Lcd.print("^");
          break;
        case SelectSetMonth :
          Lcd.setCursor(9, 2);
          Lcd.print(" ");
          Lcd.setCursor(3, 2);
          Lcd.print(" ");
          Lcd.setCursor(6, 2);
          Lcd.print("^");
          break;
        case SelectSetDate :
          Lcd.setCursor(6, 2);
          Lcd.print(" ");
          Lcd.setCursor(13, 2);
          Lcd.print(" ");
          Lcd.setCursor(9, 2);
          Lcd.print("^");
          break;
        case SelectSetHour :
          Lcd.setCursor(9, 2);
          Lcd.print(" ");
          Lcd.setCursor(16, 2);
          Lcd.print(" ");
          Lcd.setCursor(13, 2);
          Lcd.print("^");
          break;
        case SelectSetMin :
          Lcd.setCursor(13, 2);
          Lcd.print(" ");
          Lcd.setCursor(19, 2);
          Lcd.print(" ");
          Lcd.setCursor(16, 2);
          Lcd.print("^");
          break;
        case SelectSetSec :
          Lcd.setCursor(16, 2);
          Lcd.print(" ");
          Lcd.setCursor(2, 3);
          Lcd.print(" ");
          Lcd.setCursor(19, 2);
          Lcd.print("^");
          break;
        case SaveSystemTime :
          Lcd.setCursor(19, 2);
          Lcd.print(" ");
          Lcd.setCursor(12, 3);
          Lcd.print(" ");
          Lcd.setCursor(2, 3);
          Lcd.print("*");
          break;
        case ExitSystemTime :
          Lcd.setCursor(2, 3);
          Lcd.print(" ");
          Lcd.setCursor(3, 2);
          Lcd.print(" ");
          Lcd.setCursor(12, 3);
          Lcd.print("*");
          break;
      }
      switch (Index) {//處理排程選單游標
        case SetTaskDegree:
        case SetTaskActive:
          Lcd.setCursor(0, 1);
          Lcd.print("*");
          Lcd.setCursor(0, 2);
          Lcd.print(" ");
          Lcd.setCursor(0, 3);
          Lcd.print(" ");
          break;
        case SetTaskTime:
        case SaveTask:
          if (Old_Index == SetTaskTimeMin) {
            Lcd.setCursor(18, 2);
            Lcd.print(" ");
          }
          Lcd.setCursor(0, 1);
          Lcd.print(" ");
          Lcd.setCursor(0, 2);
          Lcd.print("*");
          Lcd.setCursor(0, 3);
          Lcd.print(" ");
          break;
        case SetTaskWaterTime:
        case ExitTask:
          Lcd.setCursor(0, 1);
          Lcd.print(" ");
          Lcd.setCursor(0, 2);
          Lcd.print(" ");
          Lcd.setCursor(0, 3);
          Lcd.print("*");
          break;
      }
    }
  }
  if ((DS3231_Data.Sec != Old_Sec) && (Index == ShowState)) { //單純更新時間與狀態 //////////////////////////////////////////////////////////////////////////
    Lcd.setCursor(4, 0);
    if (SystemRun) {
      Lcd.print("Auto Watering");
    }
    else {
      Lcd.print("STOP Watering");
    }
    Lcd.setCursor(0, 1);
    Lcd.print(2000 + DS3231_Data.Year);
    Lcd.print("/");
    if (DS3231_Data.Month <= 9) Lcd.print("0");
    Lcd.print(int(DS3231_Data.Month)); //為了修正I2C函示庫的問題
    Lcd.print("/");
    if (DS3231_Data.Date <= 9) Lcd.print("0");
    Lcd.print(int(DS3231_Data.Date));
    Lcd.print(" ");
    Lcd.print(" ");
    if (DS3231_Data.Hour <= 9) Lcd.print("0");
    Lcd.print(int(DS3231_Data.Hour));
    Lcd.print(":");
    if (DS3231_Data.Min <= 9) Lcd.print("0");
    Lcd.print(int(DS3231_Data.Min));
    Lcd.print(":");
    if (DS3231_Data.Sec <= 9) Lcd.print("0");
    Lcd.print(int(DS3231_Data.Sec));
  }
  if ((DS3231_Data.Min != Old_Min) && SystemRun) { //整分檢查任務時鐘並設立旗標
    NowTime = int(DS3231_Data.Hour) * 60 + int(DS3231_Data.Min);
    for (i = 1; i <= 9; i++) {
      if ((TimeValue[i] == NowTime) && EEPROM.read(i * 10)) TaskOrder[i] = 1; //設立旗標
    }
  } /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  switch (TaskOperation) {
    case TaskSearch:
      for (i = 1; i <= 9; i++) { //序列搜尋啟動旗標
        if (TaskOrder[i]) {
          ArmMoveTime = SystemTime;
          Task = i;
          ServoPostionSet(EEPROM.read(Task * 10 + 4));//移動手臂
          TaskOperation = TaskMoveArm;
          break;
        }
      }
      break;
    case TaskMoveArm:
      if (SystemTime - ArmMoveTime > 1500) { //延遲1.5秒
        WateringStart = SystemTime;
        WateringTime = EEPROM.read(Task * 10 + 3) * 1000;
        digitalWrite(Pump_Pin, 0); //開啟幫浦
        TaskOperation = TaskWatering;
      }
      break;
    case TaskWatering:
      if (SystemTime - WateringStart > WateringTime) {
        digitalWrite(Pump_Pin, 1); //關閉幫浦
        WateringStop = SystemTime;
        TaskOperation = TaskReturn;
      }
      break;
    case TaskReturn:
      if (SystemTime - WateringStop > 1500) {
        ServoPostionSet(0); //復歸手臂
        TaskOrder[Task] = 0; //清除旗標
        TaskOperation = TaskSearch;
      }
      break;
  }
}
