//////////////////////////////////////////////
//   StrikeComputer with OLED Display		//
//   Ver 0.15								//
//											//
//   Sergey Bondarenko						//
//											//
//	 Board :		Arduino Micro			//
//	 OLED Driver :	SH1106					//
//////////////////////////////////////////////

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <TimeLib.h>
#include <DS1307RTC.h>

// Display setup -------------------------------------------------------

#define OLED_RESET 4
Adafruit_SH1106 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#if (SH1106_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SH1106.h!");
#endif

//----------------------------------------------------------------------

#define DEBUG_MODE false

// Pins setup ----------------------------------------------------------

#define photocellPin 0
#define Button1Pin 5
#define Button2Pin 6
#define Button3Pin 7

// Some constants ( No need to change this ) ----------------------------

#define ButtonUpdateRate 80

#define BBsInMagLimit 990
#define BBsCountLimit 9999
#define ThresholdLimit 20
#define BBLowPercentLimit 52  // (Max % + 2)

#define CalibrationMeasurements 10

// Variables ------------------------------------------------------------

int laserValue = 0;				// Датчик шаров
int laserAmbient = 250;
int laserCompareInterval = 5;

int bbMagFull = 150;			// Переменные для счета шаров
int bbMagCurrent = 150;
int bbShotCount = 0;
int bbLowPercent = 20;

bool isReloading = false;

byte buttons = B10000000;		// Нажатие кнопок

int Page = 0;					// Меню
int CurrentMenuID = 0;
int SelectedMenuObjectID = 0;

tmElements_t tm;				// Время
tmElements_t tmSet;
int TimeSelection = 0;




// Setup ------------------------------------------------------------------------------------------------------------

void setup()   {
  
  //Настройка последовательного порта
  Serial.begin(115200);
  
  //Настройка прерывания для кнопки
  pinMode(Button1Pin, INPUT);
  pinMode(Button2Pin, INPUT);
  pinMode(Button3Pin, INPUT);
  
  //Настройка и инициализация экрана
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  
  display.clearDisplay();
  display.display();

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);

  //Калибровка датчика
  display.clearDisplay();
  Draw_BBCalibrateCheck();
  UpdateDisplay();
  BBCalibrateCheck();
  delay(500);
  display.clearDisplay();
  Draw_BBCalibrateCheckDone();
  UpdateDisplay();
  delay(2000);
  
  //Переход в главное меню
  CurrentMenuID = 0;
}



// Loop ------------------------------------------------------------------------------------------------------------

void loop() {
  
  if (CurrentMenuID == 0){
     MainMenu();			// Меню
     ButtonsCheck();
  }
  
  if (CurrentMenuID == 1){
     BBCheckMagShot();		// Счетчик шаров в магазине
     ButtonsCheck();
  }

  if (CurrentMenuID == 2){
     BBCheckShot();			// Счетчик шаров с нуля
     ButtonsCheck();
  }

  if (CurrentMenuID == 3){
     Settings();			// Настройки
     ButtonsCheck();
  }

  if (CurrentMenuID == 4) {
    SetThreshold();			// Настройка порогового значения датчика
    ButtonsCheck();
  }

  if (CurrentMenuID == 5) {
	  SetBBsInMag();		// Настройка шаров в магазине
	  ButtonsCheck();
  }

  if (CurrentMenuID == 6) {
	  SetTime();			// Настройка времени
	  ButtonsCheck();
  }

  if (CurrentMenuID == 7) {
	  SetLowBBPercent();	// Настройка низкого процента шаров в магазине
	  ButtonsCheck();
  }

}



// Кнопки, дисплей и другие функции ----------------------------------------------------------------------------------------

void ButtonsCheck(){
    if ( digitalRead(Button1Pin) == HIGH ){
      buttons = buttons | B00000001;
    }else{
      buttons = buttons & B11111110;
    }
    if ( digitalRead(Button2Pin) == HIGH ){
      buttons = buttons | B00000010;
    }else{
      buttons = buttons & B11111101;
    }
    if ( digitalRead(Button3Pin) == HIGH ){
      buttons = buttons | B00000100;
    }else{
      buttons = buttons & B11111011;
    }
}


