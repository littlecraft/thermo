#include <Wire.h>
#include <SPI.h>
#include <FastLED.h>
#include <cstdint>

#include <pins_arduino.h>
#include <wiring_private.h>

#define ADDRESS  (0x2A)

#define AGC (0x01)
#define SYS (0x02)
#define VID (0x03)
#define OEM (0x08)

#define GET (0x00)
#define SET (0x01)
#define RUN (0x02)

typedef struct range_image {
  virtual const uint16_t* get_pels() const = 0;
  virtual uint16_t get_width() const = 0;
  virtual uint16_t get_height() const = 0;
  virtual uint16_t get_range_min() const = 0;
  virtual uint16_t get_range_max() const = 0;
};

template <int CSPin>
class lepton_imager : public range_image {
private:
  enum {
    kLeptonPacketLen = 164,
    kLeptonImageWidth = 80,
    kLeptonImageHeight = 60,
    kMisReadResetDelayUs = 1000
  };

public:
  lepton_imager() {
    minValue = UINT16_MAX;
    maxValue = 0;
  }

  void read_frame()
  {
    const uint8_t bytes_per_packet = kLeptonPacketLen / 2;
    
    for (int line = 0; line < kLeptonImageHeight; ++line) {
      uint8_t* p = packet;
      for (int b = 0; b < bytes_per_packet; ++b) {
  
        digitalWrite(CSPin, LOW);
        
        //  send in the address and value via SPI:
        *p++ = SPI.transfer(0x00);
        *p++ = SPI.transfer(0x00);
    
        digitalWrite(CSPin, HIGH);
      }
  
      uint8_t line_number = packet[1];
      if (line_number != line) {
        --line;
        delayMicroseconds(kMisReadResetDelayUs);
        continue;
      }
  
      if (line_number < kLeptonImageHeight) {
        // Skip 2 byte frame ID, and 2 byte CRC
        uint8_t* p = packet + 4;
        
        for (int x = 0; x < kLeptonImageWidth; ++x) {
          uint16_t value = *p++ << 8 | *p++;
          
          if (value < minValue)
            minValue = value;
          if (value > maxValue)
            maxValue = value;

          image[line_number][x] = value;
        }
      }
    }
  }

  const uint16_t* get_pels() const {
    return (uint16_t*)image;
  }

  uint16_t get_width() const {
    return kLeptonImageWidth;
  }

  uint16_t get_height() const {
    return kLeptonImageHeight;
  }

  uint16_t get_range_min() const {
    return minValue;
  }

  uint16_t get_range_max() const {
    return maxValue;
  }
  
  uint8_t packet[kLeptonPacketLen];
  uint16_t image[kLeptonImageHeight][kLeptonImageWidth];

  uint16_t minValue;
  uint16_t maxValue;
};


class heat_palette {
public:
  void render_into(CRGB* out_pels, uint16_t numPels, const range_image* image) {
    const int line = 30;
    
    uint16_t w = image->get_width();
    uint16_t h = image->get_height();
    const uint16_t* in_pels = image->get_pels() + (line * w);
    
    uint16_t minVal = image->get_range_min();
    uint16_t maxVal = image->get_range_max();
  
    for (int x = 0; x < numPels; ++x) {
      uint8_t index = map(in_pels[x], minVal, maxVal, 0, UINT8_MAX);
      out_pels[x] = map_color(index);
    }
  }
  
private:
  inline uint32_t map_color(uint8_t index) {
    uint8_t r = color_map[3 * index];
    uint8_t g = color_map[3 * index + 1];
    uint8_t b = color_map[3 * index + 2];
    uint8_t w = 0;
  
   return (uint32_t)((w << 24) | (g << 16) | (r << 8) | b);
  }

  static const uint8_t color_map[];
};

