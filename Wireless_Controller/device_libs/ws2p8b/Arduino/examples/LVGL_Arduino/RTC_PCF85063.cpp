#include "RTC_PCF85063.h"

datetime_t datetime= {0};

static uint8_t decToBcd(int val);
static int bcdToDec(uint8_t val);

const unsigned char MonthStr[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov","Dec"};


void RTC_Loop(void *arg);
/******************************************************************************
function:	PCF85063 initialized
parameter:
            
Info:Initiate Normal Mode, RTC Run, NO reset, No correction , 24hr format, Internal load capacitane 12.5pf
******************************************************************************/
void PCF85063_Init()
{
	uint8_t Value = RTC_CTRL_1_DEFAULT|RTC_CTRL_1_CAP_SEL;

	I2C_Write(PCF85063_ADDRESS, RTC_CTRL_1_ADDR, &Value, 1);
  I2C_Read(PCF85063_ADDRESS, RTC_CTRL_1_ADDR,  &Value, 1);
	if(Value & RTC_CTRL_1_STOP)
		printf("PCF85063 failed to be initialized.state :%d\r\n",Value);
	else
		printf("PCF85063 is running,state :%d\r\n",Value);

}

void RTC_Loop(void)
{
  PCF85063_Read_Time(&datetime);
}
/******************************************************************************
function:	Reset PCF85063
parameter:
Info:		
******************************************************************************/
void PCF85063_Reset()
{
	uint8_t Value = RTC_CTRL_1_DEFAULT|RTC_CTRL_1_CAP_SEL|RTC_CTRL_1_SR;
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_CTRL_1_ADDR, &Value, 1);
	if(ret != ESP_OK)
		printf("PCF85063 : Reset failure\r\n");
}
/******************************************************************************
function:	Set Time 
parameter:
Info:		
******************************************************************************/
void PCF85063_Set_Time(datetime_t time)
{
	uint8_t buf[3] = {decToBcd(time.second),
					  decToBcd(time.minute),
					  decToBcd(time.hour)};
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_SECOND_ADDR, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Time setting failure\r\n");
}

/******************************************************************************
function:	Set Date
parameter:
Info:		
******************************************************************************/
void PCF85063_Set_Date(datetime_t date)
{
	uint8_t buf[4] = {decToBcd(date.day),
					  decToBcd(date.dotw),
					  decToBcd(date.month),
					  decToBcd(date.year - YEAR_OFFSET)};
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_DAY_ADDR, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Date setting failed\r\n");
}

/******************************************************************************
function:	Set Time And Date
parameter:
Info:		
******************************************************************************/
void PCF85063_Set_All(datetime_t time)
{
	uint8_t buf[7] = {decToBcd(time.second),
					  decToBcd(time.minute),
					  decToBcd(time.hour),
					  decToBcd(time.day),
					  decToBcd(time.dotw),
					  decToBcd(time.month),
					  decToBcd(time.year - YEAR_OFFSET)};
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_SECOND_ADDR, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Failed to set the date and time\r\n");
}

/******************************************************************************
function:	Read Time And Date
parameter:
Info:		
******************************************************************************/
void PCF85063_Read_Time(datetime_t *time)
{
	uint8_t buf[7] = {0};
	esp_err_t ret = I2C_Read(PCF85063_ADDRESS, RTC_SECOND_ADDR, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Time read failure\r\n");
	else{
		time->second = bcdToDec(buf[0] & 0x7F);
		time->minute = bcdToDec(buf[1] & 0x7F);
		time->hour = bcdToDec(buf[2] & 0x3F);
		time->day = bcdToDec(buf[3] & 0x3F);
		time->dotw = bcdToDec(buf[4] & 0x07);
		time->month = bcdToDec(buf[5] & 0x1F);
		time->year = bcdToDec(buf[6]) + YEAR_OFFSET;
	}
}

/******************************************************************************
function:	Enable Alarm and Clear Alarm flag
parameter:			
Info:		
******************************************************************************/
void PCF85063_Enable_Alarm()
{
	uint8_t Value = RTC_CTRL_2_DEFAULT | RTC_CTRL_2_AIE;
	Value &= ~RTC_CTRL_2_AF;
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_CTRL_2_ADDR, &Value, 1);
	if(ret != ESP_OK)
		printf("PCF85063 : Failed to enable Alarm Flag and Clear Alarm Flag \r\n");
}

/******************************************************************************
function:	Get Alarm flag
parameter:			
Info:		
******************************************************************************/
uint8_t PCF85063_Get_Alarm_Flag()
{
	uint8_t Value = 0;
	esp_err_t ret = I2C_Read(PCF85063_ADDRESS, RTC_CTRL_2_ADDR, &Value, 1);
	if(ret != ESP_OK)
		printf("PCF85063 : Failed to obtain a warning flag.\r\n");
	else
		Value &= RTC_CTRL_2_AF | RTC_CTRL_2_AIE;
	//printf("Value = 0x%x",Value);
	return Value;
}

/******************************************************************************
function:	Set Alarm
parameter:			
Info:		
******************************************************************************/
void PCF85063_Set_Alarm(datetime_t time)
{

	uint8_t buf[5] ={
		decToBcd(time.second)&(~RTC_ALARM),
		decToBcd(time.minute)&(~RTC_ALARM),
		decToBcd(time.hour)&(~RTC_ALARM),
		//decToBcd(time.day)&(~RTC_ALARM),
		//decToBcd(time.dotw)&(~RTC_ALARM)
		RTC_ALARM, 	//disalbe day
		RTC_ALARM	//disalbe weekday
	};
	esp_err_t ret = I2C_Write(PCF85063_ADDRESS, RTC_SECOND_ALARM, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Failed to set alarm flag\r\n");
}

/******************************************************************************
function:	Read Alarm
parameter:			
Info:		
******************************************************************************/
void PCF85063_Read_Alarm(datetime_t *time)
{
	uint8_t buf[5] = {0};
	esp_err_t ret = I2C_Read(PCF85063_ADDRESS, RTC_SECOND_ALARM, buf, sizeof(buf));
	if(ret != ESP_OK)
		printf("PCF85063 : Failed to read the alarm sign\r\n");
	else{
		time->second = bcdToDec(buf[0] & 0x7F);
		time->minute = bcdToDec(buf[1] & 0x7F);
		time->hour = bcdToDec(buf[2] & 0x3F);
		time->day = bcdToDec(buf[3] & 0x3F);
		time->dotw = bcdToDec(buf[4] & 0x07);
	}
}


/******************************************************************************
function:	Convert normal decimal numbers to binary coded decimal
parameter:			
Info:		
******************************************************************************/
static uint8_t decToBcd(int val)
{
	return (uint8_t)((val / 10 * 16) + (val % 10));
}

/******************************************************************************
function:	Convert binary coded decimal to normal decimal numbers
parameter:			
Info:		
******************************************************************************/
static int bcdToDec(uint8_t val)
{
	return (int)((val / 16 * 10) + (val % 16));
}

/******************************************************************************
function:	
parameter:	
Info:		
******************************************************************************/
void datetime_to_str(char *datetime_str,datetime_t time)
{
	sprintf(datetime_str, " %d.%d.%d  %d %d:%d:%d ", time.year, time.month, 
			time.day, time.dotw, time.hour, time.minute, time.second);
} 