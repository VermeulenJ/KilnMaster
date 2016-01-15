#define DEBUG
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "LedControl.h"
#include "Adafruit_MAX31855.h"
#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68

//aansluiting van de externe led blokken
/*
 pin 22 is connected to the DataIn
 pin 24 is connected to the CLK
 pin 26 is connected to LOAD
 2 is het aantal schermen die ik aanstuur
 */
LedControl lc = LedControl(22, 24, 26, 2);

//Aansluiting van de thermocouple
#define MAXDO   28
#define MAXCS   30
#define MAXCLK  32
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0

// Assign human-readable names to some common 16-bit color values:
#define  BLACK   0xFFFF
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0x0000

// When using the BREAKOUT BOARD only, use these 8 data lines to the LCD:
// For the Arduino Uno, Duemilanove, Diecimila, etc.:
//   D0 connects to digital pin 8  (Notice these are
//   D1 connects to digital pin 9   NOT in order!)
//   D2 connects to digital pin 2
//   D3 connects to digital pin 3
//   D4 connects to digital pin 4
//   D5 connects to digital pin 5
//   D6 connects to digital pin 6
//   D7 connects to digital pin 7
// For the Arduino Mega, use digital pins 22 through 29
// (on the 2-row header at the end of the board).

// For Arduino Uno/Duemilanove, etc
//  connect the SD card with DI going to pin 11, DO going to pin 12 and SCK going to pin 13 (standard)
//  Then pin 10 goes to CS (or whatever you have set up)
#if defined __AVR_ATmega2560__
#define SD_SCK 13
#define SD_MISO 12
#define SD_MOSI 11

#endif
#define SD_CS 10     // Set the chip select line to whatever you use (10 doesnt conflict with the library)

File dataFile;

// our TFT wiring
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, A4);

// declareer de output functie
void schrijfCijfer(int cijfer);


// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val)
{
	return( (val / 10 * 16) + (val % 10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val)
{
	return( (val / 16 * 10) + (val % 16) );
}


void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
                   dayOfMonth, byte month, byte year)
{
	// sets time and date data to DS3231
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); // set next input to start at the seconds register
	Wire.write(decToBcd(second)); // set seconds
	Wire.write(decToBcd(minute)); // set minutes
	Wire.write(decToBcd(hour)); // set hours
	Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
	Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
	Wire.write(decToBcd(month)); // set month
	Wire.write(decToBcd(year)); // set year (0 to 99)
	Wire.endTransmission();
}



void setup()
{
	tft.reset();
//this sketch force the  ILI9341 LCD driver chage the next code line to chage the driver
//0x9325  ILI9325 LCD driver
//0x9328  ILI9328 LCD driver
//0x7575  HX8347G LCD driver
//0x9341  ILI9341 LCD driver
//0x8357  HX8357D LCD driver
	tft.begin(0x8357);
	tft.setTextSize(2);
	tft.fillScreen(0);
//  tft.begin(0x9341);
	tft.setRotation(1);
#if defined __AVR_ATmega2560__
	if (!SD.begin(SD_CS, SD_MOSI, SD_MISO, SD_SCK ))
	{
		tft.println(F("failed!"));
		return;
	}
#else
	if (!SD.begin(SD_CS))
	{
		tft.println(F("failed!"));
		return;
	}
#endif
	bmpDraw("start.bmp", 0, 0);
	//initialisatie van het led display
	int devices = lc.getDeviceCount();
	//iedere display initialiseren
	for(int address = 0; address < devices; address++)
	{
		//het scherm aanzetten; staat standaard uit
		lc.shutdown(address, false);
		//felheid minimaal zetten
		lc.setIntensity(address, 8);
		// scherm opschonen
		lc.clearDisplay(address);
	}

	// klok initialisatie
	Wire.begin();
	Serial.begin(9600);
	// set the initial time here:
	// DS3231 seconds, minutes, hours, day, date, month, year
	// setDS3231time(30,50,20,1,10,1,16);

	delay(2000);
	// status balk
	tft.setRotation(1);
	tft.fillScreen(BLACK);
	tft.fillRect(0, 0, 320, 17, WHITE);
	tft.setCursor(6, 2);
	tft.setTextColor(BLACK);
	tft.setTextSize(2);
	tft.println("  200 C  Kiln Master");
	tft.setCursor(250, 2);
	tft.setTextColor(GREEN);
	tft.setTextSize(2);
	tft.println("IDLE");

}



