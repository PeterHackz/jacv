#include "jutils/jutils.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// https://stackoverflow.com/a/39052987/17370961
uint64_t hex2int(const char *hex)
{
    uint64_t val = 0;
    while (*hex)
    {
        // get current character then increment
        uint8_t byte = *hex++;
        // transform hex character to the 4bit equivalent number, using the
        // ascii table indexes
        if (byte >= '0' && byte <= '9')
            byte = byte - '0';
        else if (byte >= 'a' && byte <= 'f')
            byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <= 'F')
            byte = byte - 'A' + 10;
        // shift 4 to make space for new digit, and add the 4 bits of the new
        // digit
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

uint64_t oc2int(const char *str)
{
    uint64_t ans = 0;
    for (int i = 0; str[i]; i++)
        ans = ans * 8 + str[i] - '0';
    return ans;
}

uint64_t bin2int(const char *str)
{
    uint64_t result = 0; // 64 zero bits
    // set bits backwards in the result
    int len = strlen(str);
    for (int i = 0; str[i]; i++)
    {
        if (str[i] == '1')
            result |= (1 << (len - i - 1)); // set the bit to 1
    }
    return result;
}

const char HEX_CHARS[] = "1234567890abcdefABCDEF";

bool isHexChar(char c)
{
    for (int i = 0; HEX_CHARS[i]; i++)
        if (c == HEX_CHARS[i])
            return true;
    return false;
}

ParseNumberResult parse_number(const char *str)
{
    // TODO: implement bases 16, 2 and 8
    int base = 10;
    bool hasFloat = false;
    ParseNumberResult result = {};

    char firstChar = str[0];

    if (str[1] == 0)
    {
        result.num.uint64 = firstChar - '0';
        result.state = STATE_SUCCESS_NUM;
        return result;
    }

    if (firstChar == '0')
    {
        char b = str[1];
        if (b == 'x' || b == 'X')
        {
            base = 16;
            str += 2;
        }
        else if (b == 'b' || b == 'B')
        {
            base = 2;
            str += 2;
        }
        else if (b != 'o' && b != 'O' && !(b >= '0' && b <= '9'))
            goto fail;
        else
        {
            base = 8;
            str += ((b == 'o' || b == 'O') ? 2 : 1);
        }
    }
    else if (firstChar == '.')
    {
        str++;
    }

    int i;

    if (base == 10)
    {
        int dotsCount = 0;
        for (i = 0; str[i]; i++)
            if (!(str[i] >= '0' && str[i] <= '9'))
            {
                if (str[i] != '.')
                {
                    goto fail;
                }
                hasFloat = true;
                dotsCount++;
            }

        if (dotsCount > 1)
        {
            goto fail;
        }

        uint64_t num = 0;
        result.state = STATE_SUCCESS_FLOAT;
        for (i = 0; str[i]; i++)
        {
            if (str[i] != '.')
            {
                num *= 10;
                num += str[i] - '0';
            }
        }

        if (!hasFloat)
        {
            result.state = STATE_SUCCESS_NUM;
            result.num.uint64 = num;
        }
        else
        {
            double div = 1.0;
            bool passedDot = false;
            for (i = 0; str[i]; i++)
                if (str[i] == '.')
                    passedDot = true;
                else if (passedDot)
                {
                    div *= 10;
                }
            result.num.float64 = num / div;
        }

        if (firstChar == '.')
        {
            if (result.state == STATE_SUCCESS_FLOAT)
            {
                // another '.' was found after the first one
                goto fail;
            }
            result.state = STATE_SUCCESS_FLOAT;

            double div = 1.0;
            for (i = 0; str[i]; i++)
                div *= 10;

            result.num.float64 = result.num.uint64 / (double)div;
        }
    }
    else if (base == 16)
    {
        for (i = 0; str[i]; i++)
            if (!isHexChar(str[i]))
                goto fail;
        result.num.uint64 = hex2int(str);
        result.state = STATE_SUCCESS_NUM;
    }
    else if (base == 2)
    {
        for (i = 0; str[i]; i++)
            if (str[i] != '1' && str[i] != '0')
                goto fail;

        result.num.uint64 = bin2int(str);
        result.state = STATE_SUCCESS_NUM;
    }
    else if (base == 8)
    {
        for (i = 0; str[i]; i++)
            if (!(str[i] >= '0' && str[i] <= '7'))
                goto fail;

        result.num.uint64 = oc2int(str);
        result.state = STATE_SUCCESS_NUM;
    }

    return result;

fail:
    result.state = STATE_FAIL;
    return result;
}