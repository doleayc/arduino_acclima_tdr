#include "AcclimaTDR.h"

#define debugPrintLn(...)                             \
    {                                                 \
        if (this->_debugSerial)                       \
            this->_debugSerial->println(__VA_ARGS__); \
    }
#define debugPrint(...)                             \
    {                                               \
        if (this->_debugSerial)                     \
            this->_debugSerial->print(__VA_ARGS__); \
    }

#define FRAME_TERMINATOR "\r\n"
#define FRAME_TERMINATOR_LENGTH (sizeof(FRAME_TERMINATOR) - 1) // Length of CRLF without '\0' terminator

static const char *CMD_ASK_ADDRESS = "?";
static const char *CMD_INFO = "I";
static const char *CMD_READ_START = "M";
static const char *CMD_READ_VALUES = "D0";

AcclimaTDR::AcclimaTDR(SDI12 &sdiInstance) : _sdi(sdiInstance)
{
}

AcclimaTDR::AcclimaTDR(SDI12 &sdiInstance, char address) : _sdi(sdiInstance), _address(address)
{
}

void AcclimaTDR::setCustomDelay(void (*f)(unsigned long))
{
    _delay = *f;
}

void AcclimaTDR::setDebugSerial(Stream &stream)
{
    _debugSerial = &stream;
}

char AcclimaTDR::getSavedAddress() const
{
    return _address;
}

char AcclimaTDR::findAddress()
{

    _sdi.begin();
    _delay(500);

    _sendCommand(CMD_ASK_ADDRESS);

    _readLine();

    if ((_frameBuffer[0] >= '0' && _frameBuffer[0] <= '9') ||
        (_frameBuffer[0] >= 'A' && _frameBuffer[0] <= 'Z') ||
        (_frameBuffer[0] >= 'a' && _frameBuffer[0] <= 'z'))
    {
        _address = _frameBuffer[0];
        return _address;
    }
    else
    {
        return '?';
    }

    _sdi.end();
}

void AcclimaTDR::getInfo(char *buffer, uint8_t size)
{
    _sdi.begin();
    _delay(500);

    _sendCommand(CMD_INFO);

    _readLine();

    if (buffer && size > 1)
    {
        strncpy(buffer, _frameBuffer, strlen(_frameBuffer) < size - 1 ? strlen(_frameBuffer) : size - 1);
    }

    _sdi.end();
}

int AcclimaTDR::readValues(float *vol_water, float *temperature, float *permitivity, uint16_t *electrical_cond, uint16_t *pore_water)
{
    _sdi.begin();
    _delay(250);

    _sendCommand(CMD_READ_START);

    /**
     * Receive aTTTn response where:
     *  a = address
     *  TTT = number of seconds that reads will be available
     *  n = number of values (sensors)
     * */
    _readLine();

    if (_frameBuffer[0] != _address)
    {
        return -1;
    }

    uint16_t wait_seconds = _frameBuffer[1] * 100 + _frameBuffer[2] * 10 + _frameBuffer[3];
    int values_read = _frameBuffer[4];

    // wait TTT seconds
    _readLine(wait_seconds * 1000);

    _sendCommand(CMD_READ_VALUES);
    _readLine();

    if (strlen(_frameBuffer) < 1)
    {
        return -2;
    }

    char *ptr = &_frameBuffer[1];
    float aux_f;

    aux_f = strtod(ptr, &ptr);

    if (vol_water != nullptr)
    {
        *vol_water = aux_f;
    }

    aux_f = strtod(ptr, &ptr);

    if (temperature != nullptr)
    {
        *temperature = aux_f;
    }

    aux_f = strtod(ptr, &ptr);

    if (permitivity != nullptr)
    {
        *permitivity = aux_f;
    }

    unsigned long aux_ul;

    aux_ul = strtoul(ptr, &ptr, 10);

    if (electrical_cond != nullptr)
    {
        *electrical_cond = aux_ul;
    }

    aux_ul = strtoul(ptr, &ptr, 10);

    if (pore_water != nullptr)
    {
        *pore_water = aux_ul;
    }

    return values_read;
}

/** 
 * There are two different versions of probes.
 *  - Older version sends 3 values for every read. When one of the three expected sensor is not found, value is sent as 00.0000.
 *      Format four sensors probe: X+XXX.XXXX+000.000+000.0000\r\n
 *  - New version sends ONLY values from sensors that are presents.
 *      Format four sensors probe: X+XXX.XXXX\r\n 
 **/
void AcclimaTDR::_parseValues(float *values, byte startIndex)
{
    if (strlen(_frameBuffer) < 1)
    {
        values[startIndex] = -1.0;
        values[startIndex + 1] = -1.0;
        values[startIndex + 2] = -1.0;
    }
    else if (_frameBuffer[0] != _address)
    {
        values[startIndex] = -2.0;
        values[startIndex + 1] = -2.0;
        values[startIndex + 2] = -2.0;
    }
    else
    {

        char *ptr = &_frameBuffer[1];
        size_t i = 0;
        while (i < 3 && ptr != nullptr)
        {
            values[startIndex + i] = strtod(ptr, &ptr);
            ++i;
        }

        while (i < 3)
        {
            values[startIndex + i] = 0.0;
            ++i;
        }
    }
}

void AcclimaTDR::_sendCommand(const char *command)
{
    char cmd[5] = "";

    if (strncmp(command, CMD_ASK_ADDRESS, strlen(CMD_ASK_ADDRESS)) != 0)
    {
        cmd[0] = _address;
    }

    strncat(cmd, command, strlen(command));
    strcat(cmd, "!");

    debugPrintLn(cmd);

    _sdi.sendCommand(cmd);
    _sdi.flush();
}

int AcclimaTDR::_readByte(uint32_t timeout)
{
    int c;
    uint32_t _startMillis = millis();

    do
    {
        c = _sdi.read();
        if (c >= 0)
        {
            return c;
        }

    } while (millis() - _startMillis < timeout);

    return -1; //-1 indicates timeout;
}

size_t AcclimaTDR::_readBytesUntil(char terminator, char *buffer, size_t length, uint32_t timeout)
{
    if (length < 1)
    {
        return 0;
    }

    size_t index = 0;

    while (index < length)
    {
        int c = _readByte(timeout);
        if (c < 0 || c == terminator)
        {
            break;
        }

        // Skip null characters. Why sometimes sensor sent it?
        if (c > 0)
        {
            buffer[index] = static_cast<char>(c);
            ++index;
        }
    }

    if (index < length)
    {
        buffer[index] = '\0';
    }

    return index;
}

size_t AcclimaTDR::_readLine(char *buffer, size_t size, uint32_t timeout)
{
    size_t len = _readBytesUntil(FRAME_TERMINATOR[FRAME_TERMINATOR_LENGTH - 1], buffer, size - 1, timeout);

    if ((FRAME_TERMINATOR_LENGTH > 1) && (buffer[len - (FRAME_TERMINATOR_LENGTH - 1)] == FRAME_TERMINATOR[0]))
    {
        len -= FRAME_TERMINATOR_LENGTH - 1;
    }

    buffer[len] = '\0';
    // Without this brief delay 485 probes aren't read well with 485 adapters... Why??
    _delay(50);
    return len;
}

size_t AcclimaTDR::_readLine()
{
    return _readLine(_frameBuffer, FRAME_BUFFER_SIZE);
}

size_t AcclimaTDR::_readLine(uint32_t timeout)
{
    return _readLine(_frameBuffer, FRAME_BUFFER_SIZE, timeout);
}