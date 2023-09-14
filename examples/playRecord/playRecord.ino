/******************************************************************
File:             playRecord.ino
Description:      send cmd to contrl record and play the recording 
Note:             -       
******************************************************************/

#include <BMV36T001.h>


BMV36T001 myBMV36T001;

uint8_t recordStatus = BMV36T001_RECORD_DISABLE;//Recording status
uint8_t currentKeyValue = 0;//Current key state
uint8_t lastKeyValue = BMV36T001_NO_KEY; //Last key state
uint8_t keycode = BMV36T001_NO_KEY;//Recorded keys
uint8_t volume = DEFAULT_VOLUME;

void setup() {
    // put your setup code here, to run once:
    myBMV36T001.begin();
    myBMV36T001.setPlayMode(BMV36T001_MODE_RECORD); //Set recording mode
    //===============================================
    //If you need to erase the recording, please add this sentence.
    myBMV36T001.recordErase(); 
    //===============================================
}

uint8_t keyPressDispose(uint8_t keyValue) 
{
    uint8_t result = BMV36T001_NO_KEY;
    if(lastKeyValue == BMV36T001_NO_KEY)//Didn't press the key last time
    {
        if((keyValue & BMV36T001_MIDDLE_KEY) != 0) {//Middle key pressed
            result  = BMV36T001_MIDDLE_KEY;//Record the keys pressed
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
    // put your main code here, to run repeatedly:
    myBMV36T001.scanKey();//Key scanning
    if(myBMV36T001.isKeyAction())//If there is a rising edge or a falling edge
    {
        currentKeyValue = myBMV36T001.readKeyValue();//Read key status
        keycode = keyPressDispose(currentKeyValue) ;//Call the key press analysis function
        if(keycode != BMV36T001_NO_KEY) //If a key is pressed
        {
            switch(keycode)//Judge and dispose key
            {
                case BMV36T001_MIDDLE_KEY://The middle key is pressed
                    if(recordStatus == BMV36T001_RECORD_DISABLE) {
                        myBMV36T001.recordStart();//Start recording
                        recordStatus = BMV36T001_RECORD_ENABLE;
                    }
                    else {
                        myBMV36T001.recordStop();//Recording ends
                        recordStatus = BMV36T001_RECORD_DISABLE;
                    }
                    break;
                case BMV36T001_UP_KEY://The up key is pressed   
                    if(volume < BMV36T001_MAX_VOLUME)
                    {
                        volume++;
                        myBMV36T001.setVolume(volume); //Set volume
                    }
                    break;
                case BMV36T001_DOWN_KEY://The down key is pressed
                    if(volume > BMV36T001_MIN_VOLUME)
                    {
                        volume--;   
                        myBMV36T001.setVolume(volume);//Set volume
                    }
                    break;
                case BMV36T001_LEFT_KEY://The left key is pressed
                    myBMV36T001.playPrevious();//Play the last recording
                    break;
                case BMV36T001_RIGHT_KEY://The right key is pressed
                    myBMV36T001.playNext(); //Play the next recording
                    break;
                default:
                    break;     
            } 
            keycode = BMV36T001_NO_KEY; 
        }
    }
}
