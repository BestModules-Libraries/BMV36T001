/*********************************************************************************************
File:       	  BMV36T001.cpp
Author:           BESTMODULE
Description:      SPI communicates with BMV36T001 and controls recording and audio playback
Version:          V1.0.1   -- 2023-09-13

**********************************************************************************************/

#include <BMV36T001.h>

#if (USE_HT32_BOARD)
	#include "mTM.h"
	#include "SD_BC/utility/Sd2Card_bc.h"
	#include "SD_BC/utility/SdFat_bc.h"
	#include "SD_BC/SD_bc.h"
	using namespace SD_bc;
	Sd2Card  	myCard;
	SdVolume   	myVolume;
	SdFile 		myRoot;
	SdFile 		myFile;
	WAV_TAG 	WAVE_HEAD;//wav head struct
	songtype 	list[250];//Store 250 songs
#endif




#define     ACK_RESPOND			(0x3E)	//The correct ack from BMV36T001
#define     NACK_RESPOND		(0xE3)
#define     PLAY_END			(0x55)	//Play finished

#define		GET_FW_VERSION		(0X01)
#define		RESET_BMV36T001		(0x02)
#define		SET_WORKMODE		(0X03)
#define     SET_VOLUME			(0X04)
#define     SET_CTL_MODE		(0X05)
#define		STOP_AUDIO			(0X06)
#define		PLAY_AUDIO			(0X07)
#define		PLAY_NEXT_AUDIO		(0X08)
#define		PLAY_PRE_AUDIO		(0X09)
#define		PLAY_REAPEAT		(0X0A)
#define		GET_RECORD_AMOUNT	(0X0B)
#define		GET_PLAY_STATUS		(0X0C)
#define		END_REC				(0X0D)
#define		START_REC			(0X0E)
#define		ERASE_REC			(0X0F)
#define		SET_PROMPT_TONE		(0X10)

#define		KEY_RESPOND			(0X99)
#define		FW_VERSION			(0X01)
#define		FRAME_HEADER		(0XFA)
#define		DUMMY_BYTE			(0X77)

#define		ENABLE_CMD_TRANSMIT		digitalWrite(D_C_PIN, LOW)
#define		DISABLE_CMD_TRANSMIT	digitalWrite(D_C_PIN, HIGH)

#define 	SD_CS_PIN 			(4)	//SD card select pin linked to pin10 of MCU
#define 	HT_CS_PIN 			(10)		//Shield CS pin
#define 	REQ_PIN			(9)		//When the SD card is playing, the BMV36T001 requests the SD card audio data
#define 	D_C_PIN			(8)		//SPI2 bus data type switch, pull up the transmission WAV audio data, pull down the transmission communication protocol command

static uint8_t fileBuf1[512];
static uint8_t fileBuf2[512];
static uint8_t fileBuf3[512];
static uint8_t rxdata = 0;
static uint16_t rxDataBytes1 = 0;
static uint16_t rxDataBytes2 = 0;
static uint16_t rxDataBytes3 = 0;
static uint16_t bufCount1 = 0;
static uint16_t bufCount2 = 0;
static uint16_t bufCount3 = 0;
static uint8_t readSel = 0;
static uint8_t buf1Full = 0;
static uint8_t buf2Full = 0;
static uint8_t buf3Full = 0;
static uint8_t headDispose = 0;
static uint32_t mainCount = 0;
static uint32_t remainCount = 0;
static uint8_t startRead = 0;
static uint8_t resetParamFlag = 0;
static uint8_t lastState = 0;

static uint8_t playFinished = 0;
static uint8_t readFinished = 0;

uint32_t BMV36T001::_taillessFileSize = 0;
short BMV36T001::_songIndex = 0;
short BMV36T001::_maxSong = 0;
uint8_t BMV36T001::_loopCurrentSongFlag = 0;
uint8_t BMV36T001::_playOnce = 0;
uint8_t BMV36T001::_modeFlag = 0;
uint8_t BMV36T001::_lastCmd = 0;
uint8_t BMV36T001::_sendFinished = 1;

uint8_t BMV36T001::_firstStep = 0;
uint8_t BMV36T001::_keyValue = 0;
uint8_t BMV36T001::_isKey = 0;
uint8_t BMV36T001::_playStatus = 0;

