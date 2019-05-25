/*
 *  Open.Theremin.UNO control software for Arduino.UNO
 *  Version 1.2
 *  Copyright (C) 2010-2013 by Urs Gaudenz
 *
 *  Open.Theremin.UNO control software is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Open.Theremin.UNO control software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  the Open.Theremin.UNO control software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Version 1.2A - minor 'arduino-izing' - Michael Margolis 20 Oct 2013
 * Version 1.2B - Configure for Larumbe's PCB - Gonzalo Larumbe Jun 2016
 *
 */

// * Includes
#include "mcpDac.h"
#include "theremin_sintable.c"
#include "freq_tempo_table.h"
#include "Theremin.h"


// * General Setup Routine
void setup() {
  Serial.begin(BAUD);
  setupPins();
  setupTimer1();
  setupInterrupts();
  mcpDacInit();

  LEDSon();

  playCalibrationSounds();
  if (WELCOME_SOUNDS) {
      playWelcomeSounds();
      playSong(&songTest);
  }

  InitValues();
  serialInitShow(true);

  LEDSoff();
}


// * Main Loop
void loop() {

 mloop:
  calibrateTheremin();
  handlePitch();
  handleVol();

  if(usePots)
    getPotValues();

  goto mloop;
}



// * Routines
// ** Assembly 16 bit by 8 bit multiplication
static inline uint32_t mul_16_8(uint16_t a, uint8_t b) {
  uint32_t product;
  asm (
       "mul %A1, %2\n\t"
       "movw %A0, r0\n\t"
       "clr %C0\n\t"
       "clr %D0\n\t"
       "mul %B1, %2\n\t"
       "add %B0, r0\n\t"
       "adc %C0, r1\n\t"
       "clr r1"
       :
       "=&r" (product)
       :
       "r" (a), "r" (b));
  return product;
}

// ** Wait for ticks (32 us)
void ticktimer (uint16_t ticks) {
    timer=0;
    while(timer<ticks)
        ;
}

// ** Wait x millisecs
void tickms (uint32_t msecs) {
    uint16_t ticks = (uint16_t)(msecs*FSAMPLING/1000);
    timer=0;
    while(timer<ticks)
        ;
}

// ** Initialize pitch and volume values for calibration
/* Wait for 2 consecutive values on ICR/Pitch and Timer1/Volume */
/* Gets value from ICR/Timer1, clears flags and waits till flags are set again to calculate init values */
void InitValues(void) {
    /* Set initial pitch value */
    flag_pitch = false;
    while (!flag_pitch) // Wait for new value (with exit after 10 ms)
        ;
    pitch_init = pitch;
    flag_pitch = false;

    /* Set initial volume value */
    flag_vol = false;
    while(!flag_vol) // Wait for new value (with exit after 10 ms)
        ;
    vol_init = vol;  // Counter change since last interrupt = init volume value
    flag_vol = false;
}


// * Own Auxiliary functions (readability purposes)
// ** Regular functions
void serialInitShow(boolean show){
  if (show == true) {
    Serial.print("vol_init = ");
    Serial.println(vol_init);
    Serial.print("pitch_init = ");
    Serial.println(pitch_init);
  }
}


void playWelcomeSounds(void){
  vol8 = INIT_VOL;     // Set volume to half (from 0 to 255)

  add_val = A3N << 1;
  tickms(500);

  add_val = A4N << 1;
  tickms(500);

  add_val = A5N << 1;
  tickms(1000);
}


void playCalibrationSounds(void){
  LED_ORANGE_ON;

  vol8 = VOL8_MAX_VALUE; // 1st beep
  add_val = A3N << 1;
  tickms(150);
  vol8 = 0;              // 1st Silence
  tickms(100);

  vol8 = VOL8_MAX_VALUE; // 2nd beep
  tickms(150);
  vol8 = 0;              // 2nd silence
  tickms(100);

  vol8 = VOL8_MAX_VALUE; // 3rd beep
  tickms(150);
  vol8 = 0;              // 3rd silence
  tickms(100);

  add_val = A4N << 1;    // 4th higher pitch beep
  vol8 = VOL8_MAX_VALUE;
  tickms(200);

  LED_ORANGE_OFF;
}


void playSong(Song* song){
    for (uint16_t i=0; song->notes[i] || song->durations[i] || song->volumes[i]; i++){
        /* add_val=F*L*64/Fs
           With L=1024, FSAMPLING=31250 and a 64 factor because add_val is 16 bits and pointer 10:
              - The result is multiplying by 2 or shifting left by 1 */
        add_val = song->notes[i] << 1;
        vol8 = song->volumes[i];
        ticktimer(song->durations[i]);
    }
}


