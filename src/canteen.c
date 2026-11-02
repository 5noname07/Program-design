#include "canteen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* 名称不允许空白或分隔符，保证文本格式可无歧义读取。UTF-8按字节计长。 */
static int valid_name(const char *name)
{
    size_t i; /* 名称中的字节位置 */
    if (!name || !*name || strlen(name) >= 64)
        return 0;
    for (i = 0; name[i]; ++i)
        if ((unsigned char)name[i] <= 32 || name[i] == '|')
            return 0;
    return 1;
}

int card_index(const Canteen *s, int id)
{
    int i; /* 顺序查找的下标 */
    for (i = 0; i < s->card_count; ++i)
        if (s->cards[i].id == id)
            return i;
    return -1;
}
int dish_index(const Canteen *s, int id)
{
    int i; /* 包括停用菜品，保证历史编号不被重新使用 */
    for (i = 0; i < s->dish_count; ++i)
        if (s->dishes[i].id == id)
            return i;
    return -1;
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
const char *register_card(Canteen *s, int id, const char *name)
{
    Card *c; /* 待初始化的新卡 */
    if (id <= 0 || !valid_name(name))
        return "卡号须为正整数，姓名须为1至63字节且不含空白";
    if (card_index(s, id) >= 0)
        return "卡号重复";
    if (s->card_count == MAX_CARDS)
        return "学生卡容量已满";
    c = &s->cards[s->card_count++];
    c->id = id;
    strcpy(c->name, name);
    c->balance = 0;
    c->lost = 0;
    return NULL;
}
const char *recharge(Canteen *s, int id, int cents)
{
    int i = card_index(s, id); /* 目标卡下标 */
    if (i < 0)
        return "卡号不存在";
    if (cents <= 0 || s->cards[i].balance > INT_MAX - cents)
        return "充值金额无效或余额超限";
    s->cards[i].balance += cents;
    return NULL;
}
const char *set_lost(Canteen *s, int id, int lost)
{
    int i = card_index(s, id); /* 目标卡下标 */
    if (i < 0)
        return "卡号不存在";
    if (lost != 0 && lost != 1)
        return "状态只能为0正常或1挂失";
    s->cards[i].lost = lost;
    return NULL;
}
const char *put_dish(Canteen *s, int id, const char *name, int cents)
{
    int i = dish_index(s, id); /* 已有编号则修改，否则新增 */
    Dish *d;                   /* 当前菜品 */
    if (id <= 0 || !valid_name(name) || cents <= 0)
        return "编号、名称或价格无效";
    if (i >= 0 && !s->dishes[i].active)
        return "该编号已停用，请使用新编号";
    if (i < 0)
    {
        if (s->dish_count == MAX_DISHES)
            return "菜品容量已满";
        i = s->dish_count++;
        s->dishes[i].sales = 0;
    }
    d = &s->dishes[i];
    d->id = id;
    strcpy(d->name, name);
    d->price = cents;
    d->active = 1;
    return NULL;
}
const char *delete_dish(Canteen *s, int id)
{
    int i = dish_index(s, id); /* 使用软删除保护历史记录 */
    if (i < 0 || !s->dishes[i].active)
        return "菜品不存在或已经删除";
    s->dishes[i].active = 0;
    return NULL;
}
const char *purchase(Canteen *s, int card_id, int dish_id)
{
    int c = card_index(s, card_id), d = dish_index(s, dish_id); /* 两个对象下标 */
    time_t now = time(NULL);                                    /* 本机成交时间 */
    struct tm *local = localtime(&now);                         /* 本地时间结构 */
    Record *r;                                                  /* 新消费流水 */
    if (c < 0 || s->cards[c].lost)
        return "卡不存在或已挂失";
    if (d < 0 || !s->dishes[d].active)
        return "菜品不存在或已下架";
    if (s->cards[c].balance < s->dishes[d].price)
        return "余额不足";
    if (s->record_count == MAX_RECORDS || !local)
        return "记录容量已满或时间获取失败";
    /* 先检查全部条件，再同时更新余额、销量和流水，保证业务一致性。 */
    r = &s->records[s->record_count++];
    r->card_id = card_id;
    r->dish_id = dish_id;
    strcpy(r->name, s->dishes[d].name);
    r->amount = s->dishes[d].price;
    strftime(r->time, sizeof(r->time), "%Y-%m-%dT%H:%M:%S", local);
    s->cards[c].balance -= r->amount;
    ++s->dishes[d].sales;
    return NULL;
}

int save_data(const Canteen *s, const char *path)
{
    char temp[1024]; /* 同目录临时文件，避免跨文件系统替换 */
    FILE *f;         /* 输出文件 */
    int i, ok;       /* 循环下标及写入结果 */
    if (snprintf(temp, sizeof(temp), "%s.tmp", path) >= (int)sizeof(temp))
        return 0;
    f = fopen(temp, "w");
    if (!f)
        return 0;
    fprintf(f, "CANTEEN1 %d %d %d\n", s->card_count, s->dish_count, s->record_count);
    for (i = 0; i < s->card_count; ++i)
    {
        const Card *c = &s->cards[i]; /* 当前卡 */
        fprintf(f, "%d %s %d %d\n", c->id, c->name, c->balance, c->lost);
    }
    for (i = 0; i < s->dish_count; ++i)
    {
        const Dish *d = &s->dishes[i]; /* 当前菜 */
        fprintf(f, "%d %s %d %d %d\n", d->id, d->name, d->price, d->sales, d->active);
    }
    for (i = 0; i < s->record_count; ++i)
    {
        const Record *r = &s->records[i]; /* 当前流水 */
        fprintf(f, "%d %d %s %d %s\n", r->card_id, r->dish_id, r->name, r->amount, r->time);
    }
    ok = !ferror(f);
    if (fclose(f) != 0)
        ok = 0;
    if (!ok)
    {
        remove(temp);
        return 0;
    }
#ifdef _WIN32
    return MoveFileExA(temp, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temp, path) == 0;
#endif
}
int load_data(Canteen *s, const char *path)
{
    Canteen *t = (Canteen *)calloc(1, sizeof(Canteen)); /* 隔离解析，失败不影响现有状态 */
    FILE *f = fopen(path, "r");                         /* 输入文件 */
    char magic[16], extra[2];                           /* 版本标记和多余数据探测 */
    int i, j, ok = 0;                                   /* 读取下标、重复检查下标和结果 */
    if (!f || !t)
        goto finish;
    if (fscanf(f, "%15s%d%d%d", magic, &t->card_count, &t->dish_count, &t->record_count) !=
            4 ||
        strcmp(magic, "CANTEEN1"))
        goto finish;
    if (t->card_count < 0 || t->card_count > MAX_CARDS || t->dish_count < 0 ||
        t->dish_count > MAX_DISHES || t->record_count < 0 || t->record_count > MAX_RECORDS)
        goto finish;
    for (i = 0; i < t->card_count; ++i)
    {
        Card *c = &t->cards[i]; /* 待读取卡 */
        if (fscanf(f, "%d%63s%d%d", &c->id, c->name, &c->balance, &c->lost) != 4 ||
            c->id <= 0 || c->balance < 0 || (c->lost != 0 && c->lost != 1) ||
            !valid_name(c->name))
            goto finish;
        for (j = 0; j < i; ++j)
            if (t->cards[j].id == c->id)
                goto finish;
    }
    for (i = 0; i < t->dish_count; ++i)
    {
        Dish *d = &t->dishes[i]; /* 待读取菜 */
        if (fscanf(f, "%d%63s%d%d%d", &d->id, d->name, &d->price, &d->sales, &d->active) !=
                5 ||
            d->id <= 0 || d->price <= 0 || d->sales < 0 || d->sales > MAX_RECORDS ||
            (d->active != 0 && d->active != 1) || !valid_name(d->name))
            goto finish;
        for (j = 0; j < i; ++j)
            if (t->dishes[j].id == d->id)
                goto finish;
    }
    for (i = 0; i < t->record_count; ++i)
    {
        Record *r = &t->records[i]; /* 待读取消费流水 */
        if (fscanf(f, "%d%d%63s%d%19s", &r->card_id, &r->dish_id, r->name, &r->amount,
                   r->time) != 5 ||
            r->amount <= 0 || card_index(t, r->card_id) < 0 || dish_index(t, r->dish_id) < 0 ||
            strlen(r->time) != 19 || !valid_name(r->name))
            goto finish;
    }
    /* 流水是销量的依据，拒绝统计值与流水不一致的数据文件。 */
    for (i = 0; i < t->dish_count; ++i)
    {
        int count = 0; /* 对应菜品的实际流水数 */
        for (j = 0; j < t->record_count; ++j)
            if (t->records[j].dish_id == t->dishes[i].id)
                ++count;
        if (count != t->dishes[i].sales)
            goto finish;
    }
    if (fscanf(f, "%1s", extra) == 1 || ferror(f))
        goto finish;
    *s = *t;
    ok = 1;
finish:
    if (f)
        fclose(f);
    free(t);
    return ok;
}