/************************************************************************* 
Description:    Constructor
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
BMV36T001::BMV36T001()
{
	_ctrlFlag = 0;
	_modeFlag = 0;
}
/************************************************************************* 
Description:    Initialize the IO pin
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::initIO(void)
{

	pinMode(SD_CS_PIN, OUTPUT);
	digitalWrite(SD_CS_PIN, HIGH);
	
	pinMode(HT_CS_PIN, OUTPUT);
	digitalWrite(HT_CS_PIN, HIGH);

	pinMode(D_C_PIN, OUTPUT);
	digitalWrite(D_C_PIN, HIGH);

	pinMode(REQ_PIN, INPUT);

}
#else
void BMV36T001::initIO(void)
{
	pinMode(HT_CS_PIN, OUTPUT);
	digitalWrite(HT_CS_PIN, HIGH);

	pinMode(D_C_PIN, OUTPUT);
	digitalWrite(D_C_PIN, HIGH);

	pinMode(REQ_PIN, INPUT);

}
#endif
/************************************************************************* 
Description:    Initialize the SD Card
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::initSD(void)
{
	uint8_t i = 0;
	dir_t p;
	char name[13];
	/* init sd card */
	if (!myCard.init(SPI_FULL_SPEED, SD_CS_PIN))   //SPI_FULL_SPEED
	{ 
		return;
	}

	if (!myVolume.init(myCard))
	{
		return;
	}
	myRoot.openRoot(&myVolume);
	while (myRoot.readDir(&p) != 0)
	{
		SdFile::dirName(p, name);
		char *s = name;
		s = strupr(s);
		if (strstr(s, ".WAV"))
		{
			uint32_t curPos = myRoot.curPosition();
			uint16_t index = (curPos - 32) >> 5;
			if (myFile.open(&myRoot, index, O_READ))
			{
				myFile.close();
				strncpy(list[i].name, name, 13);
				list[i].index = index;
				i++;
			}	
		}
	}
	_maxSong = (i - 1);
}
#endif
/************************************************************************* 
Description:    Initialize the BMV36T001 shield
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::initShield(void)
{

	digitalWrite(HT_CS_PIN, HIGH);
	sendPlayEnd();
	reset();
}
#else
void BMV36T001::initShield(void)
{
	reset();
}
#endif
/************************************************************************* 
Description:    Initialize communication between development board and BMV36T001
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::begin(void)
{
	BMV36T001_SPI.begin();
	initIO();
	initSD();
	delay(1500);//Get ready to send the command later
	initShield();
	setControlMode();
	mTM_Configuration(32500, BMV36T001::transmitWAVData);
}
#else
void BMV36T001::begin(void)
{
	BMV36T001_SPI.begin();
	initIO();
	delay(1500);//Get ready to send the command later
	initShield();
	setControlMode();
}
#endif
/************************************************************************* 
Description:    Get F/W version with BMV36T001
parameter：
	Input:          
	Output:         
Return:         0：Failed to get the version value. non-zero：version value
Others:         
*************************************************************************/
uint8_t BMV36T001::getFWVer(void)
{
	uint8_t readBuf[2];
	uint8_t writeBuf[3] = {FRAME_HEADER, GET_FW_VERSION, GET_FW_VERSION};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
    if(readRespone(readBuf, 2, GET_FW_VERSION))
    {
        return readBuf[1];
    }
    else
    {
        return 0;
    }	
}
/************************************************************************* 
Description:    Reset BMV36T001
parameter：
	Input:          
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::reset(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, RESET_BMV36T001, RESET_BMV36T001};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, RESET_BMV36T001);
}
/************************************************************************* 
Description:    Set the prompt tone switch status
parameter：
	Input:      state: true: open prompt tone; false: close prompt tone，default true   
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::setVoiceGuided(bool state)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_PROMPT_TONE, state, SET_PROMPT_TONE + state};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_PROMPT_TONE, state);
}
/************************************************************************* 
Description:    Set the working mode of the BMV36T001
parameter：
	Input:      mode: 1: record mode; 2: SD card mode
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::setPlayMode(uint8_t mode)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_WORKMODE, mode,  (uint8_t)(SET_WORKMODE + mode)};
	if(1 == mode)
	{
		stopTimer();//In recording mode, disable the timer interrupt for sending audio data
	}
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_WORKMODE, mode);
}
#else
bool BMV36T001::setPlayMode(uint8_t mode)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_WORKMODE, mode, SET_WORKMODE + mode};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_WORKMODE, mode);
}
#endif
/************************************************************************* 
Description:    Set BMV36T001 to network mode
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::setControlMode(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_CTL_MODE, 1, SET_CTL_MODE + 1};
	stopTimer();//Before sending this command, disable the timer interrupt for sending audio data
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_CTL_MODE);
}
#else
bool BMV36T001::setControlMode(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_CTL_MODE, 1, SET_CTL_MODE + 1};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_CTL_MODE);
}
#endif
/************************************************************************* 
Description:    Cyclic detection of key status value
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::scanKey(void)
{
	uint8_t readBuf[1];
	if((0 == _playOnce) && (0 == _loopCurrentSongFlag))//Can be executed only when wav is not played.
	{												// Otherwise, wav cannot be played properly
		ENABLE_CMD_TRANSMIT;						//This part is transferred to the timer interrupt
		readBytes(readBuf, 1);
		if (0 == _firstStep)
		{
			if (KEY_RESPOND == readBuf[0])
			{	
				_firstStep = 1;
			}
			else
			{
				_keyValue = 0;
			}
		}
		else
		{
			_firstStep = 0;
			_keyValue = readBuf[0];
			_isKey = 1;
		}
	}	
}
#else
void BMV36T001::scanKey(void)
{
	uint8_t readBuf[1];
	ENABLE_CMD_TRANSMIT;
	readBytes(readBuf, 1);
	if (0 == _firstStep)
	{
		if (KEY_RESPOND == readBuf[0])
		{	
			_firstStep = 1;
		}
		else
		{
			_keyValue = 0;
		}
	}
	else
	{
		_firstStep = 0;
		_keyValue = readBuf[0];
		_isKey = 1;
	}
	
}
#endif
/************************************************************************* 
Description:    Determine whether the keys have changed
parameter：
	Input:          
	Output:         
Return:         0: The keys don't change; 1: The keys have changed.
Others:         
*************************************************************************/
bool BMV36T001::isKeyAction(void)
{
    if(_isKey)
    {
        _isKey = 0;
        return 1;     
    }
	else 
    {
        return 0;
    }
}
/************************************************************************* 
Description:    Read the key level value
parameter：
	Input:          
	Output:         
Return:         0: pressing; 1: no pressing
Others:         
*************************************************************************/
uint8_t BMV36T001::readKeyValue(void)
{
	return _keyValue;
}
/************************************************************************* 
Description:    Stop playing recordings / SD card audio
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::playStop(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, STOP_AUDIO, STOP_AUDIO};
	if(2 == _modeFlag)
	{
		stopTimer();//Before sending this command, disable the timer interrupt for sending audio data
		delayMicroseconds(15);//To prevent a byte of audio data from being discarded, 
							  //the BMV36T001 must wait until it receives the last byte of audio data before sending the command
	}
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, STOP_AUDIO, _modeFlag);	
}
#else
bool BMV36T001::playStop(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, STOP_AUDIO, STOP_AUDIO};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, STOP_AUDIO, _modeFlag);	
}
#endif
/************************************************************************* 
Description:    Play recordings / SD card audio
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::playVoice(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_AUDIO, PLAY_AUDIO};
	if(2 == _modeFlag)
	{
		stopTimer();
		delayMicroseconds(15);
	}	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_AUDIO, _modeFlag);	
}
#else
bool BMV36T001::playVoice(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_AUDIO, PLAY_AUDIO};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_AUDIO, _modeFlag);	
}
#endif
/************************************************************************* 
Description:    Play the next recordings / SD card audio
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::playNext(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_NEXT_AUDIO, PLAY_NEXT_AUDIO};
	if(2 == _modeFlag)
	{
		stopTimer();
		delayMicroseconds(15);
	}	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_NEXT_AUDIO, _modeFlag);	
}
#else
bool BMV36T001::playNext(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_NEXT_AUDIO, PLAY_NEXT_AUDIO};
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_NEXT_AUDIO, _modeFlag);	
}
#endif
/************************************************************************* 
Description:    Play the previous recordings / SD card audio
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::playPrevious(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_PRE_AUDIO, PLAY_PRE_AUDIO};
	if(2 == _modeFlag)
	{
		stopTimer();
		delayMicroseconds(15);
	}	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_PRE_AUDIO, _modeFlag);	
}
#else
bool BMV36T001::playPrevious(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_PRE_AUDIO, PLAY_PRE_AUDIO};	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_PRE_AUDIO, _modeFlag);	
}
#endif
/************************************************************************* 
Description:    Repeat the recordings / SD card audio
parameter：
	Input:          
	Output:         
Return:      	Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::playVoiceRepeat(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_REAPEAT, PLAY_REAPEAT};
	if(2 == _modeFlag)
	{
		stopTimer();
		delayMicroseconds(15);
	}	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_REAPEAT, _modeFlag);	
}
#else
bool BMV36T001::playVoiceRepeat(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_REAPEAT, PLAY_REAPEAT};	
    ENABLE_CMD_TRANSMIT;
    writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_REAPEAT, _modeFlag);	
}
#endif
/************************************************************************* 
Description:    Gets the total amount of audio in the corresponding mode type
parameter：
	Input:      type:1: record mode; 2: SD card mode
	Output:         
Return:         the total amount of audio
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
uint8_t BMV36T001::getVoiceSum(uint8_t type)
{
	uint8_t readBuf[2];
	uint8_t writeBuf[3] = {FRAME_HEADER, GET_RECORD_AMOUNT, GET_RECORD_AMOUNT};

	if(1 == type)//record mode
	{
		ENABLE_CMD_TRANSMIT;
    	writeBytes(writeBuf, 3);
		if(readRespone(readBuf, 2, GET_RECORD_AMOUNT))
		{
			return readBuf[1];
		}
		else
		{
			return 0;
		}	
	}
	else if(2 == type)//SD card mode
	{
		return (_maxSong + 1);
	}	
	return 0;	
}
#else
uint8_t BMV36T001::getVoiceSum(uint8_t type)
{
	uint8_t readBuf[2];
	uint8_t writeBuf[3] = {FRAME_HEADER, GET_RECORD_AMOUNT, GET_RECORD_AMOUNT};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	if(readRespone(readBuf, 2, GET_RECORD_AMOUNT))
	{
		return readBuf[1];
	}
	else
	{
		return 0;
	}		
}
#endif
/************************************************************************* 
Description:    Get the play status
parameter：
	Input:          
	Output:         
Return:         0: idle; 1: in the play
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::isPlaying(void)
{
	uint8_t readBuf[2];
	uint8_t writeBuf[3] = {FRAME_HEADER, GET_PLAY_STATUS, GET_PLAY_STATUS};


	if(1 == _modeFlag)//record mode
	{
		ENABLE_CMD_TRANSMIT;
    	writeBytes(writeBuf, 3);
		if(readRespone(readBuf, 2, GET_PLAY_STATUS))
		{
			return readBuf[1];
		}
		else
		{
			return 0;
		}	
	}
	else if(2 == _modeFlag)
	{
		return _playStatus;	
	}	
	return 0;
}
#else
bool BMV36T001::isPlaying(void)
{
	uint8_t readBuf[2];
	uint8_t writeBuf[3] = {FRAME_HEADER, GET_PLAY_STATUS, GET_PLAY_STATUS};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	if(readRespone(readBuf, 2, GET_PLAY_STATUS))
	{
		return readBuf[1];
	}
	else
	{
		return 0;
	}	
}
#endif
/************************************************************************* 
Description:    Set volume
parameter：
	Input:      volume: 1~14(There are 14 levels of volume adjustment)
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::setVolume(uint8_t volume)
{

	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_VOLUME, volume, (uint8_t)(SET_VOLUME + volume)};
	stopTimer();
	delayMicroseconds(15);//Delay in case the last audio data was not received
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_VOLUME, _modeFlag);
}
#else
bool BMV36T001::setVolume(uint8_t volume)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[4] = {FRAME_HEADER, SET_VOLUME, volume, SET_VOLUME + volume};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 4);
	return readRespone(readBuf, 1, SET_VOLUME);
}
#endif
/************************************************************************* 
Description:    End of the recording
parameter：
	Input:          
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::recordStop(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, END_REC, END_REC};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, END_REC);
}
/************************************************************************* 
Description:    Start the recording
parameter：
	Input:          
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::recordStart(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, START_REC, START_REC};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, START_REC);
}
/************************************************************************* 
Description:    Erase all recordings
parameter：
	Input:          
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::recordErase(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, ERASE_REC, ERASE_REC};
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, ERASE_REC);
}
/************************************************************************* 
Description:    Get response
parameter：
	Input:      *rxbuf : Used to store the received data
					bufLength：The length of data to receive
	Output:     Read RxBuffer 
Return:         Read status  1:Success 0:Fail     
Others:         
**************************************************************************/
#if	(USE_HT32_BOARD)
bool BMV36T001::readRespone(uint8_t *rxBuf, uint8_t bufLength, uint8_t cmd, uint8_t value)
{
	uint8_t cnt = 0;
	uint8_t readBuf[1] = {0};
	while(1)
	{
		readBytes(readBuf, 1);
		if(cnt < bufLength)
		{
			rxBuf[cnt] = readBuf[0];	
			if(0xe3 == rxBuf[0])
			{
				DISABLE_CMD_TRANSMIT;
				return 0;
			}
			else if(0x3e == rxBuf[0])
			{
				cnt++;
			}
		}
		if(cnt == bufLength)
		{
			switch(cmd)
			{
				case SET_WORKMODE:
					if (1 == value)
					{
						_modeFlag = 1;
						_loopCurrentSongFlag = 0;
						_playOnce = 0;
					}
					else if (2 == value)
					{
						_modeFlag = 2;
						_loopCurrentSongFlag = 0;
						_playOnce = 0;		
						resetParam();
						_songIndex = 0;
					}
					break;
				case STOP_AUDIO:
					if (2 == value)
					{
						if (STOP_AUDIO != _lastCmd)//Prevent stop from coming in twice and changing the previous playback state
						{
							if (_playOnce)
							{
								lastState = 1;
							}
							else if (_loopCurrentSongFlag)
							{
								lastState = 2;
							}
							else
							{
								lastState = 0;
							}
							_playOnce = 0;
							_loopCurrentSongFlag = 0;
						}
						_lastCmd = STOP_AUDIO;	
					}	
					_playStatus = 0;
					break;
				case PLAY_AUDIO:
					if (2 == value)
					{
						if(STOP_AUDIO == _lastCmd)
						{
							if (1 == lastState)
							{
								_playOnce = 1;
							}
							else if(2 == lastState)
							{
								_loopCurrentSongFlag = 1;
							}	
							startTimer();
						}
						else
						{
							_playOnce = 1;
						}
						_lastCmd = PLAY_AUDIO;
					}
					break;
				case PLAY_NEXT_AUDIO:
					if (2 == value)
					{
						if (++_songIndex > _maxSong)
						{
							_songIndex = _maxSong;
						}	
						resetParam();
						_playOnce = 1;
						_loopCurrentSongFlag = 0;
						_lastCmd = PLAY_NEXT_AUDIO;
					}
					break;
				case PLAY_PRE_AUDIO:
					if (2 == value)
					{
						if (--_songIndex < 0)
						{
							_songIndex = 0;
						}
						resetParam();
						_playOnce = 1;
						_loopCurrentSongFlag = 0;
						_lastCmd = PLAY_PRE_AUDIO;
					}
					break;
				case PLAY_REAPEAT:
					if (2 == value)
					{
						_playOnce = 0;
						_loopCurrentSongFlag = 1;	
						startTimer();	
						_lastCmd = PLAY_REAPEAT;
					}
					break;
				case SET_VOLUME:
					if(2 == value)
					{
						if(STOP_AUDIO == _lastCmd)
						{
							;
						}
						else
						{
							startTimer();
						}
					}
					break;
			}
			DISABLE_CMD_TRANSMIT;
			
			return 1;
		} 
	}
}
#else
bool BMV36T001::readRespone(uint8_t *rxBuf, uint8_t bufLength, uint8_t cmd, uint8_t value)
{
	uint8_t cnt = 0;
	uint8_t readBuf[1] = {0};
	while(1)
	{
		readBytes(readBuf, 1);
		if(cnt < bufLength)
		{
			rxBuf[cnt] = readBuf[0];
			if(0xe3 == rxBuf[0])
			{
				DISABLE_CMD_TRANSMIT;
				return 0;
			}
			else if(0x3e == rxBuf[0])
			{
				cnt++;
			}
		}
		if(cnt == bufLength)
		{
			DISABLE_CMD_TRANSMIT;
			return 1;
		} 
	}
}
#endif
/************************************************************************* 
Description:    Write bytes to BMV36T001
parameter：
	Input:      writeBuf[]: Stores the data to be sent
				writeLen: The length of data to send
	Output:         
Return:         
Others:         
*************************************************************************/
void BMV36T001::writeBytes(uint8_t writeBuf[], uint8_t writeLen)
{
	uint8_t i = 0;
	digitalWrite(HT_CS_PIN, LOW);
	for(i = 0; i < writeLen; i++)
	{
		BMV36T001_SPI.transfer(writeBuf[i]); 
		delayMicroseconds(30);//Leave time for the BMV36T001 to process the data it receives
	}
	digitalWrite(HT_CS_PIN, HIGH);
}
/************************************************************************* 
Description:    Read bytes from BMV36T001
parameter：
	Input:      readBuf[]: Used to store the received data
				readLen: The length of data to receive
	Output:         
Return:         
Others:         
*************************************************************************/
void BMV36T001::readBytes(uint8_t readBuf[], uint8_t readLen)
{
	uint8_t i = 0;
	digitalWrite(HT_CS_PIN, LOW);
	for(i = 0; i < readLen; i++)
	{
		readBuf[i] = BMV36T001_SPI.transfer(DUMMY_BYTE); 
		delayMicroseconds(30);//Leave time for the BMV36T001 to process the data it receives
	}
	digitalWrite(HT_CS_PIN, HIGH);	
}
/************************************************************************* 
Description:    Transmit WAV audio data
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
#if	(USE_HT32_BOARD)
void BMV36T001::transmitWAVData(void)
{
	if (buf1Full||buf2Full||buf3Full)
	{
		_playStatus = 1;
		DISABLE_CMD_TRANSMIT;
		if (0 == digitalRead(REQ_PIN))
		{	
			if (0 == readSel)
			{
				digitalWrite(HT_CS_PIN, LOW);
				rxdata = BMV36T001_SPI.transfer(fileBuf1[bufCount1++]);	
				if (rxDataBytes1 == bufCount1)
				{			
					bufCount1 = 0;
					readSel = 1;
					buf1Full = 0;
				}
			}
			else if (1 == readSel)
			{
				digitalWrite(HT_CS_PIN, LOW);
				rxdata = BMV36T001_SPI.transfer(fileBuf2[bufCount2++]);			
				if (rxDataBytes2 == bufCount2)
				{
					bufCount2 = 0;
					readSel = 2;
					buf2Full = 0;
				}
			}
			else if (2 == readSel)
			{
				digitalWrite(HT_CS_PIN, LOW);
				rxdata = BMV36T001_SPI.transfer(fileBuf3[bufCount3++]);		
				if (rxDataBytes3 == bufCount3)
				{
					bufCount3 = 0;
					readSel = 0;
					buf3Full = 0;
				}				
			}
			if (0 == _firstStep)
			{
				if (KEY_RESPOND == rxdata)
				{	
					_firstStep = 1;
				}
			}
			else
			{
				_firstStep = 0;
				_keyValue = rxdata;
				_isKey = 1;
			}	
		}
	}	
	if (readFinished && (0 == buf1Full) && (0 == buf2Full) && (0 == buf3Full))
	{
		if(1 == _playOnce)
		{
			digitalWrite(HT_CS_PIN, HIGH);
			sendPlayEnd();
			ENABLE_CMD_TRANSMIT;
			_playStatus = 0;
		}
		stopTimer();
		_playOnce = 0;	
		playFinished = 1;
		readFinished = 0;
		readSel = 0;
	}
}
/************************************************************************* 
Description:    Play the song with the specified name in the SD card
parameter：
	Input:      name:The song name
	Output:         
Return:         
Others:         Format must conform to WAV 16bit PCM, mono
*************************************************************************/
void BMV36T001::playWAV(char name[])
{
	uint8_t songNum = 0;
	static char lastName[12];
	if(strncmp(name, lastName, 12))
	{
		playNext();//Mainly to switch to a new playback state
		_songIndex--;	
	}
	if (0 == headDispose)
	{
		_playOnce = 1;
		playFinished = 2;
		headDispose = 1;
		if (myFile.open(&myRoot, name, O_READ))
		{
			_index = (myRoot.curPosition() - 32) >> 5;
			while(_index != list[songNum].index)
			{
				songNum++;
			}
			_songIndex = songNum;
			WAVE_HEAD.headSize = getHeadSize();
			mainCount = _taillessFileSize / 512;
			remainCount = _taillessFileSize % 512;
			memcpy(lastName, name, 12);
		}
		else
		{
			return;//No exist,please check the myFile name!
		}
	}	
}
/************************************************************************* 
Description:    Play audio from the SD Card,This must be done in a loop
parameter：
	Input:          
	Output:         
Return:         
Others:         Format must conform to WAV 16bit PCM, mono
*************************************************************************/
void BMV36T001::loopWAV(void)
{	
	if (_loopCurrentSongFlag || _playOnce)
	{
		if (0 == headDispose)
		{
			playFinished = 2;
			headDispose = 1;
			if (myFile.open(&myRoot, list[_songIndex].index, O_READ))
			{
				WAVE_HEAD.headSize = getHeadSize();
				mainCount = _taillessFileSize / 512;
				remainCount = _taillessFileSize % 512;
			}
			else
			{
				return;//Could not open the file
			}
		}
	}
	if (_loopCurrentSongFlag)
	{
		readMusicData();
	}
	else
	{		
		if (_playOnce)
		{		
			readMusicData();
		}
	}
	if (1 == playFinished)
	{
		headDispose = 0;
		startRead = 0;
	}			
}
/************************************************************************* 
Description:    Get the size of the WAV head
parameter：
	Input:          
	Output:         
Return:         return the size of the WAV head
Others:         
*************************************************************************/
uint32_t BMV36T001::getHeadSize(void)
{
	uint32_t offset = 0;
	uint32_t skipLength = 0;
	char* otherChunkID;
	/*RIFF*/
	myFile.read(WAVE_HEAD.ChunkID, sizeof(WAVE_HEAD.ChunkID));
	offset += 4;
	/*length*/
	WAVE_HEAD.ChunkSize = getChunkSize();
	offset += 4;	
	/*wave*/
	myFile.read(WAVE_HEAD.Format, sizeof(WAVE_HEAD.Format));
	offset += 4;	
	/*fmt*/
	myFile.read(WAVE_HEAD.SubChunk1ID, sizeof(WAVE_HEAD.SubChunk1ID));	
	offset += 4;
	/*fmt length*/
	WAVE_HEAD.SubChunk1Size = getChunkSize();
	offset += 4;
	skipLength = WAVE_HEAD.SubChunk1Size;
	offset += (unsigned char)skipLength;
	/*skipread*/
	skipRead((unsigned char)skipLength);
	/*other ChunkID*/
	otherChunkID = getOtherChunkId();
	offset += 4;
	while (strncmp(otherChunkID, "data", 4))
	{	
		skipLength = getChunkSize();
		offset += 4;
		offset += (unsigned char)skipLength;

		skipRead((unsigned char)skipLength);
		
		otherChunkID = getOtherChunkId();
		offset += 4;		
	}
	WAVE_HEAD.dataSize = getChunkSize();//Gets the length of the audio data
	offset += 4;
	_taillessFileSize = WAVE_HEAD.dataSize + offset;
	return offset;//return the size of the WAV head
}
/************************************************************************* 
Description:    Get the size of the Chunk
parameter：
	Input:          
	Output:         
Return:         return the size of the Chunk
Others:         
*************************************************************************/
uint32_t BMV36T001::getChunkSize(void)
{
	
	uint32_t sizeOfChunk;
	uint8_t sizeBuf[4];
	myFile.read(sizeBuf, sizeof(sizeBuf));
	uint32_t first8 = sizeBuf[0];
	uint32_t first16 = sizeBuf[1];
	uint32_t first24 = sizeBuf[2];
	uint32_t first32 = sizeBuf[3];

	return sizeOfChunk = first8 | (first16 << 8) | (first24 << 16) | (first32 << 24);
}
/************************************************************************* 
Description:    Get the other Chunk of ID
parameter：
	Input:          
	Output:         
Return:         return other Chunk of ID
Others:         
*************************************************************************/
char* BMV36T001::getOtherChunkId(void)
{
	myFile.read(WAVE_HEAD.otherChunkID, sizeof(WAVE_HEAD.otherChunkID));
	return WAVE_HEAD.otherChunkID;
}
/************************************************************************* 
Description:    Skip read some datas
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
void BMV36T001::skipRead(unsigned char skipNum)
{
	uint8_t skipBuf[skipNum];
	myFile.read(skipBuf, sizeof(skipBuf));
}
/************************************************************************* 
Description:    Read WAV audio data(excluding the head)
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
void BMV36T001::readMusicData(void)
{
	if (_taillessFileSize != myFile.curPosition())
	{
		if (0 == buf1Full)
		{
			if (WAVE_HEAD.headSize == myFile.curPosition())
			{
				rxDataBytes1 = myFile.read(fileBuf1, 512-WAVE_HEAD.headSize);// Read out the 128 bytes except for the header
				mainCount--;
			}
			else if (mainCount > 0)
			{		
				rxDataBytes1 = myFile.read(fileBuf1, 512);
				mainCount--;
			}
			else
			{		
				rxDataBytes1 = myFile.read(fileBuf1, remainCount);
			}	
			buf1Full = 1;	
		}
		if (0 == buf2Full)
		{
			if (mainCount > 0)
			{	
				rxDataBytes2 = myFile.read(fileBuf2, 512);
				mainCount--;
			}
			else
			{		
				rxDataBytes2 = myFile.read(fileBuf2, remainCount);
			}	
			buf2Full = 1;				
		}
		if (0 == buf3Full)
		{
			if (mainCount > 0)
			{	
				rxDataBytes3 = myFile.read(fileBuf3, 512);
				mainCount--;
			}
			else
			{		
				rxDataBytes3 = myFile.read(fileBuf3, remainCount);
			}	
			buf3Full = 1;				
		}
		if (resetParamFlag)
		{	
			resetParamFlag = 0;
			resetParam();
		}
		if (buf1Full && buf2Full && buf3Full)
		{
			if (0 == startRead)
			{
				startRead = 1;
				startTimer();
			}			
		}
	}
	else
	{
		readFinished = 1;	
		myFile.close();				
	}
}
/************************************************************************* 
Description:    Informs the BMV36T001 that a song(SD Card) has been played
parameter：
	Input:          
	Output:         
Return:         Read status  1:Success 0:Fail 
Others:         
*************************************************************************/
bool BMV36T001::sendPlayEnd(void)
{
	uint8_t readBuf[1];
	uint8_t writeBuf[3] = {FRAME_HEADER, PLAY_END, PLAY_END};
	delayMicroseconds(15);//Delay in case the last audio data was not received
	ENABLE_CMD_TRANSMIT;
	writeBytes(writeBuf, 3);
	return readRespone(readBuf, 1, PLAY_END);
}
/************************************************************************* 
Description:    Get the current working mode of the BMV36T001
parameter：
	Input:          
	Output:         
Return:      	1: record mode; 2: SD card mode
Others:         
*************************************************************************/
uint8_t BMV36T001::getPlayMode(void)
{
	return _modeFlag;
}
/************************************************************************* 
Description:    Reset WAV-related parameters
parameter：
	Input:          
	Output:         
Return:         
Others:         
*************************************************************************/
void BMV36T001::resetParam(void)
{
	buf1Full = 0;
	buf2Full = 0;
	buf3Full = 0;
	bufCount1 = 0;
	bufCount2 = 0;
	bufCount3 = 0;
	readSel = 0;
	readFinished = 0;	
	headDispose = 0;
	startRead = 0;
	lastState = 0;
	myFile.close();
}
#endif