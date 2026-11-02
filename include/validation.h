#ifndef CANTEEN_VALIDATION_H
#define CANTEEN_VALIDATION_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* 输入校验模块，不依赖业务结构体。
     * name为以零结尾的字符串；返回1合法、0非法。
     * 名称必须非空、短于64字节，且不含空白和分隔符。 */
    int valid_name(const char *name);
    /* text为元单位金额，只接受正数、最多两位小数；cents指向输出分值。
     * 返回1成功；0表示格式错误或超限，此时不修改输出值。 */
    int parse_money(const char *text, int *cents);
    /* 非负十进制整数，拒绝符号、空白和超出int范围的输入；失败不修改value。 */
    int parse_integer(const char *text, int *value);
    /* 检查YYYY-MM-DDTHH:MM:SS格式及真实日历日期，包括闰年。 */
    int valid_timestamp(const char *text);

#ifdef __cplusplus
}
#endif
#endif
