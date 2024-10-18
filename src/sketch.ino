#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

//------------------------------------ Define OLED Parameters -------------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//----------------------------------------- LED ---------------------------------------------
int alarmLED = 15;

//------------------------------- Buttons - Active High -------------------------------------
int buttonUP = 16;
int buttonDOWN = 17;
int buttonSLCT = 18;
int buttonCNCL = 19;

#define ButtonDebounceTime 50

//------------------------------------------ NTP --------------------------------------------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

int days = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 5.5 * 60 * 60;
const int daylightOffset_sec = 0;
int gmt_hour = 0;
float gmt_minute = 0;
float gmt_offset = 5.5 * 60 * 60;

//----------------------------------------- Modes -------------------------------------------
int current_mode = 0;
int max_modes = 5;
String modes[] = {"Set \nAlarm 1", "Set \nAlarm 2", "Set \nAlarm 3", "Disable \nall \nalarms", "Set \nTime Zone"};

//---------------------------------------- Alarms ------------------------------------------
bool alarm_enabled = false;
int n_alarms = 3;
int alarm_hours[] = {0, 0, 0};
int alarm_minutes[] = {0, 0, 0};
bool alarm_triggered[] = {true, true, true};

//---------------------------------------- Setup -------------------------------------------
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Hello, ESP32!");

  // display initialization
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed!"));
    for (;;)
      ;
  }

  display.display();
  delay(1000);

  display.clearDisplay();
  print_line("ECD & MP", 18, 0, 2);
  print_line("AUG 2024", 18, 20, 2);
  delay(1000);

  // wifi initialization
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    display.clearDisplay();
    print_line("WIFI \nConnecting... ", 0, 0, 2);
  }
  display.clearDisplay();
  print_line("WIFI \nConnected ", 0, 0, 2);
  delay(1000);
  display.clearDisplay();
  display.display();

  Serial.println("Wifi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  timeClient.setTimeOffset(5.5 * 3600);

  // initialize the digital pins as inputs and outputs
  pinMode(alarmLED, OUTPUT);
  pinMode(buttonUP, INPUT);
  pinMode(buttonDOWN, INPUT);
  pinMode(buttonSLCT, INPUT);
  pinMode(buttonCNCL, INPUT);
}

//------------------------------------------ Loop -------------------------------------------
void loop()
{

  update_time_and_check_alarm();
  print_time_now();

  // put your main code here, to run repeatedly:
  if (digitalRead(buttonSLCT) == HIGH)
  {
    delay(ButtonDebounceTime);
    Serial.print("Entered the MENU - ");
    Serial.println(buttonSLCT);
    goto_menu();
  }

  delay(10); // this speeds up the simulation
}

//-------------------------------------- Print LIne -----------------------------------------
void print_line(String text, int column, int row, int text_size)
{
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column, row); // Set cursor position
  display.println(text);          // Print text
  display.display();              // Update the display
}

