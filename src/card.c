#include "card.h"
#include "validation.h"
#include <limits.h>
#include <string.h>

int card_index(const Canteen *s, int id)
{
    int i; /* 顺序查找的下标 */
    for (i = 0; i < s->card_count; ++i)
        if (s->cards[i].id == id)
            return i;
    return -1;
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