void UpdateDisplay() {  //Обновление экрана
  display.display();
}


void Print2digits(int number) {			// Вывод двух чисел (пример: 01, 02, 03, ... , 10, 11, ...)
	if (number >= 0 && number < 10) {
		display.print("0");
	}
	display.print(number);
}


// Счетчик шаров ------------------------------------------------------------------------------------------------------------

void BBCalibrateCheck(){                         // Калибровка датчика
  for (int i = 0; i < CalibrationMeasurements; i++){                  // 10 раз измеряем значения датчика и находим среднее
    laserAmbient += analogRead(photocellPin);
  }
  laserAmbient = laserAmbient / CalibrationMeasurements;

  if (DEBUG_MODE) { Serial.println("Calibrated!    Value = "); Serial.println(laserAmbient); }
}


void BBCheckMagShot() {             // Счетчик шаров в магазине
  
   // Работа с датчиком
   laserValue = analogRead(photocellPin);
   
   if (laserValue > laserAmbient + laserCompareInterval){      // Обнаружение шара
      if (DEBUG_MODE){ Serial.print("Detected!   laserValue: "); Serial.println(laserValue); }
      if(bbMagCurrent > 0){        // Если есть шары, вычитаем
         bbMagCurrent--;
         if (DEBUG_MODE){ Serial.print("SHOT!   BBs left: "); Serial.println(bbMagCurrent); Serial.println(); }
      }  
      // Обновление экрана
      display.clearDisplay();
      Draw_Header();
      Draw_BBMagCount();
      UpdateDisplay(); 
   }
   
   // Проверка кнопок
   if (buttons == B10000001){
      bbMagCurrent = bbMagFull;
      if (DEBUG_MODE){ Serial.print("Reloaded!\n\n"); }
      display.clearDisplay();
      Draw_Header();
      Draw_BBMagCount();
      UpdateDisplay();
      delay(200);
   }
   
   if (buttons == B10000010){
      CurrentMenuID = 0;
      SelectedMenuObjectID = 0;
      delay(200);
   }
   
   if (buttons == B10000100){
     
   }
   
}



void BBCheckShot() {                     // Счетчик шаров с нуля
  
   // Работа с датчиком
   laserValue = analogRead(photocellPin);
   
   if (laserValue > laserAmbient + laserCompareInterval){        // Обнаружение шара
      if (DEBUG_MODE){ Serial.print("Detected!   laserValue: "); Serial.println(laserValue); } 
      bbShotCount++;
	  if (bbShotCount > BBsCountLimit) bbShotCount = BBsCountLimit;
      if (DEBUG_MODE){ Serial.print("SHOT!   BBs count: "); Serial.println(bbShotCount); Serial.println(); }
      //Обновление экрана
      display.clearDisplay();
      Draw_Header();
      Draw_BBShotCount();
      UpdateDisplay(); 
   }
   
   // Проверка кнопок
   if (buttons == B10000001){
      bbShotCount = 0;
      if (DEBUG_MODE){ Serial.print("Set to 0\n\n"); }
      display.clearDisplay();
      Draw_Header();
      Draw_BBShotCount();
      UpdateDisplay();
   }
   
   if (buttons == B10000010){
      CurrentMenuID = 0;
      SelectedMenuObjectID = 0;
      delay(200);
   }
   
   if (buttons == B10000100){
     
   }
   
}



void SetThreshold() {

	if (buttons == B10000100) {
		laserCompareInterval++;
		if (laserCompareInterval > ThresholdLimit) laserCompareInterval = ThresholdLimit;
	}
	if (buttons == B10000010) {
		CurrentMenuID = 3;
		SelectedMenuObjectID = 0;
		Page = 0;
		delay(200);
	}
	if (buttons == B10000001) {
		laserCompareInterval--;
		if (laserCompareInterval < 1) laserCompareInterval = 1;
	}

  display.clearDisplay();
  Draw_Header();
  Draw_SetThreshold();
  UpdateDisplay();

}