const uint8_t heat_palette::color_map[] = {
  255, 255, 255, 253, 253, 253, 251, 251, 251, 249, 249, 249,
  247, 247, 247, 245, 245, 245, 243, 243, 243, 241, 241, 241, 239, 239, 239, 237, 237,
  237, 235, 235, 235, 233, 233, 233, 231, 231, 231, 229, 229, 229, 227, 227, 227, 225,
  225, 225, 223, 223, 223, 221, 221, 221, 219, 219, 219, 217, 217, 217, 215, 215, 215,
  213, 213, 213, 211, 211, 211, 209, 209, 209, 207, 207, 207, 205, 205, 205, 203, 203,
  203, 201, 201, 201, 199, 199, 199, 197, 197, 197, 195, 195, 195, 193, 193, 193, 191,
  191, 191, 189, 189, 189, 187, 187, 187, 185, 185, 185, 183, 183, 183, 181, 181, 181,
  179, 179, 179, 177, 177, 177, 175, 175, 175, 173, 173, 173, 171, 171, 171, 169, 169,
  169, 167, 167, 167, 165, 165, 165, 163, 163, 163, 161, 161, 161, 159, 159, 159, 157,
  157, 157, 155, 155, 155, 153, 153, 153, 151, 151, 151, 149, 149, 149, 147, 147, 147,
  145, 145, 145, 143, 143, 143, 141, 141, 141, 139, 139, 139, 137, 137, 137, 135, 135,
  135, 133, 133, 133, 131, 131, 131, 129, 129, 129, 126, 126, 126, 124, 124, 124, 122,
  122, 122, 120, 120, 120, 118, 118, 118, 116, 116, 116, 114, 114, 114, 112, 112, 112,
  110, 110, 110, 108, 108, 108, 106, 106, 106, 104, 104, 104, 102, 102, 102, 100, 100,
  100, 98, 98, 98, 96, 96, 96, 94, 94, 94, 92, 92, 92, 90, 90, 90, 88, 88, 88, 86, 86,
  86, 84, 84, 84, 82, 82, 82, 80, 80, 80, 78, 78, 78, 76, 76, 76, 74, 74, 74, 72, 72,
  72, 70, 70, 70, 68, 68, 68, 66, 66, 66, 64, 64, 64, 62, 62, 62, 60, 60, 60, 58, 58,
  58, 56, 56, 56, 54, 54, 54, 52, 52, 52, 50, 50, 50, 48, 48, 48, 46, 46, 46, 44, 44,
  44, 42, 42, 42, 40, 40, 40, 38, 38, 38, 36, 36, 36, 34, 34, 34, 32, 32, 32, 30, 30,
  30, 28, 28, 28, 26, 26, 26, 24, 24, 24, 22, 22, 22, 20, 20, 20, 18, 18, 18, 16, 16,
  16, 14, 14, 14, 12, 12, 12, 10, 10, 10, 8, 8, 8, 6, 6, 6, 4, 4, 4, 2, 2, 2, 0, 0,
  0, 0, 0, 9, 2, 0, 16, 4, 0, 24, 6, 0, 31, 8, 0, 38, 10, 0, 45, 12, 0, 53, 14, 0, 60,
  17, 0, 67, 19, 0, 74, 21, 0, 82, 23, 0, 89, 25, 0, 96, 27, 0, 103, 29, 0, 111, 31, 0,
  118, 36, 0, 120, 41, 0, 121, 46, 0, 122, 51, 0, 123, 56, 0, 124, 61, 0, 125, 66, 0,
  126, 71, 0, 127, 76, 1, 128, 81, 1, 129, 86, 1, 130, 91, 1, 131, 96, 1, 132, 101, 1,
  133, 106, 1, 134, 111, 1, 135, 116, 1, 136, 121, 1, 136, 125, 2, 137, 130, 2, 137,
  135, 3, 137, 139, 3, 138, 144, 3, 138, 149, 4, 138, 153, 4, 139, 158, 5, 139, 163,
  5, 139, 167, 5, 140, 172, 6, 140, 177, 6, 140, 181, 7, 141, 186, 7, 141, 189, 10,
  137, 191, 13, 132, 194, 16, 127, 196, 19, 121, 198, 22, 116, 200, 25, 111, 203, 28,
  106, 205, 31, 101, 207, 34, 95, 209, 37, 90, 212, 40, 85, 214, 43, 80, 216, 46, 75,
  218, 49, 69, 221, 52, 64, 223, 55, 59, 224, 57, 49, 225, 60, 47, 226, 64, 44, 227,
  67, 42, 228, 71, 39, 229, 74, 37, 230, 78, 34, 231, 81, 32, 231, 85, 29, 232, 88, 27,
  233, 92, 24, 234, 95, 22, 235, 99, 19, 236, 102, 17, 237, 106, 14, 238, 109, 12, 239,
  112, 12, 240, 116, 12, 240, 119, 12, 241, 123, 12, 241, 127, 12, 242, 130, 12, 242,
  134, 12, 243, 138, 12, 243, 141, 13, 244, 145, 13, 244, 149, 13, 245, 152, 13, 245,
  156, 13, 246, 160, 13, 246, 163, 13, 247, 167, 13, 247, 171, 13, 248, 175, 14, 248,
  178, 15, 249, 182, 16, 249, 185, 18, 250, 189, 19, 250, 192, 20, 251, 196, 21, 251,
  199, 22, 252, 203, 23, 252, 206, 24, 253, 210, 25, 253, 213, 27, 254, 217, 28, 254,
  220, 29, 255, 224, 30, 255, 227, 39, 255, 229, 53, 255, 231, 67, 255, 233, 81, 255,
  234, 95, 255, 236, 109, 255, 238, 123, 255, 240, 137, 255, 242, 151, 255, 244, 165,
  255, 246, 179, 255, 248, 193, 255, 249, 207, 255, 251, 221, 255, 253, 235, 255, 255,
  24
};

