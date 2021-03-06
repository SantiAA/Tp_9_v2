#include "FTDIHandler.h"

#include <chrono>

#include "ErrLCD.h"
#include "delay.h"

#define ENABLE_0 0xFE //este tengo que hacer un and para usarlo 
#define ENABLE_1 0x01 // este lo uso con or
#define RS_DR 0x02 // se usa con or
#define RS_IR 0xFD // se usa con and

#define BITMODE8 0x30
#define BITMODE4 0x20 
#define LINES2AND5x8 0x08
#define DIPLAYONOFF 0x0C
#define CLEAR 0x01
#define ENTRYMODE 0x06
#define RETHOME 0x02

#define CONNECTING_TIME 5 //en segundos

FTDIHandler::FTDIHandler()
{
	FT_STATUS status = !FT_OK;
	DWORD sizeSent = 0;

	/*The display has a settling time after the physical connection so the attempt to connect will be done for a few seconds*/
	std::chrono::seconds MaxTime(CONNECTING_TIME);

	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> current = start;

	while (status != FT_OK && ((current - start) < MaxTime))//loop till succesful connection o max connecting time is exceeded
	{
		status = FT_Open(1, &deviceHandler);

		if (status == FT_OK)
		{
			UCHAR Mask = 0xFF;	//Selects all FTDI pins.
			UCHAR Mode = 1; 	// Set asynchronous bit-bang mode
			if (FT_SetBitMode(deviceHandler, Mask, Mode) == FT_OK)	// Sets LCD as asynch bit mode. Otherwise it doesn't work.
			{
				/*Set 4 bits mode */
				try
				{
					set4BitsMode();
					lcdWriteIR(BITMODE4 | LINES2AND5x8); // modo 4 buts , 2 lineas , display 5x8
					delay(1);
					lcdWriteIR(DIPLAYONOFF); // prendo el display
					delay(1); 
					lcdWriteIR(CLEAR); // limpio y mando el cursor al adress 0
					delay(10);
					lcdWriteIR(ENTRYMODE); // Mueve el cursor despues de cada DR y NO!!! shiftea el display
					delay(1);
					lcdWriteIR(RETHOME); // cursor al adress 0
					delay(2);
				}
				catch ( ErrrType type)
				{
					if (type == ErrrType::LCD_NO_ESCRIBE)
					{
						throw ErrrType::LCD_CHAGE_MODE_ERROR;
					}
					//not abel to set 4 bit mode trow exeption
				}
			}
			else
			{
				throw ErrrType::LCD_SETTING_ERROR;
				//not able to set asynchronous mode trow exeption	
			}
		}
		current = std::chrono::system_clock::now();
	}

	if (status != FT_OK)
	{
		throw ErrrType::LCD_NOT_FOUND;
		//trow exeption unable to open the display
	}
	
}

void FTDIHandler::lcdWriteIR(uint8_t valor)
{
	uint8_t temp =  ((valor & 0xF0) | ENABLE_1 & RS_IR ) ; // primero escribo el nibble mas significativo 
	lcdWriteNibble(temp);									// con el RS en 0
	temp = (((valor & 0x0F) << 4) | ENABLE_1 & RS_IR); // despues el menos significativo
	lcdWriteNibble(temp);
}

void FTDIHandler::lcdWriteDR(uint8_t valor)
{
	uint8_t temp = ((valor & 0xF0) | ENABLE_1 | RS_DR);  // mas significativo primero, con RS en 1
	lcdWriteNibble(temp);
	temp = (((valor & 0x0F) << 4) | ENABLE_1 | RS_DR);
	lcdWriteNibble(temp);
}


FTDIHandler::~FTDIHandler()
{
	lcdWriteIR(CLEAR); // limpio el display antes de cerrarlo
	FT_Close(deviceHandler); 
}

void FTDIHandler::lcdWriteNibble(uint8_t value )
{
	FT_STATUS status = !FT_OK ;
	DWORD bytesWriten;
	uint8_t temp;

	temp = value & ENABLE_0;
	status = FT_Write(deviceHandler, &temp , 1 , &bytesWriten); // dejo que se estabilice los datos y el RS
	if (status != FT_OK)
	{
		throw ErrrType::LCD_NO_ESCRIBE;
	}
	status = !FT_OK;
	delay(2); // tiempo en milisegundos

	temp = value | ENABLE_1;
	status = FT_Write(deviceHandler, &temp , 1, &bytesWriten); // ENABLE en 1 para que tome los datos
	if (status != FT_OK)
	{
		throw ErrrType::LCD_NO_ESCRIBE;
		//throw exepction 
	}
	status = !FT_OK;
	delay(2); // tiempo en milisegundos
	
	temp = value & ENABLE_0;
	status = FT_Write(deviceHandler, &temp , 1, &bytesWriten); //Deja de tomar datos
	if (status != FT_OK)
	{
		throw ErrrType::LCD_NO_ESCRIBE;
		//throw exepction 
	}
	status = !FT_OK;
	delay(1); // tiempo en milisegundos


}

void FTDIHandler::set4BitsMode(void)
{

	lcdWriteNibble(BITMODE8);  //le mando modo 8 bits
	delay(5); // tiempo en milisegundos

	lcdWriteNibble(BITMODE8);
	delay(1); // tiempo en milisegundos

	lcdWriteNibble(BITMODE8);
	delay(1); // tiempo en milisegundos

	lcdWriteNibble(BITMODE4);  // modo 4 bits, a partir de ahora trabajo en modo 4 bits
	delay(1); // tiempo en ms

}
