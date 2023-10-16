#include <stdint.h>
#include <Wire.h>

class DPS {
    public:

    static const int dpsValues_len = 9;    // ughhh
    typedef union {
        struct {
            float inV;
            float inA;
            float inW;
            float outV;
            float outA;
            float outW;
            float intakeTemp;
            float internTemp;
            float fanRpm;
        };
        float raw[dpsValues_len];
    } dpsValues_t;
    // static const int dpsValues_len = sizeof(dpsValues_t) / sizeof(float);

    const uint8_t registersForValues[dpsValues_len] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0D, 0x0E, 0x0F};

    typedef struct {
        const char *name;
        float factor;
    } dpsRegisterEntry_t;

    // actual registers are at address * 2
    const dpsRegisterEntry_t dpsRegisters[16] = {
        {"", 0},                    // 0x00
        {"flags", 0},               // 0x01
        {"", 0},                    // 0x02
        {"", 0},                    // 0x03
        {"in_volts", 32.0},         // 0x04
        {"in_amps", 128.0},         // 0x05
        {"in_watts", 2.0},          // 0x06
        {"out_volts", 256.0},       // 0x07
        {"out_amps", 128.0},        // 0x08
        {"out_watts", 2.0},         // 0x09
        {"", 0},                    // 0x0A
        {"", 0},                    // 0x0B
        {"", 0},                    // 0x0C
        {"intake_temp", 128.0},     // 0x0D
        {"internal_temp", 128.0},   // 0x0E
        {"fan_rpm", 1},             // 0x0F
    };
    static const int dpsRegisters_len = sizeof(dpsRegisters) / sizeof(dpsRegisters[0]);

    enum class WriteRegs : uint8_t {
        FAN_RPM = 0x40,
    };

    uint8_t _calcChecksum(uint8_t *data, size_t size) {
        uint8_t checksum = 0;
        checksum -= _addr << 1;
        for (int i = 0; i < size; i++) {
            checksum -= data[i];
        }
        return checksum;
    }


    uint16_t _read(uint8_t reg) {
        // uint8_t checksum = (0xFF - ((_addr << 1) + reg) + 1) & 0xFF;
        Wire.beginTransmission(_addr);
        Wire.write(reg);
        Wire.write(_calcChecksum(&reg, 1));
        Wire.endTransmission();
        // printf("> %02X %02X %02X\n", _addr, reg, checksum);

        delayMicroseconds(1000);

        Wire.requestFrom(_addr, 3);
        uint8_t msg[3]; // LSB, MSB, CS
        Wire.readBytes(msg, sizeof(msg));
        // printf("< %02X %02X %02X\n", msg[0], msg[1], msg[2]);
        return (msg[1] << 8) | msg[0];
    }


    void _write(uint8_t reg, uint16_t val) {
        uint8_t msg[] = {reg, val & 0xFF, val >> 8};

        Wire.beginTransmission(_addr);
        Wire.write(msg, sizeof(msg));
        Wire.write(_calcChecksum(msg, sizeof(msg)));
        Wire.endTransmission();
    }



    dpsValues_t getValues() {
        dpsValues_t vals = {0};
        // float vals[dpsValues_len] = {0};

        for (int i = 0; i < sizeof(registersForValues); i++) {
            uint8_t reg = registersForValues[i];

            if (reg < dpsRegisters_len) {
                uint16_t val = _read(reg * 2);
                float factor = dpsRegisters[reg].factor;
                if (factor != 0) {
                    vals.raw[i] = (float)val / factor;
                }
            }

        }
        return vals;
    }


    void setFanSpeed(uint16_t rpm) {
        printf("Setting RPM to %5d\n", rpm);
        _write((uint8_t)WriteRegs::FAN_RPM, rpm);
        
    }


    DPS(uint8_t i2c_addr) : _addr(i2c_addr) {};

    private:
        uint8_t _addr;

};