void SetBBsInMag() {			// Настройка шаров в магазине

	if (buttons == B10000100) {
		bbMagFull += 10;
		if (bbMagFull > BBsInMagLimit) bbMagFull = BBsInMagLimit;
	}
	if (buttons == B10000010) {
		CurrentMenuID = 3;
		SelectedMenuObjectID = 0;
		Page = 0;
		delay(200);
	}
	if (buttons == B10000001) {
		bbMagFull -= 10;
		if (bbMagFull < 10) bbMagFull = 10;
	}

	display.clearDisplay();
	Draw_Header();
	Draw_SetBBsInMag();
	UpdateDisplay();
}


void SetLowBBPercent() {

	if (buttons == B10000100) {
		bbLowPercent += 2;
		if (bbLowPercent > BBLowPercentLimit) bbLowPercent = BBLowPercentLimit;
	}
	if (buttons == B10000010) {
		CurrentMenuID = 3;
		SelectedMenuObjectID = 0;
		Page = 0;
		delay(200);
	}
	if (buttons == B10000001) {
		bbLowPercent -= 2;
		if (bbLowPercent < 2) bbLowPercent = 2;
	}

	display.clearDisplay();
	Draw_Header();
	Draw_SetLowBBPercent();
	UpdateDisplay();
}


void SetTime() {			// Настройка времени

	if (buttons == B10000100) {
		if (TimeSelection == 0) {
			tmSet.Hour++;
		}
		else if (TimeSelection == 1) {
			tmSet.Minute++;
		}
	}
	if (buttons == B10000010) {
		TimeSelection++;
		if (TimeSelection == 3) {
			RTC.write(tmSet);
			CurrentMenuID = 3;
		}
		delay(200);
	}
	if (buttons == B10000001) {
		if (TimeSelection == 0) {
			tmSet.Hour--;
		}
		else if (TimeSelection == 1) {
			tmSet.Minute--;
		}
	}

	display.clearDisplay();
	Draw_Header();
	Draw_SetTime();
	UpdateDisplay();
}



// Меню ------------------------------------------------------------------------------------------------------------

void MainMenu(){  

  if (buttons == B10000100){
    SelectedMenuObjectID++;
    if (SelectedMenuObjectID > 2) SelectedMenuObjectID = 2;
	delay(ButtonUpdateRate);
  }else if (buttons == B10000001) {
    SelectedMenuObjectID--;
    if (SelectedMenuObjectID < 0) SelectedMenuObjectID = 0;
	delay(ButtonUpdateRate);
  }
    
    display.setTextSize(1);
    display.clearDisplay();
    Draw_Header();

	display.drawRect(0, SelectedMenuObjectID * 10 + 10, 128, 11, WHITE);


      display.setCursor(2, 12);
      display.setTextColor(WHITE, BLACK);
      display.print("BB MAG COUNTER");
  
      display.setCursor(2, 22);
      display.setTextColor(WHITE, BLACK);
      display.print("BB COUNTER");

      display.setCursor(2, 32);
      display.setTextColor(WHITE, BLACK);
      display.print("SETTINGS");
      
       if (SelectedMenuObjectID == 0){
         if (buttons == B10000010){
            CurrentMenuID = 1;
            //Обновление экрана
            display.clearDisplay();
            Draw_Header();
            Draw_BBMagCount();
            UpdateDisplay(); 
            delay(200);
         }
       }
       if (SelectedMenuObjectID == 1){
         if (buttons == B10000010){
            CurrentMenuID = 2;
            //Обновление экрана
            display.clearDisplay();
            Draw_Header();
            Draw_BBShotCount();
            UpdateDisplay(); 
            delay(200);
         }
       }
       if (SelectedMenuObjectID == 2){
         if (buttons == B10000010){
            CurrentMenuID = 3;
			Page = 0;
            SelectedMenuObjectID = 0;
            delay(200);
         }
       }

  UpdateDisplay();

}



// Настройки ------------------------------------------------------------------------------------------------------------

