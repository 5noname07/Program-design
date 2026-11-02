#include "validation.h"
#include <limits.h>
#include <string.h>
#include <time.h>

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
    int value = 0;                 /* 累计数字；每次计算前检查溢出 */
    int digits = 0, fraction = -1; /* 总数字数，小数位数；-1表示未遇小数点 */
    const char *p;                 /* 当前字符 */
    if (!text || !cents)
        return 0;
    for (p = text; *p; ++p)
    {
        if (*p == '.' && fraction == -1 && digits)
        {
            fraction = 0;
            continue;
        }
        if (*p < '0' || *p > '9' || fraction == 2)
            return 0;
        if (value > (INT_MAX - (*p - '0')) / 10)
            return 0;
        value = value * 10 + (*p - '0');
        ++digits;
        if (fraction >= 0)
            ++fraction;
    }
    if (!digits || fraction == 0)
        return 0;
    if (fraction == -1)
    {
        if (value > INT_MAX / 100)
            return 0;
        value *= 100;
    }
    else if (fraction == 1)
    {
        if (value > INT_MAX / 10)
            return 0;
        value *= 10;
    }
    if (value <= 0 || value > INT_MAX)
        return 0;
    *cents = (int)value;
    return 1;
}

int parse_integer(const char *text, int *value)
{
    int result = 0; /* 尚未写入调用者输出的累积值 */
    const char *p;  /* 当前数字 */
    if (!text || !*text || !value)
        return 0;
    for (p = text; *p; ++p)
    {
        if (*p < '0' || *p > '9' || result > (INT_MAX - (*p - '0')) / 10)
            return 0;
        result = result * 10 + (*p - '0');
    }
    *value = result;
    return 1;
}

int valid_timestamp(const char *text)
{
    int i, year, month, day, hour, minute, second; /* 日期字段及字节下标 */
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; /* 平年天数 */
    int max_day;                                                                /* 当月天数 */
    if (!text || strlen(text) != 19)
        return 0;
    for (i = 0; i < 19; ++i)
    {
        if (i == 4 || i == 7)
        {
            if (text[i] != '-')
                return 0;
        }
        else if (i == 10)
        {
            if (text[i] != 'T')
                return 0;
        }
        else if (i == 13 || i == 16)
        {
            if (text[i] != ':')
                return 0;
        }
        else if (text[i] < '0' || text[i] > '9')
            return 0;
    }
    year =
        (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + text[3] - '0';
    month = (text[5] - '0') * 10 + text[6] - '0';
    day = (text[8] - '0') * 10 + text[9] - '0';
    hour = (text[11] - '0') * 10 + text[12] - '0';
    minute = (text[14] - '0') * 10 + text[15] - '0';
    second = (text[17] - '0') * 10 + text[18] - '0';
    if (year < 1 || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59)
        return 0;
    max_day = days[month - 1];
    if (month == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
        ++max_day;
    return day >= 1 && day <= max_day;
}
