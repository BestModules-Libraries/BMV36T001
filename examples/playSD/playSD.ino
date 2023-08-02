/******************************************************************
File:             playSD.ino
Description:      Play WAV audio file in SD card
Note:             -       
******************************************************************/


#include <BMV36T001.h>

BMV36T001 myBMV36T001;

uint8_t volume = DEFAULT_VOLUME;
uint8_t currentKeyValue = 0;//Current key state
uint8_t lastKeyValue = BMV36T001_NO_KEY; //Last key state
uint8_t keycode = BMV36T001_NO_KEY;//Recorded keys

void setup() {
  // put your setup code here, to run once:
    myBMV36T001.begin();
    myBMV36T001.setPlayMode(BMV36T001_MODE_SD);  //Enter SD mode
}

uint8_t keyPressDispose(uint8_t keyValue) {
    uint8_t result = BMV36T001_NO_KEY;
    if(lastKeyValue == BMV36T001_NO_KEY)//Didn't press the key last time
    {
        if((keyValue & BMV36T001_MIDDLE_KEY) != 0) {//Middle key pressed
            result  = BMV36T001_MIDDLE_KEY;//Press the middle key
        }
        else if((keyValue & BMV36T001_UP_KEY) != 0) {//Press the up key
            result  = BMV36T001_UP_KEY;
        }
        else if((keyValue & BMV36T001_DOWN_KEY) != 0) {//Down key press
            result  = BMV36T001_DOWN_KEY;
        }
        else if((keyValue & BMV36T001_LEFT_KEY) != 0) { //Left key press
            result  = BMV36T001_LEFT_KEY;
        }      
        else {
            result  = BMV36T001_RIGHT_KEY;
        }
    }   
    lastKeyValue = currentKeyValue; //Save this key state
    return result ;
}



void loop() {
    myBMV36T001.loopWAV();
    // put your main code here, to run repeatedly:
    myBMV36T001.scanKey();
    if(myBMV36T001.isKeyAction())
    {
        currentKeyValue = myBMV36T001.readKeyValue();//Read key status
        keycode = keyPressDispose(currentKeyValue) ;//Call the key press analysis function
        
        if(keycode != BMV36T001_NO_KEY)
        {
            switch(keycode)//Judge and dispose key
            {
                case BMV36T001_MIDDLE_KEY://The middle key is pressed
                    if(myBMV36T001.isPlaying())
                    {
                        myBMV36T001.playStop();
                    }
                    else
                    {
                        myBMV36T001.playVoice();
                    }
                    break;
                case BMV36T001_UP_KEY://The up key is pressed
                    if(volume < BMV36T001_MAX_VOLUME)
                    {
                        volume++;
                        myBMV36T001.setVolume(volume); 
                    }
                    break;
                case BMV36T001_DOWN_KEY://The down key is pressed
                    if(volume > BMV36T001_MIN_VOLUME)
                    {
                        volume--;   
                        myBMV36T001.setVolume(volume);
                    }
                    break;
                case BMV36T001_LEFT_KEY://The left key is pressed
                    myBMV36T001.playPrevious();
                    break;
                case BMV36T001_RIGHT_KEY://The right key is pressed
                    myBMV36T001.playNext(); 
                    break;
                default:
                    break;     
            } 
            keycode = BMV36T001_NO_KEY; 
        }
    }
}