void playMidiSong(MidiSong* midiSong){
    for (uint16_t i=0;
         midiSong->midiNotes[i] || midiSong->midiTicks[i] || midiSong->midiVolumes[i];
         i++){

        if(BUTTON_STATE == LOW)
            return;

        add_val = midi_freq_idx[midiSong->midiNotes[i]] << 1;
        vol8 = midiSong->midiVolumes[i];
        ticktimer((midiSong->midiTicks[i+1]) * 100);
    }
}


void sweepAddVal(int ticks){
    for (uint32_t i=0; i<SWEEP_DURATION_ADDVAL; i++){
        add_val = i;
        ticktimer(ticks);

        if(BUTTON_STATE == LOW)
            return;
    }
}


void sweepPitch16(int ticks, boolean bypass){
    for (uint32_t i=0; i<SWEEP_DURATION_PITCHVAL; i++){
        if (bypass == false)
            add_val = (uint16_t)((pitch_v - pitch_init) + 200);
        else
            add_val = pitch_v;

        pitch_v++;
        ticktimer(ticks);

        if(BUTTON_STATE == LOW)
            return;
    }
}


void LEDSon(void){
    LED_RED_ON;
    LED_ORANGE_ON;
    LED_YELLOW_ON;
}


void LEDSoff(void){
    LED_RED_OFF;
    LED_ORANGE_OFF;
    LED_YELLOW_OFF;
}


void setupPins(void){
    pinMode(buttonPin,INPUT_PULLUP);
    pinMode(ledRPin, OUTPUT);
    pinMode(ledOPin, OUTPUT);
    pinMode(ledYPin, OUTPUT);
}


void setupTimer1(void){
    /* Setup Timer 1, 16 bit timer used to measure pitch and volume frequency */
    TCCR1A = 0;                    // Set Timer 1 to Normal port operation (Arduino does activate something here ?)
    TCCR1B = (1<<ICES1)|(1<<CS10); // Input Capture Positive edge select, Run without prescaling (16 Mhz)
    TIMSK1 = (1<<ICIE1);           // Enable Input Capture Interrupt. If activated, use ISR for this interrupts to handle it
}


void setupInterrupts(void){
    /* Setup interrupts for Wave Generator and Volume read */
    EICRA = (1<<ISC00) | (1<<ISC01) | (1<<ISC11) | (1<<ISC10) ; // Rising edges of INT0 and INT1 generate an interrupt request.
    EIMSK = (1<<INT0)|(1<<INT1);                                // Enable External Interrupt INT0 and INT1
    interrupts();                                               // Enable Interrupts (this is a macro for SEI)
}


void setupSweeps(void){
    sweepAddVal(SWEEP_ADDVAL_INC); // Sweeps add_val, no external pitch influence

    pitch_v = pitch_init;
    sweepPitch16(5, false); // Sweeps pitch_v, no bypass (normal operation)
    LED_ORANGE_OFF;
    LED_YELLOW_OFF;

    pitch_v = 0;
    sweepPitch16(5, true);  // Sweeps pitch_v, bypass init value (testing purposes)
}


// ** Inline functions
inline void beep(CalibMode mode){
    /* Beep 'n' times depending on mode. */
    for (uint8_t i=0; i<mode; i++) {
        vol8 = VOL8_MAX_VALUE; // Beep ...
        add_val = 3000;
        ticktimer(1500);

        vol8 = 0;              // ... and silence
        ticktimer(1500);

        vol8 = VOL8_MAX_VALUE; // Reset
        add_val = 0;
    }
}


inline void nextCalibMode(CalibMode* cur_mode, CalibModeAdd* cur_mode_add) {
    switch (*cur_mode){

    case CALIB_PITCH:
        *cur_mode = CALIB_VOL;
        break;

    case CALIB_VOL:
        *cur_mode = NO_VOLUME;
        break;

    case NO_VOLUME:
        *cur_mode = NORMAL;
        break;

    case NORMAL:
        if (*cur_mode_add == IDLE)
            *cur_mode_add = PLAY_SONG;
        else if (*cur_mode_add == PLAY_SONG)
            *cur_mode_add = DO_SWEEPS;
        else if (*cur_mode_add == DO_SWEEPS){
            *cur_mode_add = IDLE;
            *cur_mode = CALIB_PITCH;
        }
        else {
            *cur_mode_add = IDLE; // Default, never supposed to reach this
            *cur_mode = CALIB_PITCH;
        }
        break;
    }
}