void Settings(){

  if (buttons == B10000100){
    if (Page == 0){
        SelectedMenuObjectID++;
        if (SelectedMenuObjectID > 4) { SelectedMenuObjectID = 0; Page = 1; }
		delay(ButtonUpdateRate);
    }else if (Page == 1) {
		SelectedMenuObjectID++;
		if (SelectedMenuObjectID > 1) { SelectedMenuObjectID = 1; }
		delay(ButtonUpdateRate);
    }
  }else if (buttons == B10000001) {
    if (Page == 0){
        SelectedMenuObjectID--;
        if (SelectedMenuObjectID < 0) { SelectedMenuObjectID = 0; } 
		delay(ButtonUpdateRate);
    }else if (Page == 1) {
        SelectedMenuObjectID--;
        if (SelectedMenuObjectID < 0) { SelectedMenuObjectID = 4; Page = 0; }
		delay(ButtonUpdateRate);
    }
  }
    
    display.setTextSize(1);
    display.clearDisplay();
    Draw_Header();

    if (Page == 0){

      display.drawRect(124, 10, 4, 54, WHITE);
      display.drawRect(125, 33, 2, 30, WHITE);

	  display.drawRect(0, SelectedMenuObjectID * 10 + 10, 124, 11, WHITE);


      display.setCursor(2, 12);
      display.setTextColor(WHITE, BLACK);
      display.print("SET THRESHOLD");
  
      display.setCursor(2, 22);
      display.setTextColor(WHITE, BLACK);
      display.print("SET BBs IN MAG");

	  display.setCursor(2, 32);
	  display.setTextColor(WHITE, BLACK);
	  display.print("SET BBs LOW PERCENT");

      display.setCursor(2, 42);
      display.setTextColor(WHITE, BLACK);
      display.print("CALIBRATE");

      display.setCursor(2, 52);
      display.setTextColor(WHITE, BLACK);
      display.print("SET TIME");

      
      
       if (SelectedMenuObjectID == 0){
         if (buttons == B10000010){
			 CurrentMenuID = 4;
			 delay(200);
         }
       }
       if (SelectedMenuObjectID == 1){
         if (buttons == B10000010){
			 CurrentMenuID = 5;
			 delay(200);
         }
       }
	   if (SelectedMenuObjectID == 2) {
		   if (buttons == B10000010) {
			   CurrentMenuID = 7;
			   delay(200);
		   }
	   }
       if (SelectedMenuObjectID == 3){
         if (buttons == B10000010){
			 //Калибровка датчика
			 display.clearDisplay();
			 Draw_Header();
			 Draw_BBCalibrateCheck();
			 UpdateDisplay();
			 BBCalibrateCheck();
			 delay(500);
			 display.clearDisplay();
			 Draw_Header();
			 Draw_BBCalibrateCheckDone();
			 UpdateDisplay();
			 delay(2000);
         }
       }
       if (SelectedMenuObjectID == 4){
         if (buttons == B10000010){

			 if (!RTC.read(tm)) {
				 if (RTC.chipPresent()) {
					 tm = {0, 0, 15, 1, 1, 1, 48};
					 RTC.write(tm);
				 }
			 }

			 tmSet = tm;
			 TimeSelection = 0;
			 CurrentMenuID = 6;
			 delay(200);
         }
       }
       

       
    }

    if (Page == 1){

    display.drawRect(124, 10, 4, 54, WHITE);
    display.drawRect(125, 11, 2, 30, WHITE);

	display.drawRect(0, SelectedMenuObjectID * 10 + 10, 124, 11, WHITE);

		display.setCursor(2, 12);
		display.setTextColor(WHITE, BLACK);
		display.print("VERSION");

        display.setCursor(2, 22);
        display.setTextColor(WHITE, BLACK);
        display.print("<-  BACK");

        
		if (SelectedMenuObjectID == 0) {
			if (buttons == B10000010) {
				display.clearDisplay();
				Draw_Header();
				Draw_Version();
				UpdateDisplay();
				delay(2000);
			}
		}
		if (SelectedMenuObjectID == 1) {
			if (buttons == B10000010) {
				CurrentMenuID = 0;
				SelectedMenuObjectID = 0;
				delay(200);
			}
		}
    }

  UpdateDisplay();

}




// Функции отрисовки ------------------------------------------------------------------------------------------------------------

