#pragma once

#include <stdint.h>
#include "SDI12.h"


#define FRAME_BUFFER_SIZE 64


class AcclimaTDR
{
public:
    AcclimaTDR(SDI12 &sdiInstance);
    AcclimaTDR(SDI12 &sdiInstance, char address);
    ~AcclimaTDR(){};

    virtual void init() {}
    char getSavedAddress() const;
    void getInfo(char *buffer, uint8_t size);
    char findAddress();
    int readValues(float *vol_water, float *temperature, float *permitivity, uint16_t *electrical_cond, uint16_t *pore_water);
    void setCustomDelay(void (*f)(unsigned long));
    void setDebugSerial(Stream &stream);

private:
    SDI12 &_sdi;
    char _address;

    char _frameBuffer[FRAME_BUFFER_SIZE];
    Stream *_debugSerial = nullptr;
    void (*_delay)(unsigned long) = delay;

    virtual void _sendCommand(const char *command);
    int _readByte(uint32_t timeout);
    size_t _readBytesUntil(char terminator, char *buffer, size_t length, uint32_t timeout);
    size_t _readLine(char *buffer, size_t length, uint32_t timeout = 2000);

    size_t _readLine();
    size_t _readLine(uint32_t timeout);
    void _parseValues(float *values, uint8_t startIndex);
};