void lepton_command(unsigned int moduleID, unsigned int commandID, unsigned int command, uint16_t* data = NULL, int dataLen = 0)
{
  byte error;
  
  Wire.beginTransmission(ADDRESS);

  // Command Register is a 16-bit register located at Register Address 0x0004
  Wire.write(0x00);
  Wire.write(0x04);

  if (moduleID == 0x08) //OEM module ID
  {
    Wire.write(0x48);
  }
  else
  {
    Wire.write(moduleID & 0x0f);
  }
  
  Wire.write( ((commandID << 2 ) & 0xfc) | (command & 0x3));

  error = Wire.endTransmission();    // stop transmitting
  if (error != 0)
  {
    Serial.print("error=");
    Serial.println(error);
  }
}

void lepton_get_scene_roi()
{
  lepton_command(SYS, 0x30 >> 2, GET);

  uint16_t roi[4];
  int len = read_data(roi, 4);

  Serial.print("StartCol: ");
  Serial.println(roi[0]);
  Serial.print("StartRow: ");
  Serial.println(roi[1]);
  Serial.print("EndCol: ");
  Serial.println(roi[2]);
  Serial.print("EndRow: ");
  Serial.println(roi[3]);
}

void lepton_set_scene_roi(uint8_t startRow, uint8_t startCol, uint8_t endRow, uint8_t endCol)
{
  uint16_t roi[4] = {startCol, startRow, endCol, endRow};
  lepton_command(SYS, 0x30 >> 2, SET, roi, 4);
}

void agc_enable()
{
  byte error;
  Wire.beginTransmission(ADDRESS); // transmit to device #4
  Wire.write(0x01);
  Wire.write(0x05);
  Wire.write(0x00);
  Wire.write(0x01);

  error = Wire.endTransmission();    // stop transmitting
  if (error != 0)
  {
    Serial.print("error=");
    Serial.println(error);
  }
}

void set_reg(unsigned int reg)
{
  byte error;
  Wire.beginTransmission(ADDRESS); // transmit to device #4
  Wire.write(reg >> 8 & 0xff);
  Wire.write(reg & 0xff);            // sends one byte

  error = Wire.endTransmission();    // stop transmitting
  if (error != 0)
  {
    Serial.print("error=");
    Serial.println(error);
  }
}

