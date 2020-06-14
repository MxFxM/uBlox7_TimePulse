#include <Arduino.h>

// ----------------------------------------------------------------------------
// uncomment lines to your taste:
//#define FORCE_COLDSTART
// ----------------------------------------------------------------------------

#define USB Serial
#define GPS Serial2

// cfg-tp5 message: time pulse parameters
// with tis message the time pulse can be configured
// it is also the reponse to a poll message, confirming the actual settings of the module
// notes                          | crc starts here |     | little endian  |         |         |           | ns            | ns           | set to freq (Hz)      |                       | set to ratio 2^-32    |                       | ns                    | utc                   | CRC
// description          header    | class           | id  | length (bytes) | tpIndex | version | reserved  | antCableDelay | rfGroupDelay | freqPeriod            | freqPeriodLock        | pulseLenRatio         | pulseLenRatioLock     | userConfigDelay       | flags                 | ck_a  ck_b
// decimal                        |                 |     | 32             |         |         |           | 50            | 0            | 10                    | 255                   | 3.435.973.837 => 80%  | 858.993.460 => 20%    | 0                     |                       |
uint8_t cfg_tp5_32[] = {0xB5, 0x62, 0x06,             0x31, 0x20, 0x00,      0x00,     0x01,     0x00, 0x00, 0x32, 0x00,     0x00, 0x00,    0x0A, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0xCC, 0xCC, 0x34, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x79, 0xFC};

// cfg-tp5 message: poll time pulse parameters
// this message is sent after setting new parameters
// the gps module shall respond with the same message as we send, confirming the correct settings
// notes                         |       |     |           |                 | CRC
// description         header    | class | id  | length    | pulse selection | ck_a  ck_b
// decimal                       |       |     | 1         |                 |
uint8_t cfg_tp5_1[] = {0xB5, 0x62, 0x06,   0x31, 0x01, 0x00, 0x00,             0x38, 0xE5};

// cfg-rst message: reset receiver / clear backup data structures
// this message can reset the gps module with cold/warm/hot start
// the shown configuration is a cold start example
// notes                         |       |     |           |           |           |          | CRC
// description         header    | class | id  | length    | coldstart | resetMode | reserved | ck_a  ck_b
// decimal                       |       |     | 4         |           |           |          |
uint8_t cfg_rst_4[] = {0xB5, 0x62, 0x06,   0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02,       0x00,      0x0E, 0x61};

void calcCRC(uint8_t* message);
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock);

void setup()
{
  USB.begin(9600); // usb is always 12Mbit/s
  GPS.begin(9600);

  #ifdef FORCE_COLDSTART
  calcCRC(&cfg_rst_4[0]);
  GPS.write(cfg_rst_4, sizeof(cfg_rst_4));
  #endif

  configureTimepulse(1, 1000, 0.1, 0.9);

  //calcCRC(&cfg_tp5_32[0]);
  //Serial2.write(cfg_tp5_32, 40); // write to gps module
}

void loop()
{
}

// calculate the crc for the message
// the whole message is given (with header and crc bytes)
// header is ignored, crc bytes are filled in
// the lenght is extracted from the length field
void calcCRC(uint8_t* message) {
  uint8_t ck_a = 0;
  uint8_t ck_b = 0;

  uint16_t length = 8;

  // advance 2 adresses to skip header
  message++;
  message++;

  for (uint16_t i = 2; i < length - 2; i++) {
    if (i == 4) {
      length = length + *message;
    }
    if (i == 5) {
      length = length + (*message << 8);
    }
    // calculate the checksum
    ck_a = ck_a + *message;
    ck_b = ck_b + ck_a;

    message++; // advance pointer to next byte
  }

  // add crc to message
  *message = ck_a;
  message++;
  *message = ck_b;
}

// send a message to set a new frequency and locked frequency
// this does not wait for an acknowledge, nor poll the actual settings
void configureTimepulse(uint32_t freq, uint32_t freqLock, double pulseRatio, double pulseRatioLock) {
  double factor = powl(2.0, -32.0);
  pulseRatio = 1 - pulseRatio;
  uint32_t dc = (pulseRatio / factor);
  pulseRatioLock = 1 - pulseRatioLock;
  uint32_t dcLock = (pulseRatioLock / factor);

  uint8_t message[40];
  message[0] =  0xB5; // header
  message[1] =  0x62; // header
  message[2] =  0x06; // class
  message[3] =  0x31; // id
  message[4] =  0x20; // length
  message[5] =  0x00; // length
  message[6] =  0x00; // time pulse selection
  message[7] =  0x01; // version
  message[8] =  0x00; // reserved
  message[9] =  0x00; // reserved
  message[10] = 0x32; // antenna cable delay (here fixed)
  message[11] = 0x00; // antenna cable delay
  message[12] = 0x00; // rf group delay (here fixed)
  message[13] = 0x00; // rf group delay
  message[14] = (freq >>  0) & 0xFF; // frequency
  message[15] = (freq >>  8) & 0xFF; // frequency
  message[16] = (freq >> 16) & 0xFF; // frequency
  message[17] = (freq >> 24) & 0xFF; // frequency
  message[18] = (freqLock >>  0) & 0xFF; // frequency on lock
  message[19] = (freqLock >>  8) & 0xFF; // frequency on lock
  message[20] = (freqLock >> 16) & 0xFF; // frequency on lock
  message[21] = (freqLock >> 24) & 0xFF; // frequency on lock
  message[22] = (dc >>  0) & 0xFF; // dutycycle
  message[23] = (dc >>  8) & 0xFF; // dutycycle
  message[24] = (dc >> 16) & 0xFF; // dutycycle
  message[25] = (dc >> 24) & 0xFF; // dutycycle
  message[26] = (dcLock >>  0) & 0xFF; // dutycycle on lock
  message[27] = (dcLock >>  8) & 0xFF; // dutycycle on lock
  message[28] = (dcLock >> 16) & 0xFF; // dutycycle on lock
  message[29] = (dcLock >> 24) & 0xFF; // dutycycle on lock
  message[30] = 0x00; // user configured delay
  message[31] = 0x00; // user configured delay
  message[32] = 0x00; // user configured delay
  message[33] = 0x00; // user configured delay
  message[34] = 0xEF; // flags
  message[35] = 0x00; // flags
  message[36] = 0x00; // flags
  message[37] = 0x00; // flags
  message[38] = 0x00; // crc will be included after calculation
  message[39] = 0x00; // crc will be included after calculation

  calcCRC(&message[0]);
  GPS.write(message, sizeof(message));
}