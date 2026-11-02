#include "consumption.h"
#include "card.h"
#include "dish.h"
#include <string.h>
#include <time.h>

const char *purchase(Canteen *s, int card_id, int dish_id)
{
    int c = card_index(s, card_id), d = dish_index(s, dish_id); /* 两个对象下标 */
    time_t now = time(NULL);                                    /* 本机成交时间 */
    struct tm *local = localtime(&now);                         /* 本地时间结构 */
    Record *r;                                                  /* 新消费流水 */
    char timestamp[20]; /* 先构造合法时间，再修改业务数据 */
    if (c < 0 || s->cards[c].lost)
        return "卡不存在或已挂失";
    if (d < 0 || !s->dishes[d].active)
        return "菜品不存在或已下架";
    if (s->cards[c].balance < s->dishes[d].price)
        return "余额不足";
    if (s->record_count == MAX_RECORDS || now == (time_t)-1 || !local ||
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", local) != 19)
        return "记录容量已满或时间获取失败";
    /* 先检查全部条件，再同时更新余额、销量和流水，保证业务一致性。 */
    r = &s->records[s->record_count++];
    r->card_id = card_id;
    r->dish_id = dish_id;
    strcpy(r->name, s->dishes[d].name);
    r->amount = s->dishes[d].price;
    strcpy(r->time, timestamp);
    s->cards[c].balance -= r->amount;
    ++s->dishes[d].sales;
    return NULL;
}
