/**************************************************************************/
/*!
    @file     ntag2xx_updatendef.pde
    @author   KTOWN (Adafruit Industries)
    @license  BSD (see license.txt)

    This example will wait for any NTAG203 or NTAG213 card or tag,
    and will attempt to add or update an NDEF URI at the start of the
    tag's memory.

    This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
    This library works with the Adafruit NFC breakout
      ----> https://www.adafruit.com/products/364

    Check out the links above for our tutorials and wiring diagrams
    These chips use SPI or I2C to communicate.

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!
*/
/**************************************************************************/
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Adafruit_NeoPixel.h>

// If using the breakout with SPI, define the pins for SPI communication.
#ifdef ESP8266
// If using the breakout with SPI on ESP8266, define the pins for SPI communication.
// Name  (PN532)  | ESP8266   | Color   | Arduino
// SCK (CLK)        GPIO14      orange    2
// MOSI             GPIO13      yellow    3
// MISO             GPIO12      blue      5
// SSEL             GPIO2       green     4
# define PN532_SCK  (14)
# define PN532_MOSI (13)
# define PN532_SS   (2)
# define PN532_MISO (12)

// SPI for Arduino
#else
# define PN532_SCK  (2)
# define PN532_MOSI (3)
# define PN532_SS   (4)
# define PN532_MISO (5)

#endif

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

#define BUTTON_PIN   4    // Digital IO pin connected to the button.
#define LED_PIN      5    // Digital IO pin connected to the status LED.

#define STATE_INIT 0
#define STATE_WAITING_FOR_BUTTON 1
#define STATE_WAITING_FOR_TAG 2

// Uncomment just _one_ line below depending on how your breakout or shield
// is connected to the Arduino:

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Use this line for a breakout with a hardware SPI connection.  Note that
// the PN532 SCK, MOSI, and MISO pins need to be connected to the Arduino's
// hardware SPI SCK, MOSI, and MISO pins.  On an Arduino Uno these are
// SCK = 13, MOSI = 11, MISO = 12.  The SS line can be any digital IO pin.
//Adafruit_PN532 nfc(PN532_SS);

// Or use this line for a breakout or shield with an I2C connection:
//Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

/*
    We can encode many different kinds of pointers to the card,
    from a URL, to an Email address, to a phone number, and many more
    check the library header .h file to see the large # of supported
    prefixes!
*/
// For a https:// url:
char * url = "springfieldfiles.com/sounds/homer/woohoo.mp3";
// uint8_t ndefprefix = NDEF_URIPREFIX_HTTPS;
uint8_t ndefprefix = NDEF_URIPREFIX_HTTP;

// for an email address
//char * url = "mail@example.com";
//uint8_t ndefprefix = NDEF_URIPREFIX_MAILTO;

// for a phone number
//char * url = "+1 212 555 1212";
//uint8_t ndefprefix = NDEF_URIPREFIX_TEL;

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
// also change #define in Adafruit_PN532.cpp library file
   #define Serial SerialUSB
#endif

bool btnState = HIGH;
int showType = 0;
unsigned long lastButtonRead = 0;
int debounceThreshold = 200;


int appState = STATE_INIT;

