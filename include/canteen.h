#ifndef CANTEEN_H
#define CANTEEN_H
#include <stddef.h>

/* 金额统一使用“分”，避免浮点计算造成扣款误差。固定容量便于初学者理解。 */
#define MAX_CARDS 500
#define MAX_DISHES 200
#define MAX_RECORDS 10000
typedef struct
{
    int id;        /* 唯一的正整数卡号 */
    char name[64]; /* UTF-8学生姓名，包括结尾零字符 */
    int balance;   /* 当前余额，单位：分 */
    int lost;      /* 0正常，1挂失 */
} Card;
typedef struct
{
    int id;        /* 唯一的菜品编号，删除后也不复用 */
    char name[64]; /* UTF-8菜名 */
    int price;     /* 当前售价，单位：分 */
    int sales;     /* 历史成功售出的份数 */
    int active;    /* 1在售，0已经删除 */
} Dish;
/* 保存成交时的名称与单价，修改或删除菜单不会改变历史记录。 */
typedef struct
{
    int card_id;   /* 付款学生的卡号 */
    int dish_id;   /* 成交菜品的编号 */
    char name[64]; /* 成交时的菜名快照 */
    int amount;    /* 本笔实际扣款，单位：分 */
    char time[20]; /* 本地时间YYYY-MM-DDTHH:MM:SS */
} Record;
typedef struct
{
    Card cards[MAX_CARDS];                    /* 学生卡数组 */
    Dish dishes[MAX_DISHES];                  /* 菜品数组，删除采用停用标志 */
    Record records[MAX_RECORDS];              /* 永久消费流水，不提供删除入口 */
    int card_count, dish_count, record_count; /* 三类记录的实际数量 */
} Canteen;
#ifdef __cplusplus
extern "C"
{
#endif
    /* 查询函数：s为系统，id为编号；返回数组下标，不存在返回-1。 */
    int card_index(const Canteen *s, int id);
    int dish_index(const Canteen *s, int id);
    /* 业务函数返回NULL表示成功，否则返回中文错误说明；失败不修改数据。 */
    const char *register_card(Canteen *s, int id, const char *name);
    /* 充值：id是已有卡号，cents是正整数分值；挂失卡允许充值。 */
    const char *recharge(Canteen *s, int id, int cents);
    /* 挂失/解挂：id是已有卡号，lost仅接受0或1。 */
    const char *set_lost(Canteen *s, int id, int lost);
    /* 添加或修改菜品：id为正编号，name为菜名，cents为正整数售价分值。
     * 已存在的在售编号按修改处理；界面负责区分“新增”和“修改”入口。 */
    const char *put_dish(Canteen *s, int id, const char *name, int cents);
    /* 删除：id为菜品编号，只停用菜单项，保留历史信息。 */
    const char *delete_dish(Canteen *s, int id);
    /* 购买：card_id为付款卡，dish_id为菜品；每次购买一份并产生一笔流水。 */
    const char *purchase(Canteen *s, int card_id, int dish_id);
    /* 文件读写：1成功，0失败。加载失败保持原数据；保存采用临时文件替换。 */
    int load_data(Canteen *s, const char *path);
    int save_data(const Canteen *s, const char *path);
    /* 金额文本转分：只接受正整数或最多两位小数，失败返回0。 */
    int parse_money(const char *text, int *cents);
#ifdef __cplusplus
}
#endif
#endif
