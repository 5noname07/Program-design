#ifndef CANTEEN_TYPES_H
#define CANTEEN_TYPES_H

/* 公共数据模型：仅定义类型和容量，不在头文件中定义全局变量。 */
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

#endif
