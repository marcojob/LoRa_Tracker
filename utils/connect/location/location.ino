#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

#include <HardwareSerial.h>
#include "src/TinyGPS++.h"

#define GPS_LAT_LON_ACC 5
#define PAYLOAD_SIZE 7
#define R_DIV 2.0
#define ADC_MAX 4095.0
#define V_REF 2.2
#define V_BAT_MAX 4.2
#define V_BAT_MIN 2.7
#define V_SAMPLES 100
#define SOC_MAX 16
#define BAT_SENSE_PIN A6
#define BAT_A 0.00079866 // LSQ Fitted analog read line
#define BAT_B 0.08189 // LSQ Fitted analog read line

TinyGPSPlus gps;
HardwareSerial serial_1(1);

static const u1_t PROGMEM APPEUI[8] = { 0x21, 0x62, 0x02, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

static const u1_t PROGMEM DEVEUI[8] = { 0xB8, 0xC1, 0x17, 0xBE, 0x34, 0x76, 0x55, 0x00 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

static const u1_t PROGMEM APPKEY[16] = { 0x3D, 0x1D, 0x2B, 0x2E, 0xC8, 0x09, 0x8C, 0x2C, 0xC6, 0x13, 0x82, 0x1D, 0x49, 0x50, 0x39, 0xC6 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


u1_t NWKSKEY[16]; // LoRaWAN NwkSKey, network session key.
u1_t APPSKEY[16]; // LoRaWAN AppSKey, application session key.
u4_t DEVADDR; // LoRaWAN end-device address (DevAddr)


uint8_t data[PAYLOAD_SIZE];

static osjob_t initjob;
static osjob_t sendjob;

const unsigned int TX_INTERVAL = 60; // 120

const lmic_pinmap lmic_pins = {
    .nss = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 14,
    .dio = {26, 33, 32},
};

bool join_with_otaa = true;

void blink(unsigned int del) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(del);
    digitalWrite(LED_BUILTIN, LOW);
    delay(del);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(del);
    digitalWrite(LED_BUILTIN, LOW);
}

void query_data() {
    while (serial_1.available() > 0) {
        if (gps.encode(serial_1.read())) {
            if (gps.location.isValid()) {
                Serial.println("INFO: Found valid GPS location");
                set_data(data, GPS_LAT_LON_ACC);
                break;
            }
        }
    }
}

uint8_t get_soc() {
    /* Measured by 12-bit ADC (4095 = 3.3V),
     * voltage halfed over divider. SOC 1 = 2.1V.
     * We have 4 bits to represent 1.5V (4.2V - 2.7V).
     */
    double v_bat = get_v_bat();
    if (v_bat > V_BAT_MAX) {
        v_bat = V_BAT_MAX;
    } else if (v_bat < V_BAT_MIN) {
        v_bat = V_BAT_MIN;
    }
    uint8_t soc = 0b00001111 & uint8_t((v_bat - V_BAT_MIN)*SOC_MAX/(V_BAT_MAX - V_BAT_MIN));
    return soc;
}

double get_v_bat() {
    double v_analog = 0;
    for (int i = 0; i < V_SAMPLES; i++) {
        v_analog += analogRead(BAT_SENSE_PIN);
        delay(1);
    }
    return R_DIV*(v_analog/(double)V_SAMPLES*BAT_A + BAT_B);
}

void set_data(uint8_t *data, uint8_t acc) {
    /* data[0] = <sign lat><7 lat data bytes>
     * data[1] = <8 lat data bytes>
     * data[2] = <8 lat data bytes>
     * data[3] = <2 lat data bytes><sign lon><5 lon data bytes>
     * data[4] = <8 lat data bytes>
     * data[5] = <8 lat data bytes>
     * data[6] = <4 lat data bytes><4 state of charge>
     */
    for (size_t d = 0; d < PAYLOAD_SIZE; d++) {
        //reset data
        data[d] = 0x00;
    }
    // Get gps data
    uint32_t lat = (uint32_t)(gps.location.lat()*pow(10,acc)); // -90 to 90
    uint32_t lon = (uint32_t)(gps.location.lng()*pow(10,acc)); // -180 to 180
    uint8_t soc = get_soc();

    // Set sign of lat, 1 negative
    if (gps.location.lat() < 0) {
        data[0] = 0b1000000;
    }
    // Set sign of lon, 1 negative
    if (gps.location.lng() < 0) {
        data[3] = 0b0010000;
    }


    data[3] = 0b11000000 & (lat << 6);
    lat = lat >> 2;

    data[2] = lat;
    lat = lat >> 8;

    data[1] = lat;
    lat = lat >> 8;

    data[0] = 0b01111111 & lat;

    data[6] = 0b11110000 & (lon << 4);
    lon = lon >> 4;

    data[6] = 0b00001111 & soc;

    data[5] = lon;
    lon = lon >> 8;

    data[4] = lon;
    lon = lon >> 8;

    data[3] = 0b00011111 & lon;
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
            blink(1000);
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));

            // Disable link check validation (automatically enabled
            // during join, but not supported by TTN at this time).
            LMIC_setLinkCheckMode(0);

            join_with_otaa = false;
            // when joined, save NWKSKEY, APPSKEY, DEVADDR
            DEVADDR = LMIC.devaddr;
            memcpy(NWKSKEY, LMIC.nwkKey, 16);
            memcpy(APPSKEY, LMIC.artKey, 16);

            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            blink(500);
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            digitalWrite(LED_BUILTIN, LOW);
            if (LMIC.txrxFlags & TXRX_ACK)
                Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
                Serial.println(F("Received "));
                Serial.println(LMIC.dataLen);
                Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            blink(250);
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
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void do_send(osjob_t* j) {
    Serial.println("INFO: Starting next job");
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        query_data();
        Serial.println("INFO: Finished query_data");
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, data, PAYLOAD_SIZE, 0);
        Serial.println("INFO: Set TX data");
        digitalWrite(LED_BUILTIN, HIGH);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

static void initfunc (osjob_t* j) {
    // reset MAC state
    LMIC_reset();
    LMIC_setLinkCheckMode(0);
    LMIC_setDrTxpow(DR_SF7, 14);
    LMIC.dn2Dr = DR_SF9;
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);

    if (join_with_otaa) {
      // start joining
      Serial.println ("INFO: Start joining with OTAA");
      LMIC_startJoining();
    } else {
        data[0] = 0x21;
        Serial.println ("INFO: Start joining with existing keys");
        Serial.print("INFO: dev addr: ");
        Serial.println(DEVADDR, HEX);
        LMIC_setSession(0x1, DEVADDR, NWKSKEY, APPSKEY);
        do_send(&sendjob);
    }
    Serial.println ("INFO: initfunc finished");
}

void setup() {
    Serial.begin(115200);
    Serial.println("INFO: Starting");

    analogSetAttenuation(ADC_11db);

    // Set pins
    pinMode(LED_BUILTIN, OUTPUT);

    // GPS
    serial_1.begin(9600, SERIAL_8N1, 16, 17);

    // Setup channels
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

    // LMIC init
    os_init();
    os_setCallback(&initjob, initfunc);
}

void loop() {
    os_runloop_once();
}
