#include "validation.h"
#include <limits.h>
#include <string.h>

int valid_name(const char *name)
{
    size_t i; /* 名称中的字节位置 */
    if (!name || !*name || strlen(name) >= 64)
        return 0;
    for (i = 0; name[i]; ++i)
        if ((unsigned char)name[i] <= 32 || name[i] == '|')
            return 0;
    return 1;
}

int parse_money(const char *text, int *cents)
{
    long value = 0;                /* 累计的整数分值 */
    int digits = 0, fraction = -1; /* 总数字数，小数位数；-1表示未遇小数点 */
    const char *p;                 /* 当前字符 */
    for (p = text; *p; ++p)
    {
        if (*p == '.' && fraction == -1 && digits)
        {
            fraction = 0;
            continue;
        }
        if (*p < '0' || *p > '9' || fraction == 2)
            return 0;
        value = value * 10 + (*p - '0');
        if (value > INT_MAX / 100)
            return 0;
        ++digits;
        if (fraction >= 0)
            ++fraction;
    }
    if (!digits || fraction == 0)
        return 0;
    if (fraction == -1)
        value *= 100;
    else if (fraction == 1)
        value *= 10;
    if (value <= 0 || value > INT_MAX)
        return 0;
    *cents = (int)value;
    return 1;
}