void setup(void) {
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(115200);
  Serial.flush();
  Serial.println("Hello!");

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  // nfc.setPassiveActivationRetries(0x99);

#ifdef ESP8266
  Serial.print("Using ESP8266");
#else
  Serial.print("Using Arduino");
#endif

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

void loop(void)
{
  if (appState == STATE_INIT)
  {
    state_entry_waiting_for_button();
  }
  // Get current button state & light the LED when pressed
  unsigned long ts = millis();
  if (appState == STATE_WAITING_FOR_TAG)
  {
    digitalWrite(LED_PIN,  HIGH);
  }
  else
  {
    digitalWrite(LED_PIN,  LOW);
  }

  if (ts - lastButtonRead >= debounceThreshold)
  {
    lastButtonRead = ts;
    bool newState = digitalRead(BUTTON_PIN);
    if (newState != btnState) {
      // Set the last button state to the old state.
      btnState = newState;
      Serial.print("Button state change:");
      Serial.println(btnState);

      // Check if state changed from high to low (button press).
      if (btnState == LOW && appState == STATE_WAITING_FOR_BUTTON)
      {
        state_entry_waiting_for_tag();
      }
      // button release
      else if (btnState == HIGH && appState == STATE_WAITING_FOR_TAG)
      {
        state_entry_waiting_for_button();
      }
      else
      {
        Serial.println("No action");
      }
    }
  }
}

void state_entry_waiting_for_button()
{
  appState = STATE_WAITING_FOR_BUTTON;
  Serial.println("Press and hold the button to update a NFC Tag");
}

void state_entry_waiting_for_tag()
{
  appState = STATE_WAITING_FOR_TAG;

  Serial.println("Place your NDEF formatted Mifare Classic card");
  Serial.println("or Ultralite tag on the reader to update the NDEF record");

  digitalWrite(LED_PIN,  HIGH);

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t dataLength;

  // 1.) Wait for an NTAG203 card.  When one is found 'uid' will be populated with
  // the UID, and uidLength will indicate the size of the UID (normally 7)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  // It seems we found a valid ISO14443A Tag!
  if (success)
  {
    // 2.) Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");

    if (uidLength != 7)
    {
      Serial.println("This doesn't seem to be an NTAG203 tag (UUID length != 7 bytes)!");
    }
    else
    {
      uint8_t data[32];

      // We probably have an NTAG2xx card (though it could be Ultralight as well)
      Serial.println("Seems to be an NTAG2xx tag (7 byte UID)");

      // NTAG2x3 cards have 39*4 bytes of user pages (156 user bytes),
      // starting at page 4 ... larger cards just add pages to the end of
      // this range:

      // See: http://www.nxp.com/documents/short_data_sheet/NTAG203_SDS.pdf

      // TAG Type       PAGES   USER START    USER STOP
      // --------       -----   ----------    ---------
      // NTAG 203       42      4             39
      // NTAG 213       45      4             39
      // NTAG 215       135     4             129
      // NTAG 216       231     4             225


      // 3.) Check if the NDEF Capability Container (CC) bits are already set
      // in OTP memory (page 3)
      memset(data, 0, 4);
      success = nfc.ntag2xx_ReadPage(3, data);
      if (!success)
      {
        Serial.println("Unable to read the Capability Container (page 3)");
        state_entry_waiting_for_button();
        return;
      }
      else
      {
        // If the tag has already been formatted as NDEF, byte 0 should be:
        // Byte 0 = Magic Number (0xE1)
        // Byte 1 = NDEF Version (Should be 0x10)
        // Byte 2 = Data Area Size (value * 8 bytes)
        // Byte 3 = Read/Write Access (0x00 for full read and write)
        if (!((data[0] == 0xE1) && (data[1] == 0x10)))
        {
          Serial.println("This doesn't seem to be an NDEF formatted tag.");
          Serial.println("Page 3 should start with 0xE1 0x10.");
        }
        else
        {
          // 4.) Determine and display the data area size
          dataLength = data[2]*8;
          Serial.print("Tag is NDEF formatted. Data area size = ");
          Serial.print(dataLength);
          Serial.println(" bytes");

          // 5.) Erase the old data area
          Serial.print("Erasing previous data area ");
          for (uint8_t i = 4; i < (dataLength/4)+4; i++)
          {
            memset(data, 0, 4);
            success = nfc.ntag2xx_WritePage(i, data);
            Serial.print(".");
            if (!success)
            {
              Serial.println(" ERROR!");
              state_entry_waiting_for_button();
              return;
            }
          }
          Serial.println(" DONE!");

          // 6.) Try to add a new NDEF URI record
          Serial.print("Writing URI as NDEF Record ... ");
          success = nfc.ntag2xx_WriteNDEFURI(ndefprefix, url, dataLength);
          if (success)
          {
            Serial.println("DONE!");
          }
          else
          {
            Serial.println("ERROR! (URI length?)");
          }

        } // CC contents NDEF record check
      } // CC page read check
    } // UUID length check
    state_entry_waiting_for_button();
  } // Start waiting for a new ISO14443A tag
  else {
    Serial.println("Failed to readPassiveTargetID");
  }
}