//Status reg 15:8 Error Code  7:3 Reserved 2:Boot Status 1:Boot Mode 0:busy

int read_reg(unsigned int reg)
{
  int reading = 0;
  set_reg(reg);

  Wire.requestFrom(ADDRESS, 2);

  reading = Wire.read();  // receive high byte (overwrites previous reading)
  //Serial.println(reading);
  reading = reading << 8;    // shift high byte to be high 8 bits

  reading |= Wire.read(); // receive low byte as lower 8 bits
  /*
  Serial.print("reg:");
  Serial.print(reg);
  Serial.print("==0x");
  Serial.print(reading, HEX);
  Serial.print(" binary:");
  Serial.println(reading, BIN);
  */
  return reading;
}

int read_data()
{
  int i;
  int data;
  int payload_length;

  while (read_reg(0x2) & 0x01)
  {
    Serial.println("busy");
    delay(1000);
  }

  payload_length = read_reg(0x6);
  Serial.print("payload_length=");
  Serial.println(payload_length);

  Wire.requestFrom(ADDRESS, payload_length);
  //set_reg(0x08);
  for (i = 0; i < (payload_length / 2); i++)
  {
    data = Wire.read() << 8;
    data |= Wire.read();
    Serial.println(data, HEX);
  }

  return data;
}

int read_data(uint16_t* buf, int maxLen)
{
  int i;
  int data;
  int payload_length;

  while (read_reg(0x2) & 0x01)
  {
    Serial.println("busy");
    delay(1000);
  }

  payload_length = read_reg(0x6);

  Wire.requestFrom(ADDRESS, payload_length);
  //set_reg(0x08);
  for (i = 0; i < (payload_length / 2) && i < maxLen; i++)
  {
    data = Wire.read() << 8;
    data |= Wire.read();

    buf[i] = data;
  }

  return payload_length;
}

const uint8_t kLeptonCSPin = 10;
lepton_imager<kLeptonCSPin> imager;
heat_palette palette;

const uint8_t kLedPin = 2;
const uint16_t kPelCount = 80;
CRGB pels[kPelCount];

void setup() {
  Wire.begin();
  Serial.begin(115200);

  FastLED.addLeds<WS2814, kLedPin>(pels, kPelCount);
  fill_solid(pels, kPelCount, CRGB::Black);
  FastLED.show();

  pinMode(10, OUTPUT);
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(0);

  SPI.begin();

  read_reg(0x2);

  Serial.println("SYS Camera Customer Serial Number");
  lepton_command(SYS, 0x28 >> 2 , GET);
  read_data();

  Serial.println("SYS Flir Serial Number");
  lepton_command(SYS, 0x2 , GET);
  read_data();

  Serial.println("SYS Camera Uptime");
  lepton_command(SYS, 0x0C >> 2 , GET);
  read_data();

  Serial.println("SYS Fpa Temperature Kelvin");
  lepton_command(SYS, 0x14 >> 2 , GET);
  read_data();

  Serial.println("SYS Aux Temperature Kelvin");
  lepton_command(SYS, 0x10 >> 2 , GET);
  read_data();

  Serial.println("OEM Chip Mask Revision");
  lepton_command(OEM, 0x14 >> 2 , GET);
  read_data();

  //Serial.println("OEM Part Number");
  //lepton_command(OEM, 0x1C >> 2 , GET);
  //read_data();

  Serial.println("OEM Camera Software Revision");
  lepton_command(OEM, 0x20 >> 2 , GET);
  read_data();

  Serial.println("AGC Enable");
  //lepton_command(AGC, 0x01  , SET);
  agc_enable();
  read_data();

  Serial.println("AGC READ");
  lepton_command(AGC, 0x00  , GET);
  read_data();

  Serial.println("ROI");
  lepton_get_scene_roi();
}

void loop() {
  imager.read_frame();
  palette.render_into(pels, kPelCount, &imager);
  FastLED.show();
}
