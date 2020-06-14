#include <Arduino.h>

// cfg-tp5 message: time pulse parameters
// with tis message the time pulse can be configured
// it is also the reponse to a poll message, confirming the actual settings of the module
// notes                          | crc starts here |     | little endian  |         |         |           | ns            | ns           | set to freq (Hz)      |                       | set to ratio 2^-32    |                       | ns                    | utc                   | CRC
// description          header    | class           | id  | length (bytes) | tpIndex | version | reserved  | antCableDelay | rfGroupDelay | freqPeriod            | freqPeriodLock        | pulseLenRatio         | pulseLenRatioLock     | userConfigDelay       | flags                 | ck_a  ck_b
// decimal                        |                 |     | 32             |         |         |           | 50            | 0            | 10                    | 1                     | 3.435.973.837 => 80%  | 858.993.460 => 20%    | 0                     |                       |
uint8_t cfg_tp5_32[] = {0xB5, 0x62, 0x06,             0x31, 0x20, 0x00,      0x00,     0x01,     0x00, 0x00, 0x32, 0x00,     0x00, 0x00,    0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0xCC, 0xCC, 0x34, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x79, 0xFC};

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

void setup()
{
  Serial.begin(9600); // usb is always 12Mbit/s
}

void loop()
{
  if (Serial.available()) {
    Serial.write(cfg_tp5_1, 9);
    delay(500);
    Serial.write(cfg_tp5_32, 40);
    delay(500);
    Serial.write(cfg_rst_4, 12);
    delay(1000);
    calcCRC(cfg_tp5_1);
    calcCRC(cfg_tp5_32);
    calcCRC(cfg_rst_4);
    delay(1000);
    Serial.write(cfg_tp5_1, 9);
    delay(500);
    Serial.write(cfg_tp5_32, 40);
    delay(500);
    Serial.write(cfg_rst_4, 12);
    delay(5000);
  }
}

// calculate the crc for the message
// the whole message is given (with header and crc bytes)
// header is ignored, crc bytes are filled in
// the lenght is extracted from the length field
void calcCRC(uint8_t* message) {
  uint8_t ck_a = 0;
  uint8_t ck_b = 0;

  // subtract 4 for header and crc bytes
  uint16_t length = sizeof(message) - 4;

  // advance 2 adresses to skip header
  message++;
  message++;

  for (uint16_t i = 0; i < length; i++) {
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