inline void calibrateTheremin(void){
    /* Check if Key Released */
    if ((state == RUNNING) && (BUTTON_STATE == LOW)) { // Check if key released
        state = CALIBRATING;
        timer = 0;
    }

    /* Key Pressed */
    if ((state == CALIBRATING) && (BUTTON_STATE != LOW)){ // If key pressed
        if (timer > 1500) {
            InitValues(); // Capture calibration Values
            state = RUNNING;
            mode = NORMAL;
        }
        else
            state = RUNNING;

    }

    /* Switch calibration modes */
    if ((state == CALIBRATING) && (timer > 20000)){ // If key pressed for >64 ms switch calibration modes
        LED_YELLOW_ON;
        state = RUNNING;
        nextCalibMode(&mode, &mode_add);
        beep(mode);

        if (mode_add==PLAY_SONG){
            vol8 = VOL8_MED_VALUE;
            playMidiSong(&songMary);
        }

        else if (mode_add==DO_SWEEPS){
            vol8 = VOL8_MED_VALUE;
            setupSweeps(); // Sweep functions
        }

        while (BUTTON_STATE == LOW) // Button pressed
            ;


        LED_YELLOW_OFF;
    }

}


inline void getPotValues(void){
    sensorVolValue = analogRead(sensorPinVol);    // Read Volume Pot
    sensorSensValue = analogRead(sensorPinSens);  // Read Sensitivity Pot

    if (DEBUG_POT) {
        DEBUG_SERIAL("SensValue->", sensorSensValue);
        DEBUG_SERIAL("VolValue->", sensorVolValue);
    }
}


inline void updateValue(Signal signal){
    if (signal == PITCH){
        pitch_counter = ICR1;                       // Get Timer-Counter 1 value
        pitch = (pitch_counter - pitch_counter_l);  // Counter change since last interrupt -> pitch value
        pitch_counter_l = pitch_counter;            // Set actual value as new last value
    }

    else{ // (signal == VOLUME)
        vol = (vol_counter - vol_counter_l);        // Counter change since last interrupt
        vol_counter_l = vol_counter;                // Set actual value as new last value
    }
}


inline void checkRange(Signal signal){
    if (signal == PITCH)
        if ((pitch > MIN_VALID_P) && (pitch < MAX_VALID_P))
            LED_ORANGE_ON;
        else
            LED_ORANGE_OFF;

    else // (signal == VOLUME)
        if ((vol_v > MIN_VALID_V) && (vol_v < MAX_VALID_V))
            LED_YELLOW_ON;
        else
            LED_YELLOW_OFF;
}


inline void filterValue(Signal signal){
    if (signal == PITCH){
        pitch_v = pitch;    // Averaging pitch values
        pitch_v = pitch_l + ((pitch_v - pitch_l) >> PFILT);
        pitch_l = pitch_v;
    }

    else { // (signal == VOLUME)
        vol_v = vol;
        vol_v = vol_l + ((vol_v - vol_l) >> VFILT);
        vol_l = vol_v;
    }
}


inline void truncateVolValue(void){
    if (usePots)
        vol_v = (vol_v*sensorVolValue)/MAX_POT_VOL_VALUE; // Adjust POT Value

    if (DEBUG_VOL)
        DEBUG_SERIAL("vol_v->", vol_v); // Debug after applying VolPot attenuation


    if (vol_v>8000) {
        LED_RED_ON;
        vol8 = 0;
        LED_YELLOW_ON;
        vol_off_counter_far = 0;
    }

    else if (vol_v<(-VOL_OFFSET+32)){
        LED_RED_ON;
        vol8 = 0;
        LED_ORANGE_ON;
        LED_YELLOW_OFF;
        vol_off_counter_close = 0;
    }

    else {
        LED_RED_OFF;
        vol8 = (vol_v + VOL_OFFSET) >> 5; // If using range between 0 and 4096, shift right 4 positions
        vol_off_counter_far++;
        if (vol_off_counter_far == VOL_OFF_CTR_FLAG){
            LED_YELLOW_OFF;
            vol_off_counter_far = 0;
        }

        vol_off_counter_close++;
        if (vol_off_counter_close == VOL_OFF_CTR_FLAG){
            LED_ORANGE_OFF;
            vol_off_counter_close = 0;
        }
    }


    if (DEBUG_VOL)
        DEBUG_SERIAL("vol8->", vol8); // vol8 after scaling
}


