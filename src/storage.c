#include "storage.h"
#include "card.h"
#include "dish.h"
#include "validation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

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
