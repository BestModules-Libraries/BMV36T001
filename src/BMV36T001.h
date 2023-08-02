/*************************************************************************
File:       	  BMV36T001.h
Author:           YANG, BESTMODULE
Description:      Define classes and required variables
History：		  -
	V1.0	 -- initial version； 2022-07-29； Arduino IDE : v1.8.13

**************************************************************************/


#ifndef _BMV36T001_H
#define _BMV36T001_H




#include <SPI.h>

#include <avr/io.h>
#include <arduino.h>

#define BMV36T001_RECORD_DISABLE 0
#define BMV36T001_RECORD_ENABLE 1
#define BMV36T001_MODE_RECORD 1
#define BMV36T001_MODE_SD 2
#define BMV36T001_PLAY_ENABLE 1
#define BMV36T001_PLAY_DISABLE 0


#define BMV36T001_NO_KEY 0X00
#define BMV36T001_MIDDLE_KEY 0X01
#define BMV36T001_UP_KEY 0X02
#define BMV36T001_DOWN_KEY 0X04
#define BMV36T001_LEFT_KEY 0X08
#define BMV36T001_RIGHT_KEY 0X10

#define BMV36T001_MAX_VOLUME  13
#define BMV36T001_MIN_VOLUME  0
#define DEFAULT_VOLUME		  7



#if defined(__AVR_ATmega328P__)
	#define     USE_HT32_BOARD		0
	#define		BMV36T001_SPI		SPI
#else
	#define     USE_HT32_BOARD		1
	#define		BMV36T001_SPI		SPI
#endif



/*wav head format parameters*/
typedef struct wav_tag{
	char ChunkID[4]; // "RIFF",4 bytes
	 
	uint32_t ChunkSize; // wav file length (Does not contain the first 8 bytes),4 bytes
	 
	char Format[4]; // "WAVE",4 bytes
	 
	char SubChunk1ID[4]; // "fmt",4 bytes
	 
	uint32_t SubChunk1Size; // chunk size,4 bytes
	 
	unsigned short int AudioFormat; // Audio format(10H is PCM format),2 bytes
	 
	unsigned short int NumChannels; // channel number(mono：1, Stereo：2),2 bytes
	 
	uint32_t SampleRate; // Sampling rate，4 bytes
	 
	uint32_t ByteRate; // data transfer rate, value：NumChannels*SampleRate*BitsPerSample/8,4 bytes
	 
	unsigned short int BlockAlign; // aata block alignment unit, value：NumChannels*BitsPerSample/8,2 bytes
	 
	unsigned short int BitsPerSample; // Sample data bits, 2 bytes
	 
	char SubChunk2ID[4]; // "data",4 bytes
	
	char otherChunkID[4];
	 
	uint32_t dataSize; // music data length,4 bytes
	
	uint32_t headSize;//wav hwad size

}WAV_TAG;


/*wav song parameters*/
typedef struct songDesc
{
  char name[13];//wav song name(The file name cannot be longer than 8 bytes, 
  				//the extension cannot be longer than 3 bytes, 
				//and the name cannot contain Chinese characters)
				//example：abc12345.wav
  uint16_t index;//the number of wav song 
}songtype;


/*****************class for the BMV36T001*******************/
class BMV36T001
{
public:
	
	//----------------system----------------------------------
	BMV36T001();
	void begin(void);
	static bool reset(void);
	uint8_t getFWVer(void);
//--------------key----------------------------
	void scanKey(void);
	bool isKeyAction(void);
	uint8_t readKeyValue(void);	
	//-------------play control-------------------------
	//play WAV or record
	bool isPlaying(void);
	bool playStop(void);
	bool playVoice(void);
	bool playNext(void);
	bool playPrevious(void);
	bool playVoiceRepeat(void);
	//play WAV
	void playWAV(char name[]);
	void loopWAV(void);//read WAV data from SD card on loop
	//-------------set and get status-------------------------
	static bool setPlayMode(uint8_t mode);
	uint8_t getPlayMode(void);
	uint8_t getVoiceSum(uint8_t type);
	bool setVolume(uint8_t volume);

	//-------------record control-------------------------------------
	bool recordStop(void);
	bool recordStart(void);
	bool recordErase(void);
	//--------------------------------------------------

private:
	//SPI write from BMV36T001
	static void writeBytes(uint8_t writeBuf[], uint8_t writeLen);
	//SPI read from BMV36T001
	static void readBytes(uint8_t readBuf[], uint8_t readLen);
	/*************************SD card operation function**************************/
	void initIO(void);
	void initSD(void);
	void initShield(void);
	uint32_t getHeadSize(void);
	uint32_t getChunkSize(void);
	char* getOtherChunkId(void);
	void skipRead(unsigned char skipNum);	
	void readMusicData(void);
	static void resetParam(void);
	static bool sendPlayEnd(void);
	bool setControlMode(void);
	static bool readRespone(uint8_t *rxBuf, uint8_t bufLength, uint8_t cmd, uint8_t value = 0);
	/*******************************************************/
	static uint32_t _taillessFileSize;//Tailless WAV file size
	static short _songIndex;
	static short _maxSong;
	uint16_t _index;
	static uint8_t _loopCurrentSongFlag;//flag that play SD current audio in a loop
	static uint8_t _playOnce;//Play the current audio in the SD card once
	static uint8_t _modeFlag;//the flag of mode，1：SD_MODE；2：RECORD_MODE
	static uint8_t _lastCmd;
	static uint8_t _sendFinished;
	uint8_t	_ctrlFlag;
	uint8_t _recordNum;
	uint8_t _songNum;
	static uint8_t _firstStep;
	static uint8_t _playStatus;
	static uint8_t _keyValue;
	static uint8_t _isKey;
public:	//Internal operation
	static void transmitWAVData(void);//play wav on timer(16KHz) interrupt 	
	
};



#endif
