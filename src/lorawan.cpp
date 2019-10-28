#include "lorawan.h"
#include "settings.h"
#include <hal/hal.h>
#include <SPI.h>
#include "io_pins.h"
#include "powerdown.h"
#include "functions.h"
#include "global.h"

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = PIN_LMIC_NSS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = PIN_LMIC_RST,
    .dio = {PIN_LMIC_DIO0, PIN_LMIC_DIO1, PIN_LMIC_DIO2 },
};

static uint8_t LORA_DATA[2];

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = LORA_TX_INTERVAL;

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

bool GO_DEEP_SLEEP = false;


void LoRaWANSetup()
{
    Serial.println(F("LoRaWAN_Setup ..."));
    delay(100);
    
    // LMIC init
    os_init();

    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
        // On AVR, these values are stored in flash and only copied to RAM
        // once. Copy them to a temporary buffer here, LMIC_setSession will
        // copy them into a buffer of its own again.
        uint8_t appskey[sizeof(APPSKEY)];
        uint8_t nwkskey[sizeof(NWKSKEY)];
        memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
        memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
        LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
        // If not running an AVR with PROGMEM, just use the arrays direc
        LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
        // Set up the channels used by the Things Network, which corresponds
        // to the defaults of most gateways. Without this, only three base
        // channels from the LoRaWAN specification are used, which certainly
        // works, so it is good for debugging, but can overload those
        // frequencies, so be sure to configure the full frequency range of
        // your network here (unless your network autoconfigures them).
        // Setting up channels should happen after LMIC_setSession, as that
        // configures the minimal channel set.
        LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band 
        LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
        LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
        LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
        // TTN defines an additional channel at 869.525Mhz using SF9 for class B
        // devices' ping slots. LMIC does not have an easy way to define set this
        // frequency and support for class B is spotty and untested, so this
        // frequency is not configured here.
    #elif defined(CFG_us915)
        // NA-US channels 0-71 are configured automatically
        // but only one group of 8 should (a subband) should be active
        // TTN recommends the second sub band, 1 in a zero based count.
        // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
        LMIC_selectSubBand(1);
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(LMIC_LORA_SF,14);

    // Start job
    LoraWANDo_send(&sendjob);
}


void LoraWANDo_send(osjob_t* j)
{
    LoraWANGetData();
    
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, LORA_DATA, sizeof(LORA_DATA), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        /* This event is defined but not used in the code
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE"));

            if (LMIC.txrxFlags & TXRX_ACK)
            {
                Serial.println(F("Received ack"));
            }
          
            if (LMIC.dataLen) 
            {
                Serial.print(LMIC.dataLen);
                Serial.println(F(" bytes of payload"));
            }

            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), LoraWANDo_send);
            GO_DEEP_SLEEP = true;
           
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /* This event is defined but not used in the code.
        case EV_SCAN_FOUND:
            DisplayPrintln(F("EV_SCAN_FOUND"), LORAWAN_STATE_DISPLAY_LINE);
            break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
         default:
            Serial.println(F("Unknown event"));
            Serial.println((unsigned) ev);
            break;
    }
}

void LoraWANDo(void)
{
    if(GO_DEEP_SLEEP == true)
    {
      PowerDownTXIntervall();
      GO_DEEP_SLEEP = false;
    }
    else
    {
      os_runloop_once();
    }
}

void LoraWANGetData()
{
    int32_t vcc = ( ReadVcc() / 10) - 200;
    
    float humidity_org = DHTSENSOR.readHumidity();
    int32_t humidity = humidity_org;
    
    float temperature_org = DHTSENSOR.readTemperature();
    int32_t temp = (temperature_org * 10);
     
    if ( isnan(humidity_org) || isnan(temperature_org) )
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }

    if ( isnan(humidity_org) )
    { 
      LORA_DATA[2] = 255;    
      LORA_DATA[2] = 255;
    }
    else 
    { 
      LORA_DATA[2] = temp >> 8;
      LORA_DATA[3] = temp & 0xFF;
    }

    if ( isnan(humidity_org)) { LORA_DATA[1] = 255; }
    else { LORA_DATA[1] = humidity; }
    
    LORA_DATA[0] = vcc;
    
    Serial.print(F("VCC: "));
    Serial.println(LORA_DATA[0]);
    
    Serial.print(F("Humidity: "));
    Serial.println(LORA_DATA[1]);
    
    Serial.print(F("Temperature: "));
    Serial.print(LORA_DATA[2]);
    Serial.print(F(" "));
    Serial.println(LORA_DATA[3]);
}