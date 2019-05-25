/* ******* */
/* Defines */
/* ******* */
// Serial
#define BAUD                    115200
// Button
#define buttonPin               4
#define BUTTON_STATE            (PIND&(1<<PORTD4))
// Leds
#define ledRPin                 5
#define LED_RED_ON              (PORTD |= (1<<PORTD5))
#define LED_RED_OFF             (PORTD &= ~(1<<PORTD5))
#define ledOPin                 6
#define LED_ORANGE_ON           (PORTD |= (1<<PORTD6))
#define LED_ORANGE_OFF          (PORTD &= ~(1<<PORTD6))
#define ledYPin                 1
#define LED_YELLOW_ON           (PORTB |= (1<<PORTB1))
#define LED_YELLOW_OFF          (PORTB &= ~(1<<PORTB1))
// Potentiometers (empirical values)
#define MAX_POT_VOL_VALUE       1023
#define MAX_POT_SENS_VALUE      1023
#define POT_PITCH_SENS_FACTOR   5 // Multiplying factor for pitch sensitivty (conversion from pitch_v to add_val)
// Debug
#define DEBUG_VOL               0
#define DEBUG_PITCH             0
#define DEBUG_POT               0
#define DEBUG_TIME              512
#define DEBUG_SERIAL(string, param) \
do {serCounter++;                   \
    if (serCounter==DEBUG_TIME){    \
      Serial.print(string);         \
      Serial.println(param);        \
      serCounter = 0;               \
    }                               \
  }                                 \
  while (0)

// Sweep Function Defines
#define SWEEP_DURATION_ADDVAL   65535
#define SWEEP_DURATION_PITCHVAL 65535
#define SWEEP_ADDVAL_INC        5 // Increment per add_val in 'ticks' (each tick is 32us)
// ISR1 Pitch Capture Event and Clear
#define CAPTURE_EVENT           (TIFR1 & (1<<ICF1))                     /* Event */
#define CLEAR_CAPTURE           do { TIFR1 = (1<<ICF1); } while (0)     /* Clear */
// Validate Pitch and Volumen Range
#define CHECK_RANGE             0
#define MIN_VALID_V             19000
#define MAX_VALID_V             21000
#define MIN_VALID_P             15000
#define MAX_VALID_P             17000
// Pitch/Volume
#define PITCH_OFFSET      200
#define PITCH_CEILING     33554432 // Equals 2^25 (Bit 25 set to 1)
#define VOL_OFFSET        0
#define VOL_CEILING_CALIB 4095     // Equals 2^12 - 1 (11 LSB set to 1)
#define VOL_CEILING       (2*(VOL_CEILING_CALIB+1)) // Equals 8192
#define VOL_OFF_CTR_FLAG  1024
#define VOL8_MAX_VALUE    255
#define VOL8_MED_VALUE    64       // Human volume perception is not linear
/* Pitch/Vol Filter values (divisions made up of right shifts) */
#define PFILT            2       /* Pitch Filtering Coefficient (integer between 0 and 16) */
#define VFILT            4       /* Volume Filtering Coefficient (integer between 0 and 16) */
/* Init values */
#define INIT_VOL         63      /* From 0 to 255  */
/* Song Buffer Length */
#define SONG_BUF         32
#define MIDI_SONG_BUF    128

/* Some debug/cleaning code constants */
#define MCP_DAC_BASE         2048
#define INT0_STATE           (PIND & (1<<PORTD2))
#define PC_STATE             (PINB & (1<<PORTB0))
#define INIT_TIMEOUT_VOL     624 /* Default 312 */
#define INIT_TIMEOUT_PITCH   624 /* Default 312 */

#define WELCOME_SOUNDS       0

/* ********************  */
/* Typedef declarations */
/* ********************  */
typedef enum {
  CALIB_PITCH = 1,
  CALIB_VOL,
  NO_VOLUME,
  NORMAL
} CalibMode;


typedef enum {
  IDLE = 0,
  PLAY_SONG,
  DO_SWEEPS
} CalibModeAdd;  /* Additional calibration modes */


typedef enum {
  RUNNING = 0,
  CALIBRATING
} CalibState;


typedef enum {
  PITCH,
  VOLUME
} Signal;


typedef struct{
  uint16_t notes[SONG_BUF];
  uint16_t durations[SONG_BUF];
  uint16_t volumes[SONG_BUF];
} Song;


typedef struct{
  uint8_t  midiNotes[MIDI_SONG_BUF];
  uint32_t midiTicks[MIDI_SONG_BUF];
  uint8_t  midiVolumes[MIDI_SONG_BUF];
} MidiSong;