void Draw_Header() {          // Верхняя часть экрана
	// Настройки для секции
	display.setTextSize(1);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(0, 0);
	display.print("TEXT");
	// Время
	display.setCursor(98, 0);
	if (RTC.read(tm)) {
		Print2digits(tm.Hour);
		display.print(":");
		Print2digits(tm.Minute);
	}else {
		if (RTC.chipPresent()) {
			display.print("ERR0");
			if (DEBUG_MODE) {
				Serial.println("The DS1307 is stopped.  Please set time");
				Serial.println("to initialize the time and begin running.");
				Serial.println();
			}
		}
		else {
			if (DEBUG_MODE) {
				display.print("ERR1");
				Serial.println("DS1307 read error!  Please check the circuitry.");
				Serial.println();
			}
		}
	}

	display.drawFastHLine(0,9,128,WHITE);
}


void Draw_BBMagCount(){         // Экран счета шаров в магазине
	// Настройки для секции
	display.setTextSize(5);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(2, 22);
	display.print(bbMagCurrent);
	display.setTextSize(2);
	display.setCursor(90, 47);
	display.print("/");
	display.setTextSize(1);
	display.setCursor(100, 55);
	display.print(bbMagFull);

	if (bbLowPercent != BBLowPercentLimit) {
		if (bbMagCurrent <= ((float)bbMagFull / 100) * bbLowPercent) {
			display.fillRect(0, 10, 128, 54, INVERSE);
		}
	}
}


void Draw_BBShotCount(){          // Экран счета шаров с нуля
	// Настройки для секции
	display.setTextSize(5);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(2, 22);
	display.print(bbShotCount);
}


void Draw_BBCalibrateCheck(){       // Экран калибровки
	// Настройки для секции
	display.setTextSize(1);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(0, 30);
	display.print("CALIBRATING...");
}


void Draw_BBCalibrateCheckDone(){     // Экран завершенной калибровки
	// Настройки для секции
	display.setTextSize(1);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(0, 30);
	display.print("CALIBRATION DONE");
	display.setCursor(0, 45);
	display.print("Value = "); display.print(laserAmbient);
}


void Draw_Version() {           // Экран версии ПО
	// Настройки для секции
	display.setTextSize(1);
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setCursor(0, 20);
	display.print("StrikeComputer");
	display.setCursor(0, 30);
	display.print("Version 0.15");
	display.setCursor(0, 40);
	display.print("01.08.2018");
}


void Draw_SetThreshold() {          // Экран настройки порогового значения
	// Настройки для секции
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setTextSize(1);
	display.setCursor(2, 12);
	display.print("> SET THRESHOLD");
	display.setTextSize(3);
	display.setCursor(2, 35);
	display.print("<     >");
	display.setTextSize(4);
	display.setCursor(30, 31);
	display.print(laserCompareInterval);
}


void Draw_SetBBsInMag() {         // Экран настройки шаров в магазине
	// Настройки для секции
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setTextSize(1);
	display.setCursor(2, 12);
	display.print("> SET BBs IN MAG");
	display.setTextSize(3);
	display.setCursor(2, 35);
	display.print("<     >");
	display.setTextSize(4);
	display.setCursor(30, 31);
	display.print(bbMagFull);
}


void Draw_SetLowBBPercent() {
	// Настройки для секции
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setTextSize(1);
	display.setCursor(2, 12);
	display.print("> SET BBs LOW PERCENT");
	display.setTextSize(3);
	display.setCursor(2, 35);
	display.print("<     >");
	display.setTextSize(4);
	display.setCursor(30, 31);
	if (bbLowPercent != BBLowPercentLimit) {
		display.print(bbLowPercent);
	}
	else {
		display.print("OFF");
	}	
}


void Draw_SetTime() {		// Экран настройки шаров в магазине
	// Настройки для секции
	display.setTextColor(WHITE, BLACK);
	// Отрисовка
	display.setTextSize(1);
	display.setCursor(2, 12);
	display.print("> SET TIME");

	display.setTextSize(1);
	display.setCursor(5, 30);
	Print2digits(tmSet.Hour);
	display.print(" : ");
	Print2digits(tmSet.Minute);
	display.print("   ");
	display.print("OK");

	switch (TimeSelection)
	{
	case 0:
		display.drawRect(2, 27, 17, 13, WHITE);
		break;
	case 1:
		display.drawRect(32, 27, 17, 13, WHITE);
		break;
	case 2:
		display.drawRect(62, 27, 17, 13, WHITE);
		break;
	}
 
}



