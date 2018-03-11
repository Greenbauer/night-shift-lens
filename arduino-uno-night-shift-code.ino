#include <config.h>
#include <ds3231.h>
#include <Servo.h>
#include <Wire.h>

#define BUFF_MAX 256

Servo myservo;  // create servo object to control a servo

int pos = 0;    // variable to store the servo position

uint8_t time[8];
char recv[BUFF_MAX];
unsigned int recv_size = 0;
unsigned long prev = 5000, interval = 5000;

void setup() 
{
  Serial.begin(9600);
  Wire.begin();
  DS3231_init(DS3231_INTCN);

  myservo.attach(9,530,2500);  // attaches the servo on pin 9 to the servo object, min pulse width, max pulse width
  myservo.write(0); //initial position
  
  //sets clock to real time. Only need this once
  //Serial.println("Setting time");
  //parse_cmd("T001123104062016", 16);
  //         TssmmhhWDDMMYYYY
}

void loop() 
{
  char in;
  char buff[BUFF_MAX];
  unsigned long now = millis();
  struct ts t;             

  // once a while show what is going on
  /*if ((now - prev > interval) && (Serial.available() <= 0)) 
  {
      DS3231_get(&t);
  
      // display current time
      snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d", t.year,
           t.mon, t.mday, t.hour, t.min, t.sec);
      Serial.println(buff);
  
      // display a1 debug info
      //DS3231_get_a1(&buff[0], 59);
      //Serial.println(buff);
  
      prev = now;
  }*/

  DS3231_get(&t);
  //Serial.println(t.sec);

  //evening rotation
  //if ( t.sec >= 30 && t.sec < 59 && pos <= 0 ) //every 30 sec for test
  if ( t.hour >= 19 && pos <= 5 ) 
  {
    for (pos = 0; pos <= 180; pos += 1) 
    { 
      // in steps of 1 degree
      myservo.write(pos);              
      delay(20);                       
    }
  }
  else if ( t.hour < 4 && pos <= 5 )
  {
    for (pos = 0; pos <= 180; pos += 1) 
    { 
      // in steps of 1 degree
      myservo.write(pos);              
      delay(15);                       
    }
  }
  
  //morning rotation
  //else if ( t.sec >= 0 && t.sec < 29 && pos >= 180 ) //every 30 sec for test
  else if ( t.hour >= 4 && t.hour < 19 && pos >= 175 )
  {
    for (pos = 180; pos >= 0; pos -= 1) 
    {
      myservo.write(pos);              
      delay(15);                       
    }
  }
  delay(150000); //to slow down the loop speed. reruns loop every 15 min. remove to edit
}

//for setting initial time
void parse_cmd(char *cmd, int cmdsize)
{
  uint8_t i;
  uint8_t reg_val;
  char buff[BUFF_MAX];
  struct ts t;
  
  //snprintf(buff, BUFF_MAX, "cmd was '%s' %d\n", cmd, cmdsize);
  //Serial.print(buff);
  
  // TssmmhhWDDMMYYYY aka set time
  if (cmd[0] == 84 && cmdsize == 16) {
      //T355720619112011
      t.sec = inp2toi(cmd, 1);
      t.min = inp2toi(cmd, 3);
      t.hour = inp2toi(cmd, 5);
      t.wday = cmd[7] - 48;
      t.mday = inp2toi(cmd, 8);
      t.mon = inp2toi(cmd, 10);
      t.year = inp2toi(cmd, 12) * 100 + inp2toi(cmd, 14);
      DS3231_set(t);
      Serial.println("OK");
  } else if (cmd[0] == 49 && cmdsize == 1) {  // "1" get alarm 1
      DS3231_get_a1(&buff[0], 59);
      Serial.println(buff);
  } else if (cmd[0] == 50 && cmdsize == 1) {  // "2" get alarm 1
      DS3231_get_a2(&buff[0], 59);
      Serial.println(buff);
  } else if (cmd[0] == 51 && cmdsize == 1) {  // "3" get aging register
      Serial.print("aging reg is ");
      Serial.println(DS3231_get_aging(), DEC);
  } else if (cmd[0] == 65 && cmdsize == 9) {  // "A" set alarm 1
      DS3231_set_creg(DS3231_INTCN | DS3231_A1IE);
      //ASSMMHHDD
      for (i = 0; i < 4; i++) {
          time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // ss, mm, hh, dd
      }
      uint8_t flags[5] = { 0, 0, 0, 0, 0 };
      DS3231_set_a1(time[0], time[1], time[2], time[3], flags);
      DS3231_get_a1(&buff[0], 59);
      Serial.println(buff);
  } else if (cmd[0] == 66 && cmdsize == 7) {  // "B" Set Alarm 2
      DS3231_set_creg(DS3231_INTCN | DS3231_A2IE);
      //BMMHHDD
      for (i = 0; i < 4; i++) {
          time[i] = (cmd[2 * i + 1] - 48) * 10 + cmd[2 * i + 2] - 48; // mm, hh, dd
      }
      uint8_t flags[5] = { 0, 0, 0, 0 };
      DS3231_set_a2(time[0], time[1], time[2], flags);
      DS3231_get_a2(&buff[0], 59);
      Serial.println(buff);
  } else if (cmd[0] == 67 && cmdsize == 1) {  // "C" - get temperature register
      Serial.print("temperature reg is ");
      Serial.println(DS3231_get_treg(), DEC);
  } else if (cmd[0] == 68 && cmdsize == 1) {  // "D" - reset status register alarm flags
      reg_val = DS3231_get_sreg();
      reg_val &= B11111100;
      DS3231_set_sreg(reg_val);
  } else if (cmd[0] == 70 && cmdsize == 1) {  // "F" - custom fct
      reg_val = DS3231_get_addr(0x5);
      Serial.print("orig ");
      Serial.print(reg_val,DEC);
      Serial.print("month is ");
      Serial.println(bcdtodec(reg_val & 0x1F),DEC);
  } else if (cmd[0] == 71 && cmdsize == 1) {  // "G" - set aging status register
      DS3231_set_aging(0);
  } else if (cmd[0] == 83 && cmdsize == 1) {  // "S" - get status register
      Serial.print("status reg is ");
      Serial.println(DS3231_get_sreg(), DEC);
  } else {
      Serial.print("unknown command prefix ");
      Serial.println(cmd[0]);
      Serial.println(cmd[0], DEC);
  }
}