inline void handlePitch(void){
    /* New PITCH value */
    if (flag_pitch) {
        if (DEBUG_PITCH) {
            DEBUG_SERIAL("pitch_v - pitch_l->", pitch);
            DEBUG_SERIAL("pitch_init - pitch_v->", pitch_init - pitch_v);
            DEBUG_SERIAL("pitch_v->", pitch_v);
        }
        if (CHECK_RANGE)
            checkRange(PITCH);


        filterValue(PITCH);

        switch (mode) {
        case CALIB_PITCH:
            add_val = (uint16_t) (PITCH_CEILING / pitch_v);
            break;

        case CALIB_VOL:
            break;

        case NO_VOLUME:
            add_val = (uint16_t) ((pitch_init - pitch_v) / 2 + PITCH_OFFSET);
            break;

        case NORMAL:
            if (usePots) // Default case with POT
                add_val = (uint16_t) ((pitch_init - pitch_v) / ((sensorSensValue*POT_PITCH_SENS_FACTOR/MAX_POT_SENS_VALUE)+1) + PITCH_OFFSET);
            else // Default Case without POT
                add_val = (uint16_t) ((pitch_init - pitch_v) / 2 + PITCH_OFFSET);
            break;
        }

        if (DEBUG_PITCH) {
            DEBUG_SERIAL("pointer->", (unsigned int)(pointer>>6) & 0x3ff);
            DEBUG_SERIAL("add_val->", add_val);
        }

        flag_pitch = false;
    }
}


inline void handleVol(void){
    /* New VOLUME value */
    if (flag_vol) {
        if (DEBUG_VOL) {
            DEBUG_SERIAL("vol_counter-vol_counter_l->", vol);
            DEBUG_SERIAL("vol_init - vol_v->", vol_init-vol_v);
        }

        if (CHECK_RANGE)
            checkRange(VOLUME);


        filterValue(VOLUME);

        switch (mode){
        case CALIB_PITCH:
            vol_v = VOL_CEILING;
            break;

        case CALIB_VOL:
            add_val = PITCH_CEILING / vol_v;
            vol_v = VOL_CEILING_CALIB;
            break;

        case NO_VOLUME:
            vol_v = VOL_CEILING_CALIB;
            break;

        case NORMAL:
            vol_v = VOL_CEILING - (vol_init - vol_v);
            if (DEBUG_VOL)
                DEBUG_SERIAL("vol_v->", vol_v);
            break; // normal operation

        }

        truncateVolValue();
        flag_vol = false;   // Clear volume flag
    }
}


// * ISRs
ISR (INT1_vect) {
    /* Externaly generated 31250 Hz Interrupt for WAVE generator (32us) */
    /* Interrupt takes up a total of max 25 us */
    EIMSK &= ~ (1<<INT1); // Disable External Interrupt INT1 to avoid recursive interrupts
    sei();                // Enable Interrupts to allow counter 1 interrupts - asm volatile routine

    int16_t waveSample;    // temporary variable 1
    uint32_t scaledSample; // temporary variable 2
    uint16_t offset = (uint16_t)(pointer>>6) & 0x3ff;

    waveSample = (signed int)pgm_read_word_near(sine_table + offset);  // Read next wave table value (3.0us)

    if (waveSample>0) // multiply 16 bit wave number by 8 bit volume value (11.2us / 5.4us)
        scaledSample = MCP_DAC_BASE + (mul_16_8(waveSample,vol8) >> 9); // Result of multiplication will be 8+12=20 bits (vol8=8 bits, waveSample=12bits [-2047, 2047])
    else
        scaledSample = MCP_DAC_BASE - (mul_16_8(-waveSample,vol8) >> 9);

    mcpDacSend(scaledSample);    // Send result to Digital to Analogue Converter (audio out) (9.6 us)
    pointer = pointer + add_val; // increment table pointer (ca. 2us)
    timer++;                     // update 32us timer

    /* Debounce */
    if (PC_STATE) debounce_p++;
    if (debounce_p == 3) {
        cli();
        updateValue(PITCH);
    };

    if (debounce_p == 5) {
        flag_pitch = true;
    };

    if (INT0_STATE) debounce_v++;
    if (debounce_v == 3) {
        cli();
        updateValue(VOLUME);
    };

    if (debounce_v == 5) {
        flag_vol = true;
    };


    cli();              // Turn off interrupts - asm volatile routine
    EIMSK |= (1<<INT1); // Re-Enable External Interrupt INT1
}


ISR (INT0_vect) {
    /* VOLUME read - interrupt service routine for capturing volume counter value */
    vol_counter = TCNT1;
    debounce_v = 0;
}

ISR (TIMER1_CAPT_vect) {
    /* PITCH read - interrupt service routine for capturing pitch counter value */
    debounce_p = 0;
}
