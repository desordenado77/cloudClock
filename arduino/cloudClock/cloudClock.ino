#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time  
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire (included with Arduino IDE)

#include <Encoder.h>
#include <LiquidCrystal.h>
#include <FileIO.h>

/*******************************************
 ********* Linino Interface ****************
 *******************************************/
char temp[128];
Process async_process;
int async_process_started = 0;
int now_playing = -1;

static const char* day_of_the_week[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* month_name[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

char* runProcess(char* process, char* param, char* param1, char* param2) {
  Process p;
  p.begin(process);
  p.addParameter(param);

  Serial.print("running: ");
  Serial.print(process);
  Serial.print(" ");
  Serial.print(param);

  if(param1 != NULL) {
    p.addParameter(param1);
    Serial.print(" ");
    Serial.print(param1);
  }
  if(param2 != NULL) {
    p.addParameter(param2);
    Serial.print(" ");
    Serial.print(param2);
  }
  Serial.println("");
  int ret = p.run();
  Serial.println(ret);
  int i = 0;
  char r;
  while (p.available() > 0) {
    r = p.read();
    Serial.print(r);
    if(i<128) {
      temp[i] = r;
    }
    i++;
  }
  temp[i] = 0;
  p.flush();
  p.close();
  return temp;
}

char* runAsyncShellProcess(char* process) {
  Serial.print("running async: ");
  Serial.println(process);
  async_process.runShellCommandAsynchronously(process);
  async_process_started = 1;
}


void forceNtpUpdate() {
  runProcess("/etc/init.c/ntpd", "stop", NULL, NULL);
  delay(200);
  runProcess("ntpd", "-gnqd", NULL, NULL);
  runProcess("/etc/init.c/ntpd", "start", NULL, NULL);
}


char * getNetTime() {
  char* ret = runProcess("date", "+%H:%M %a %d-%b", NULL, NULL);
  
  ret[strlen(ret)-1] = 0;
  return ret;
}

char * getRTCTime() {
  Serial.println("set RTC time");
   snprintf(temp, 128, "%02d:%02d %s %02d-%s", hour(), minute(), day_of_the_week[weekday()-1], day(), month_name[month()-1]);

  Serial.println(temp);
   return temp;
}

char* setRTCTime() {
  tmElements_t tm;
  time_t t;
  int y;
  forceNtpUpdate();
  char* ret = runProcess("date", "+%y", NULL, NULL);
  y=atoi(ret);
  if (y >= 1000)
      tm.Year = CalendarYrToTm(y);
  else    //(y < 100)
      tm.Year = y2kYearToTm(y);
  ret = runProcess("date", "+%m", NULL, NULL);
  tm.Month = atoi(ret);
  ret = runProcess("date", "+%d", NULL, NULL);
  tm.Day = atoi(ret);
  ret = runProcess("date", "+%H", NULL, NULL);
  tm.Hour = atoi(ret);
  ret = runProcess("date", "+%M", NULL, NULL);
  tm.Minute = atoi(ret);
  ret = runProcess("date", "+%S", NULL, NULL);
  tm.Second = atoi(ret);
  t = makeTime(tm);
  RTC.set(t);        //use the time_t value to ensure correct weekday is set
  setTime(t);     
  sprintf(temp, "Success");
}

char* getRadioName(int num) {
  sprintf(temp, "%d", num);
  char* ret = runProcess("python", "/root/cloudClock/radios_json.py", "name", temp);
  ret[strlen(ret)-1]=0;
  return ret;
}


int getRadioNum() {
  return atoi(runProcess("python", "/root/cloudClock/radios_json.py", "number", NULL));
}

void playRadio(int num) {
  runProcess("killall", "mpg123", NULL, NULL);
  
  now_playing = num;
  snprintf(temp, 128, "mpg123 -@ $(python /root/cloudClock/radios_json.py URL %d)", num);
  runAsyncShellProcess(temp); 
}

/*******************************************
 ********* LCD display    ******************
 *******************************************/
#define LCD_LENGTH 16
#define LCD_LINES  2
#define LCD_REFRESH_TIME 800

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

char line0[64];
char line1[64];
char temp_line[17];

char* lines[] = { line0, line1 };
int update[] = {0,0};
unsigned long update_time[] = {0,0};
int offset[] = {0,0};
int len[] = {0,0}; 
char* clearLine = "                ";

void setLine(int line, char* text) {
//  MsTimer2::stop();
  char* the_line = lines[line];
//Serial.print(line);
//Serial.print(": ");
//Serial.println(text);

  if(strcmp(the_line, text)!=0) {
    int line_length = strlen(text);
    len[line] = line_length;
    if(line_length<LCD_LENGTH){
      memcpy(the_line,text,line_length);
      memcpy(the_line+line_length, clearLine, LCD_LENGTH-line_length);
      the_line[LCD_LENGTH] = '\0';
    }
    else {
      strcpy(the_line, text);
    }
    update[line] = 1;
    offset[line] = 0;
    update_screen();
  }
//  MsTimer2::start();
}

void update_screen() {
  int i;
  unsigned long time_now = millis();
  for(i=0;i<LCD_LINES;i++) {
    if(update[i]){
      lcd.setCursor(0,i);
      memcpy(temp_line, lines[i], 16);
      temp_line[17] = '\0';
      lcd.print(temp_line);
      update[i] = 0;
      update_time[i] = time_now;
    }
    else {
      if(time_now<update_time[i]+600){
        continue;
      }
      if(len[i]>LCD_LENGTH) {
        Serial.println(i);
        Serial.println(len[i]);
        lcd.setCursor(0,i);
        memcpy(temp_line, lines[i]+offset[i], 16);
        temp_line[17] = '\0';
        lcd.print(temp_line);
        update_time[i] = time_now;
        offset[i]++;
        if((offset[i]+LCD_LENGTH)>len[i]) {
          offset[i] = 0;
        }
      }
    }
  }
}


/*******************************************
 ********* Rotary Encoder ******************
 *******************************************/

Encoder knob(5,7);
int knob_sw = 6;
unsigned long time_since_last_move = 0;
unsigned long time_last_move = 0;
long move = 0;
long prev_move = 0;
int sw_value = 0;
int prev_sw_value = 0;
unsigned long time_since_last_pbc = 0;
unsigned long time_last_pbc = 0;


long knobRead(){
  long ret = knob.read();
  

  prev_move = move;
  move = ret;

  if(ret != 0) {
    Serial.println((int)ret);
    unsigned long time_now = millis();
    time_since_last_move = time_now - time_last_move;
    time_last_move = time_now;
    knob.write(0);
  }
  
  return ret;
}

// 1 for repeat 0  for back
int repeat_or_back() {
  if(move<0 && prev_move<0 && time_since_last_move<1000) {
    return 0;
  }
  return 1;
}

int buttonPressed() {
  int value_read = digitalRead(knob_sw);  
  
  if(value_read != sw_value) {
    prev_sw_value = sw_value;
    sw_value = value_read;

    unsigned long time_now = millis();
    time_since_last_pbc = time_now - time_last_pbc;
    time_last_pbc = time_now;
  }
  
  return !sw_value;
}

unsigned long pbcDuration(){
  return time_since_last_pbc;
}


/*******************************************
 ********* Menus          ******************
 *******************************************/

char temp_menu[128];

typedef enum {
  MENU0_TIME = 0,
  MENU0_NOWPLAYING,
  MENU0_RADIOS,
  MENU0_ALARMS,
  MENU0_CONFIG,
  MENU0_MAXVALUE,
  MENU0_NONE = -1
} MENU0;

typedef enum {
  MENU1_TIME_INTERNET,
  MENU1_TIME_RTC,
  MENU1_TIME_SET,  
} MENU1_TIME;

typedef enum {
  MENU1_CONFIG_ALARM,
  MENU1_CONFIG_VOLUME,
  MENU1_CONFIG_STATUS,  
} MENU1_CONFIG;

char* menu0_str[] = { "Time", "Now Playing", "Radios", "Alarms", "Configuration" } ;
char* menu1_time_str[] = { "Net Time", "RTC Time", "Set Time" };
char* menu1_config_str[] = { "Alarm", "Volume", "Status" };
int menu0_selected = MENU0_NONE;
int menu0_view = MENU0_TIME;
int menu1_time_selected = -1;
int menu1_time_view = 0;
int menu1_now_playing_selected = -1;
int menu1_now_playing_view = 0;
int menu1_radio_selected = -1;
int menu1_radio_view = 0;
int menu1_alarms_selected = -1;
int menu1_alarms_view = 0;
int menu1_config_selected = -1;
int menu1_config_view = 0;

int volume = 100;

void do_menu0(int knob, int pbc) {
  setLine(0, "Menu:");
  setLine(1, menu0_str[menu0_view]);
//Serial.print(menu0_view);
//Serial.println(menu0_str[menu0_view]);
  if(pbc) {
    menu0_selected = menu0_view;
  }
  else {
    if(knob>0) {
      menu0_view++;
    }
    if(knob<0) {
      menu0_view--;
    }
    
    if(menu0_view < 0){
      menu0_view = 0;
    }
    if(menu0_view >= MENU0_MAXVALUE){
      menu0_view = MENU0_MAXVALUE - 1;
    }  
  }
}

void do_menu1_time(int knob, int pbc) {
  setLine(0, "Time:");
  setLine(1, menu1_time_str[menu1_time_view]);
  if(pbc && (pbcDuration()>1500)) {
    menu0_selected = MENU0_NONE;
    menu0_view = 0;
    menu1_time_view = 0;
  }
  else {
    if(pbc && (pbcDuration()<1500)) {
      menu1_time_selected = menu1_time_view;
    }
    else {
      if(knob>0) {
        menu1_time_view++;
      }
      if(knob<0) {
        menu1_time_view--;
      }
      
      if(menu1_time_view < 0){
        menu1_time_view = 0;
      }
      int confignum = sizeof(menu1_time_str)/sizeof(char*);
      if(menu1_time_view >= confignum){
        menu1_time_view = confignum - 1;
      }      
    }
  }
}

void do_menu1_config(int knob, int pbc) {
  setLine(0, "Config:");
  setLine(1, menu1_config_str[menu1_config_view]);
  if(pbc && (pbcDuration()>1500)) {
    menu0_selected = MENU0_NONE;
    menu0_view = 0;
    menu1_config_view = 0;
  }
  else {
    if(pbc && (pbcDuration()<1500)) {
      menu1_config_selected = menu1_config_view;
    }
    else {
      if(knob>0) {
        menu1_config_view++;
      }
      if(knob<0) {
        menu1_config_view--;
      }
      
      if(menu1_config_view < 0){
        menu1_config_view = 0;
      }
      int confignum = sizeof(menu1_config_str)/sizeof(char*);
      if(menu1_config_view >= confignum){
        menu1_config_view = confignum - 1;
      }      
    }
  }
}


void do_menu2_time_net(int knob, int pbc) {
  setLine(0, "Time/Net Time:");
  setLine(1, getNetTime());
  if(pbc) {
    menu1_time_view = menu1_time_selected;
    menu1_time_selected = -1;
  }
}

void do_menu2_time_rtc(int knob, int pbc) {
  setLine(0, "Time/RTC Time:");
  setLine(1, getRTCTime());
  if(pbc) {
    menu1_time_view = menu1_time_selected;
    menu1_time_selected = -1;
  }
}

void do_menu2_time_setrtc(int knob, int pbc) {
  static int setTimeDone = 0;
  if(setTimeDone == 0) {
    setLine(0, "Time/Set Time:");
    setLine(1, "");
    setLine(1, setRTCTime());
    setTimeDone = 1;
  }
  if(pbc) {
    menu1_time_view = menu1_time_selected;
    menu1_time_selected = -1;
    setTimeDone = 0;
  }
}

void do_menu1_now_playing(int knob, int pbc) {
  static int last_now_playing = -1;
  setLine(0, "Now Playing:");
  setLine(1, "");
  if(now_playing != last_now_playing) {
    if(now_playing != -1) {
      setLine(1, getRadioName(now_playing));
    }
    last_now_playing = now_playing;
  }
  if(pbc) {
    menu0_selected = -1;
    menu0_view = 0;    
    last_now_playing = -1;
  }
}

void do_menu1_radio(int knob, int pbc) {
  static int last_radio_view = -1;
  static int radionum;
  setLine(0, "Radio:");
  if(last_radio_view!=menu1_radio_view) {
    setLine(1, "");
    setLine(1, getRadioName(menu1_radio_view));
    radionum = getRadioNum();
    last_radio_view = menu1_radio_view;
  }
  if(pbc && (pbcDuration()>1500)) {
    menu0_selected = MENU0_NONE;
    menu0_view = MENU0_RADIOS;
    menu1_radio_view = 0;
    last_radio_view = -1;    
  }
  else {
    if(pbc && (pbcDuration()<1500)) {
      playRadio(menu1_radio_view);
      menu0_selected = MENU0_NOWPLAYING;
      menu0_view = MENU0_NOWPLAYING;
      last_radio_view = -1;      
    }
    else {
      if(knob>0) {
        menu1_radio_view++;
      }
      if(knob<0) {
        menu1_radio_view--;
      }
      
      if(menu1_radio_view < 0){
        menu1_radio_view = 0;
      }
      
      if(menu1_radio_view >= radionum){
        menu1_radio_view = radionum - 1;
      }      
    }
  }
}

/*******************************************
 ********* Setup          ******************
 *******************************************/
void setup() {

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // Print a message to the LCD.
  setLine(0, "cloudClock");
  setLine(1, "Booting...");

  update_screen();
  
  Serial.begin(9600);
  Bridge.begin();
  setLine(1, "Booting... Done");
  delay(500);
  
  pinMode(knob_sw, INPUT);

  digitalWrite(knob_sw, HIGH);
  
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus() != timeSet) {
    setLine(1, "RTC sync failed");
  }
}

/*******************************************
 ********* Loop           ******************
 *******************************************/
int prev_button = 0;

void loop() {
  long knob_value = knobRead();
  int button = buttonPressed();

  int high_to_low = (button - prev_button) == -1;
  
  switch(menu0_selected) {
    case MENU0_NONE:
      do_menu0(knob_value, high_to_low);
    break;
    case MENU0_TIME:
      if(menu1_time_selected == -1) {
        do_menu1_time(knob_value, high_to_low);
      }
      else {
        switch(menu1_time_selected) {
          case MENU1_TIME_INTERNET:
          do_menu2_time_net(knob_value, high_to_low);
          break;
          case MENU1_TIME_RTC:
          do_menu2_time_rtc(knob_value, high_to_low);
          break;
          case MENU1_TIME_SET:
          do_menu2_time_setrtc(knob_value, high_to_low);
          break;
        }
      }
    break;
    case MENU0_NOWPLAYING:
    do_menu1_now_playing(knob_value, high_to_low);
    break;
    case MENU0_RADIOS:
    do_menu1_radio(knob_value, high_to_low);
    break;
    case MENU0_ALARMS:
    break;
    case MENU0_CONFIG:
      if(menu1_config_selected == -1) {
        do_menu1_config(knob_value, high_to_low);
      }
      else {
        switch(menu1_config_selected) {
          case MENU1_CONFIG_ALARM:
          break;
          case MENU1_CONFIG_VOLUME:
          break;
          case MENU1_CONFIG_STATUS:
          break;
        }
      }
    break;
  }
    
  update_screen();
  int i;
  for(i=0;i<80;i++) {
    knob.read();
    delay(1);
  }
  if(async_process_started!=0) {
    Serial.println("async started");
    while(async_process.available()) {
    Serial.println("async read");
      async_process.read();
    }
    async_process.flush();
    if(!async_process.running()) {
      async_process.close();
      async_process_started = 0;
    }
  }
  else {
    now_playing = -1;
  }
  prev_button = button;
}
