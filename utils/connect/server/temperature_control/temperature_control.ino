#define T_SENS_PIN 4
#define FAN_1_PIN 3
#define FAN_2_PIN 5

#define FAN_MIN_PWM 0
#define FAN_MAX_PWM 255
void setup() {
    Serial.begin(9600);

    pinMode(FAN_1_PIN, OUTPUT);
    pinMode(FAN_2_PIN, OUTPUT);
}

void loop() {
    for(int i=0; i < 255; i++) {
        analogWrite(FAN_1_PIN, i);
        analogWrite(FAN_2_PIN, i);
        Serial.println(i);
        delay(500);
    }
}