/* 
 *  GHMidiDrums
 * Une modification du code de Evan kale afin de le rendre compatible midi over usb
 * Utilise Teeonardu
 * Il fait compiler ce code en mode Teeonardu, 16mhz, midi, sinon le compilateur ne compilera pas.
 * 
 * Edit par Farewell_
 * Contact : Discord@ Farewell_#0908
 */
 
//constants
#define NUM_PIEZOS 6 
#define CHANNEL  1

//Max velocity (Midi max velocity = 127; 7 bits)
#define MAX_MIDI_VELOCITY 127

//Program defines
//ALL TIME MEASURED IN MILLISECONDS
#define SIGNAL_BUFFER_SIZE 100
#define PEAK_BUFFER_SIZE 30
#define MAX_TIME_BETWEEN_PEAKS 20
#define MIN_TIME_BETWEEN_NOTES 50

//Ring buffers to store analog signal and peaks
short currentSignalIndex[NUM_PIEZOS];
short currentPeakIndex[NUM_PIEZOS];
unsigned short signalBuffer[NUM_PIEZOS][SIGNAL_BUFFER_SIZE];
unsigned short peakBuffer[NUM_PIEZOS][PEAK_BUFFER_SIZE];

boolean noteReady[NUM_PIEZOS];
unsigned short noteReadyVelocity[NUM_PIEZOS];
boolean isLastPeakZeroed[NUM_PIEZOS];

unsigned long lastPeakTime[NUM_PIEZOS];
unsigned long lastNoteTime[NUM_PIEZOS];

// The list with the corresponding note numbers
int notes[NUM_PIEZOS] = { 
  0x46, //70
  0x47, //71
  0x48, //72
  0x49, //73
  0x4A, //74
  0x4B  //76
};

// The list with the numbers of the pins that have a pad connected
unsigned short pins[NUM_PIEZOS] = { 
  0,1,2,3,4,5 //Pins 0 to 5
};

//Thresholds
unsigned short thresholdMap[NUM_PIEZOS] = { 
  30,  //Snare
  30,  //Left Tom
  30,  //Right Tom
  100, //Left Cymbal
  100, //Right Cymbal
  50   //Kick
};



void setup() {
  //initialize globals
  for(short i=0; i<NUM_PIEZOS; ++i)
  {
    currentSignalIndex[i] = 0;
    currentPeakIndex[i] = 0;
    memset(signalBuffer[i],0,sizeof(signalBuffer[i]));
    memset(peakBuffer[i],0,sizeof(peakBuffer[i]));
    noteReady[i] = false;
    noteReadyVelocity[i] = 0;
    isLastPeakZeroed[i] = true;
    lastPeakTime[i] = 0;
    lastNoteTime[i] = 0;    
  }
  
  for(int i = 0; i < NUM_PIEZOS; i++){  // Set all switch-pins as input, and enable the internal pull-up resistor.
    pinMode(pins[i], INPUT_PULLUP);
  }
  pinMode(13, OUTPUT);      // Set pin 13 (the one with the LED) to output
  digitalWrite(13, LOW);    // Turn off the LED
  delay(3000);              // Wait 3 second before sending messages, to be sure everything is set up, and to make uploading new sketches easier.
  digitalWrite(13, HIGH);   // Turn on the LED, when the loop is about to start.
}


void loop(){
  unsigned long currentTime = millis();                                //Read current time
  
  for(short i=0; i<NUM_PIEZOS; ++i){ 
    unsigned short newSignal = analogRead(pins[i]);                    //get a new signal from analog read
    signalBuffer[i][currentSignalIndex[i]] = newSignal;
    
    if(newSignal < thresholdMap[i]){                                   //if new signal is below threshold
      if(!isLastPeakZeroed[i] && (currentTime - lastPeakTime[i]) > MAX_TIME_BETWEEN_PEAKS) recordNewPeak(i,0); //Record a new peak (new note hit)
      else{                                                            //get previous signal
        short prevSignalIndex = currentSignalIndex[i]-1; 
        if(prevSignalIndex < 0) prevSignalIndex = SIGNAL_BUFFER_SIZE-1;
        unsigned short prevSignal = signalBuffer[i][prevSignalIndex];
        unsigned short newPeak = 0;
        
        while(prevSignal >= thresholdMap[i]){                          //find the wave peak if previous signal was not 0 by going through previous signal values until another 0 is reached
          if(signalBuffer[i][prevSignalIndex] > newPeak) newPeak = signalBuffer[i][prevSignalIndex];        
          prevSignalIndex--;                                           //decrement previous signal index, and get previous signal
          if(prevSignalIndex < 0) prevSignalIndex = SIGNAL_BUFFER_SIZE-1;
          prevSignal = signalBuffer[i][prevSignalIndex];
        }
        if(newPeak > 0) recordNewPeak(i, newPeak);
      }
    }
    currentSignalIndex[i]++;
    if(currentSignalIndex[i] == SIGNAL_BUFFER_SIZE) currentSignalIndex[i] = 0;
  }
}


void recordNewPeak(short slot, short newPeak){
  isLastPeakZeroed[slot] = (newPeak == 0);
  unsigned long currentTime = millis();                 //Read current time
  
  lastPeakTime[slot] = currentTime;                     //Seting last peak time to current time
  peakBuffer[slot][currentPeakIndex[slot]] = newPeak;   //new peak recorded (newPeak)
  
  //get previous peak
  short prevPeakIndex = currentPeakIndex[slot]-1;
  if(prevPeakIndex < 0) prevPeakIndex = PEAK_BUFFER_SIZE-1;        
  unsigned short prevPeak = peakBuffer[slot][prevPeakIndex];
  
  if(newPeak > prevPeak && (currentTime - lastNoteTime[slot])>MIN_TIME_BETWEEN_NOTES){  //If new peak < prev peak (note still rising)
    noteReady[slot] = true;                                                             //Note ready to fire, waiting for prevPeak > newPeak
    if(newPeak > noteReadyVelocity[slot]) noteReadyVelocity[slot] = newPeak;
  }else if(newPeak < prevPeak && noteReady[slot]){                                      //prevPeak > newPeak and note ready to fire : fire note
    noteFire(notes[slot], noteReadyVelocity[slot]);
    noteReady[slot] = false;
    noteReadyVelocity[slot] = 0;
    lastNoteTime[slot] = currentTime;
  }
  
  currentPeakIndex[slot]++;
  if(currentPeakIndex[slot] == PEAK_BUFFER_SIZE) currentPeakIndex[slot] = 0;  
}



void noteFire(unsigned short note, unsigned short velocity){
  if(velocity > MAX_MIDI_VELOCITY)
    velocity = MAX_MIDI_VELOCITY;
    
  usbMIDI.sendNoteOn(note, velocity, CHANNEL);
  usbMIDI.sendNoteOff(note, velocity, CHANNEL);
  //midiNoteOn(note, velocity);
  //midiNoteOff(note, velocity);
}