/* Indexes used to play MidiSongs - Convert Midi Index to Frequency */
uint16_t midi_freq_idx[128] = {
    16, 17, 18, 19, 20, 21, 23, 24, 25, 27,
    29, 30, 32, 34, 36, 38, 41, 43, 46, 49,
    51, 55, 58, 61, 65, 69, 73, 77, 82, 87,
    92, 98, 103, 110, 116, 123, 130, 138,
    146, 155, 164, 174, 185, 196, 207, 220,
    233, 246, 261, 277, 293, 311, 329, 349,
    369, 392, 415, 440, 466, 493, 523, 554,
    587, 622, 659, 698, 739, 783, 830, 880,
    932, 987, 1046, 1108, 1174, 1244, 1318,
    1396, 1479, 1567, 1661, 1760, 1864,
    1975, 2093, 2217, 2349, 2489, 2637,
    2793, 2959, 3135, 3322, 3520, 3729,
    3951, 4186, 4434, 4698, 4978, 5274,
    5587, 5919, 6271, 6644, 7040, 7458,
    7902}; /* Last B8N corresponds to index 107  */


/* ********************  */
/* Function declarations */
/* ********************  */
static inline uint32_t mul_16_8(uint16_t a, uint8_t b);
void ticktimer (uint16_t ticks);
void tickms (uint32_t msecs);
void InitValues(void);

void playWelcomeSounds(void);
void playCalibrationSounds(void);
void playSong(Song* song);
void playMidiSong(MidiSong* song);
void sweepAddVal(int ticks);
void sweepPitch16(int ticks, boolean bypass);

void LEDSon(void);
void LEDSoff(void);
void serialInitShow(boolean show);

inline void calibrateTheremin(void);
inline void nextCalibMode(CalibMode* cur_mode, CalibModeAdd* cur_mode_add);
inline void beep(CalibMode mode);
inline void updateValue(Signal signal);
inline void checkRange(Signal signal);
inline void filterValue(Signal signal);
inline void truncateVolValue(void);
inline void handlePitch(void);
inline void handleVol(void);
inline void getPotValues(void);

void setupPins(void);
void setupTimer1(void);
void setupInterrupts(void);
void setupSweeps(void);




/* **************** */
/* Global variables */
/* **************** */
int32_t pitch_init = 0;         // Initialization value of pitch
int32_t vol_init = 0;           // Initialization value of volume

int32_t pitch_v,pitch_l;        // Last value of pitch (for filtering)
int32_t vol_v,vol_l;            // Last value of volume (for filtering)

uint16_t pitch = 0;             // Pitch value
uint16_t pitch_counter = 0;     // Pitch counter
uint16_t pitch_counter_l = 0;   // Last value of pitch counter

uint16_t vol = 0;               // Volume value
uint16_t vol_counter_l = 0;     // Last value of volume counter

CalibState state = RUNNING;     // State in the calibration state machine
CalibMode mode = NORMAL;        // Calibration mode
CalibModeAdd mode_add = IDLE;   // Additional modes (midi songs, sweeps, etc..)
uint16_t serCounter = 0;

/* volatile variables  - used in the ISR Routine */
volatile uint8_t vol8;             // Volume byte
volatile boolean flag_vol = 0;     // Volume read flag
volatile uint16_t vol_counter = 0; // Volume counter

volatile uint16_t pointer = 0;     // Table pointer
volatile uint16_t add_val = 0;     // Table pointer increment
volatile boolean flag_pitch = 0;   // Pitch read flag

volatile uint16_t timer = 0;       // Timer value

uint16_t vol_off_counter_far = 0;
uint16_t vol_off_counter_close = 0;

/* Potentiometers */
int sensorPinVol  = A0;         // select the input pin for the potentiometer
int sensorPinSens = A1;         // select the input pin for the potentiometer
uint16_t sensorVolValue  = 0;   // variable to store the value coming from the sensor
uint16_t sensorSensValue = 0;   // variable to store the value coming from the sensor
boolean usePots = true;         // variable to determine if use pots

/* Debounce variables */
uint8_t debounce_p = 0;
uint8_t debounce_v = 0;


/* Test Songs: played on setup() routine if WELCOME_SOUNDS is set to true */
Song songTest = {
  .notes        = {C6N, D6N, E6N, F6N, G6N, F6N, E6N, D6N, C6N, 0},
  .durations    = {Sd , Sd , Sd , Sd , Sd , Sd , Sd , Sd , Qd , 0},
  .volumes      = {64 , 32 , 64 , 32,  64,  32 , 64 , 32 , 255, 0}
};

MidiSong songMary = {
  .midiNotes = {64,64,62,62,60,60,62,55,62,64,64,64,64,64,55,62,62,62,62,62,55,64,64,67,67,67,55,
                64,64,62,62,60,60,62,55,62,64,64,64,64,64,64,64,55,64,62,62,62,62,64,64,62,55,62,
                60,60,0},
  .midiTicks = {0,231,25,231,25,231,25,206,25,25,231,25,231,25,462,50,231,25,231,25,462,50,231,
                25,231,25,462,50,231,25,231,25,231,25,206,25,25,231,25,231,25,231,25,206,25,25,
                231,25,231,25,231,25,206,25,25,974,0},
  .midiVolumes = {72,0,72,0,71,0,79,0,0,85,0,78,0,74,0,75,0,77,0,75,0,82,0,84,0,75,0,73,0,69,0,
                  71,0,80,0,0,84,0,76,0,74,0,77,0,0,75,0,74,0,81,0,70,0,0,73,0,0}
};