void readDS3231time(byte *second,
                    byte *minute,
                    byte *hour,
                    byte *dayOfWeek,
                    byte *dayOfMonth,
                    byte *month,
                    byte *year)
{
	Wire.beginTransmission(DS3231_I2C_ADDRESS);
	Wire.write(0); // set DS3231 register pointer to 00h
	Wire.endTransmission();
	Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
	// request seven bytes of data from DS3231 starting from register 00h
	*second = bcdToDec(Wire.read() & 0x7f);
	*minute = bcdToDec(Wire.read());
	*hour = bcdToDec(Wire.read() & 0x3f);
	*dayOfWeek = bcdToDec(Wire.read());
	*dayOfMonth = bcdToDec(Wire.read());
	*month = bcdToDec(Wire.read());
	*year = bcdToDec(Wire.read());
}

void loop()
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
	tft.setCursor(40, 50);
	tft.fillRect(40, 50, 50, 80, BLACK);
	// retrieve data from DS3231
	readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);
	char TimeToLog = (hour , DEC);
	if (minute < 10)
	{
		TimeToLog += 0;
	}
	TimeToLog += (minute, DEC);
	char TempToLog = thermocouple.readCelsius();
	tft.println("Time ");
	tft.println(hour , DEC);
	tft.println("Temp ");
	tft.println(TempToLog);
	schrijfCijfer(thermocouple.readCelsius());
	char stringLog = TimeToLog;
	stringLog += ':';
	stringLog += TempToLog;
	SavDL(stringLog);

	// Take 1 measurement every 500 milliseconds
	delay(500);
}



void SavDL(char datLog)
{

	// Open up the file we're going to log to!
	dataFile = SD.open("datalog.txt", FILE_WRITE);
	if (! dataFile)
	{
		tft.println("error opening datalog.txt");
		// Wait forever since we cant write data
		while (1) ;
	}
	dataFile.println(datLog);
	// The following line will 'save' the file to the SD card after every
	// line of data - this will use more power and slow down how much data
	// you can read but it's safer!
	// If you want to speed up the system, remove the call to flush() and it
	// will save the file only every 512 bytes - every time a sector on the
	// SD card is filled with data.
	dataFile.flush();
}


// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, int x, int y)
{

	File     bmpFile;
	int      bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t  bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t  sdbuffer[3 * BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
	uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
	uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean  goodBmp = false;       // Set to true on valid header parse
	boolean  flip    = true;        // BMP is stored bottom-to-top
	int      w, h, row, col;
	uint8_t  r, g, b;
	uint32_t pos = 0, startTime = millis();
	uint8_t  lcdidx = 0;
	boolean  first = true;

	if((x >= tft.width()) || (y >= tft.height())) return;
	if ((bmpFile = SD.open(filename)) == NULL)
	{
#ifdef DEBUG
		tft.println(F("File not found"));
#endif
		return;
	}

	// Parse BMP header
	if(read16(bmpFile) == 0x4D42)
	{
		// BMP signature
		read32(bmpFile);
		(void)read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		read32(bmpFile);

		bmpWidth  = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if(read16(bmpFile) == 1)   // # planes -- must be '1'
		{
			bmpDepth = read16(bmpFile); // bits per pixel
			if((bmpDepth == 24) && (read32(bmpFile) == 0))   // 0 = uncompressed
			{

				goodBmp = true; // Supported BMP format -- proceed!
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if(bmpHeight < 0)
				{
					bmpHeight = -bmpHeight;
					flip      = false;
				}

				// Crop area to be loaded
				w = bmpWidth;
				h = bmpHeight;
				if((x + w - 1) >= tft.width())  w = tft.width()  - x;
				if((y + h - 1) >= tft.height()) h = tft.height() - y;

				// Set TFT address window to clipped image bounds
				tft.setAddrWindow(x, y,  x + w - 1, y + h - 1);

				for (row = 0; row < h; row++) // For each scanline...
				{
					// Seek to start of scan line.  It might seem labor-
					// intensive to be doing this on every line, but this
					// method covers a lot of gritty details like cropping
					// and scanline padding.  Also, the seek only takes
					// place if the file position actually needs to change
					// (avoids a lot of cluster math in SD library).
					if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
					else     // Bitmap is stored top-to-bottom
						pos = bmpImageoffset + row * rowSize;
					if(bmpFile.position() != pos)   // Need seek?
					{
						bmpFile.seek(pos);
						buffidx = sizeof(sdbuffer); // Force buffer reload
					}

					for (col = 0; col < w; col++) // For each column...
					{
						// Time to read more pixel data?
						if (buffidx >= sizeof(sdbuffer))   // Indeed
						{
							// Push LCD buffer to the display first
							if(lcdidx > 0)
							{
								tft.pushColors(lcdbuffer, lcdidx, first);
								lcdidx = 0;
								first  = false;
							}
							bmpFile.read(sdbuffer, sizeof(sdbuffer));
							buffidx = 0; // Set index to beginning
						}

						// Convert pixel from BMP to TFT format
						b = sdbuffer[buffidx++];
						g = sdbuffer[buffidx++];
						r = sdbuffer[buffidx++];
						lcdbuffer[lcdidx++] = tft.color565(r, b, g);
					} // end pixel
				} // end scanline
				// Write any remaining data to LCD
				if(lcdidx > 0)
				{
					tft.pushColors(lcdbuffer, lcdidx, first);
				}

			} // end goodBmp
		}
	}

	bmpFile.close();
	if(!goodBmp) tft.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f)
{
	uint16_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read(); // MSB
	return result;
}

uint32_t read32(File f)
{
	uint32_t result;
	((uint8_t *)&result)[0] = f.read(); // LSB
	((uint8_t *)&result)[1] = f.read();
	((uint8_t *)&result)[2] = f.read();
	((uint8_t *)&result)[3] = f.read(); // MSB
	return result;
}


void schrijfCijfer(int cijfer)
{
	int devices = lc.getDeviceCount();
	int colstart, colstart2;

// definitie van de tekenset
	byte teken[10][3] = {{B01111110, B01000010, B01111110}, //0
		{B01000010, B01111110, B01000000}, //1
		{B01110010, B01010010, B01011110}, //2
		{B01001010, B01001010, B01111110}, //3
		{B00001110, B00001000, B01111110}, //4
		{B01001110, B01001010, B01111010}, //5
		{B01111110, B01010010, B01110010}, //6
		{B00000010, B00000010, B01111110}, //7
		{B01111110, B01011010, B01111110}, //8
		{B01001110, B01001010, B01111110}
	}; //9


	// nummers verwijderen voordat de volgende weer geplaatst wordt
	for(int address = 0; address < devices; address++)
	{
		colstart = 0;
		for(int col = colstart; col < 8; col++)
		{
			lc.setRow(address, col, B00000000);
		}
	}

	// teken getal 1
	for(int col = 0; col < 3; col++)
	{
		lc.setRow(0, col, teken[((int(cijfer / 1000) % 10))][col]);
	}
	// teken getal 2
	for(int col = 4; col < 7; col++)
	{
		lc.setRow(0, col, teken[((int(cijfer / 100) % 10))][col - 4]);
	}
	// teken getal 3
	for(int col = 0; col < 3; col++)
	{
		lc.setRow(1, col, teken[((int(cijfer / 10) % 10))][col]);
	}
	// teken getal 4
	for(int col = 4; col < 7; col++)
	{
		lc.setRow(1, col, teken[((int(cijfer / 1) % 10))][col - 4]);
	}
}


void displayTime()
{
	byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
	// retrieve data from DS3231
	readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
	               &year);
	// send it to the serial monitor
	Serial.print(hour, DEC);
	// convert the byte variable to a decimal number when displayed
	Serial.print(":");
	if (minute < 10)
	{
		Serial.print("0");
	}
	Serial.print(minute, DEC);
	Serial.print(":");
	if (second < 10)
	{
		Serial.print("0");
	}
	Serial.print(second, DEC);
	Serial.print(" ");
	Serial.print(dayOfMonth, DEC);
	Serial.print("/");
	Serial.print(month, DEC);
	Serial.print("/");
	Serial.print(year, DEC);
	Serial.print(" Day of week: ");
	switch(dayOfWeek)
	{
		case 1:
			Serial.println("Sunday");
			break;
		case 2:
			Serial.println("Monday");
			break;
		case 3:
			Serial.println("Tuesday");
			break;
		case 4:
			Serial.println("Wednesday");
			break;
		case 5:
			Serial.println("Thursday");
			break;
		case 6:
			Serial.println("Friday");
			break;
		case 7:
			Serial.println("Saturday");
			break;
	}
}
