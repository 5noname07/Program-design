#ifndef CANTEEN_STATISTICS_H
#define CANTEEN_STATISTICS_H

#include "canteen_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* 只读统计模块：不改变数组、不输出界面内容。 */
    typedef struct
    {
        int count;        /* 成功消费笔数 */
        long long amount; /* 累计成交金额，单位：分 */
    } ConsumptionSummary;
    /* s为系统状态；order须指向至少MAX_DISHES个int。
     * 填入按销量降序、编号升序排列的菜品下标，含停用菜品；返回有效长度。 */
    int sales_ranking(const Canteen *s, int *order);
    /* s为系统状态，id为卡号或菜品编号；无消费时返回零汇总。 */
    ConsumptionSummary card_summary(const Canteen *s, int id);
    ConsumptionSummary dish_summary(const Canteen *s, int id);

#ifdef __cplusplus
}
#endif
#endif