//--------------------------------------- Update Time --------------------------------------
void update_time_and_check_alarm()
{
  configTime(gmt_offset, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  hours = atoi(timeHour);
  char timeMinute[3];
  strftime(timeMinute, 3, "%M", &timeinfo);
  minutes = atoi(timeMinute);
  char timeSecond[3];
  strftime(timeSecond, 3, "%S", &timeinfo);
  seconds = atoi(timeSecond);
  char timeDay[3];
  strftime(timeDay, 3, "%d", &timeinfo);
  days = atoi(timeDay);

  // check alarm
  if (alarm_enabled == true)
  {
    for (int i = 0; i < n_alarms; i++)
    {
      if ((alarm_triggered[i] == false) && (alarm_hours[i] == hours) && (alarm_minutes[i] == minutes))
      {
        ring_alarm();
        alarm_triggered[i] == true;
      }
    }
  }
}

//---------------------------------------- Print Time Now -----------------------------------
void print_time_now(void)
{
  display.clearDisplay();
  print_line(String(days), 0, 0, 2);
  print_line("|", 20, 0, 2);
  print_line(String(hours), 30, 0, 2);
  print_line(":", 50, 0, 2);
  print_line(String(minutes), 60, 0, 2);
  print_line(":", 80, 0, 2);
  print_line(String(seconds), 90, 0, 2);
}

//-------------------------------------- Wait For Button Press ------------------------------
int wait_for_button_press()
{
  while (true)
  {
    Serial.println("Entered: wait_for_button_press");
    if (digitalRead(buttonUP) == HIGH)
    {
      delay(ButtonDebounceTime);
      Serial.print("Button Pressed - ");
      Serial.println(buttonUP);
      return buttonUP;
    }
    else if (digitalRead(buttonDOWN) == HIGH)
    {
      delay(ButtonDebounceTime);
      Serial.print("Button Pressed - ");
      Serial.println(buttonDOWN);
      return buttonDOWN;
    }
    else if (digitalRead(buttonSLCT) == HIGH)
    {
      delay(ButtonDebounceTime);
      Serial.print("Button Pressed - ");
      Serial.println(buttonSLCT);
      return buttonSLCT;
    }
    else if (digitalRead(buttonCNCL) == HIGH)
    {
      delay(ButtonDebounceTime);
      Serial.print("Button Pressed - ");
      Serial.println(buttonCNCL);
      return buttonCNCL;
    }
    update_time_and_check_alarm();
  }
}

//-------------------------------------------- Go To Menu ------------------------------------
void goto_menu()
{
  display.clearDisplay();
  print_line("MENU", 40, 20, 2);
  delay(1000);

  while (true)
  {
    Serial.println("Entered the while loop inside goto_menu");
    Serial.println(current_mode);

    display.clearDisplay();
    print_line(modes[current_mode], 0, 0, 2);

    int pressed = wait_for_button_press();

    if (pressed == buttonUP)
    {
      current_mode -= 1;
      if (current_mode < 0)
      {
        current_mode = max_modes - 1;
      }
    }

    if (pressed == buttonDOWN)
    {
      current_mode += 1;
      if (current_mode == max_modes)
      {
        current_mode = 0;
      }
    }

    if (pressed == buttonSLCT)
    {
      run_mode(current_mode);
    }

    if (pressed == buttonCNCL)
    {
      break;
    }
  }
}

//---------------------------------------- Run Mode ------------------------------------------
void run_mode(int mode)
{
  if (mode == 0 || mode == 1 || mode == 2)
  {
    set_alarm(mode);
  }
  else if (mode == 3)
  {
    // disable all alarms
    alarm_enabled = false;
    display.clearDisplay();
    print_line("All \nalarms \ndisabled!", 0, 0, 2);
  }
  else if (mode == 4)
  {
    display.clearDisplay();
    // change time zone
    set_time_zone();
  }
}

//-------------------------------------- Set Alarm --------------------------------------------
void set_alarm(int alarm)
{
  Serial.println("Entered: set_alarm");
  alarm_enabled = true;

  int temp_hour = alarm_hours[alarm];
  while (true)
  {
    display.clearDisplay();
    print_line("Enter", 0, 0, 2);
    print_line("hour", 0, 16, 2);
    print_line(String(temp_hour), 80, 0, 4);

    int pressed = wait_for_button_press();

    if (pressed == buttonUP)
    {
      temp_hour += 1;
      temp_hour %= 24;
    }

    if (pressed == buttonDOWN)
    {
      temp_hour -= 1;
      if (temp_hour < 0)
      {
        temp_hour = 23;
      }
    }

    if (pressed == buttonSLCT)
    {
      alarm_hours[alarm] = temp_hour;
      break;
    }

    if (pressed == buttonCNCL)
    {
      display.clearDisplay();
      print_line("Cancelled", 0, 0, 2);
      delay(1000);
      break;
    }
  }

  int temp_minute = alarm_minutes[alarm];
  while (true)
  {
    display.clearDisplay();
    print_line("Enter", 0, 0, 2);
    print_line("minute", 0, 16, 2);
    print_line(String(temp_minute), 80, 0, 4);

    int pressed = wait_for_button_press();

    if (pressed == buttonUP)
    {
      temp_minute += 1;
      temp_minute %= 60;
    }

    if (pressed == buttonDOWN)
    {
      temp_minute -= 1;
      if (temp_minute < 0)
      {
        temp_minute = 59;
      }
    }

    if (pressed == buttonSLCT)
    {
      alarm_minutes[alarm] = temp_minute;

      alarm_triggered[alarm] = false;

      display.clearDisplay();
      print_line("Alarm", 0, 0, 2);
      print_line("is set!", 0, 16, 2);
      delay(1000);
      break;
    }

    if (pressed == buttonCNCL)
    {
      display.clearDisplay();
      print_line("Cancelled!", 0, 0, 2);
      delay(1000);
      break;
    }
  }
}

//-------------------------------------- Ring Alarm --------------------------------------------
void ring_alarm()
{
  display.clearDisplay();
  print_line("Alarm!!!", 0, 0, 2);
  digitalWrite(alarmLED, HIGH);
  while (true)
  {
    int pressed = wait_for_button_press();

    if (pressed == buttonCNCL)
    {
      display.clearDisplay();
      print_line("Alarm OFF!", 0, 0, 2);
      digitalWrite(alarmLED, LOW);
      break;
    }
  }
}

//-------------------------------------- Set Time Zone --------------------------------------------
void set_time_zone()
{
  Serial.println("Entered: set_alarm");
  alarm_enabled = true;

  int temp_hour = gmt_hour;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter", 0, 0, 2);
    print_line("hour", 0, 16, 2);
    print_line(String(temp_hour), 80, 0, 4);

    int pressed = wait_for_button_press();

    if (pressed == buttonUP)
    {
      temp_hour += 1;
      temp_hour %= 24;
    }

    if (pressed == buttonDOWN)
    {
      temp_hour -= 1;
      if (temp_hour < 0)
      {
        temp_hour = 23;
      }
    }

    if (pressed == buttonSLCT)
    {
      gmt_hour = temp_hour;
      break;
    }

    if (pressed == buttonCNCL)
    {
      display.clearDisplay();
      print_line("Cancelled", 0, 0, 2);
      delay(1000);
      break;
    }
  }

  int temp_minute = gmt_minute;
  while (true)
  {
    display.clearDisplay();
    print_line("Enter", 0, 0, 2);
    print_line("minute", 0, 16, 2);
    print_line(String(temp_minute), 80, 0, 4);

    int pressed = wait_for_button_press();

    if (pressed == buttonUP)
    {
      temp_minute += 1;
      temp_minute %= 60;
    }

    if (pressed == buttonDOWN)
    {
      temp_minute -= 1;
      if (temp_minute < 0)
      {
        temp_minute = 59;
      }
    }

    if (pressed == buttonSLCT)
    {
      gmt_minute = temp_minute;

      display.clearDisplay();
      print_line("Time zone \nis set", 0, 0, 2);
      delay(1000);
      break;
    }

    if (pressed == buttonCNCL)
    {
      display.clearDisplay();
      print_line("Cancelled!", 0, 0, 2);
      delay(1000);
      break;
    }
  }
  gmt_offset = (gmt_hour + (gmt_minute / 60)) * 60 * 60;
}