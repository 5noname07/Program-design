#include "statistics.h"

int sales_ranking(const Canteen *s, int *order)
{
    int i, j; /* 待确定名次、后续候选位置 */
    for (i = 0; i < s->dish_count; ++i)
        order[i] = i;

    /* 选择排序，只交换下标，不改变菜品的原始存储顺序。 */
    for (i = 0; i < s->dish_count; ++i)
        for (j = i + 1; j < s->dish_count; ++j)
        {
            const Dish *a = &s->dishes[order[i]]; /* 当前名次的菜品 */
            const Dish *b = &s->dishes[order[j]]; /* 待比较菜品 */
            if (b->sales > a->sales || (b->sales == a->sales && b->id < a->id))
            {
                int temp = order[i]; /* 交换临时下标 */
                order[i] = order[j];
                order[j] = temp;
            }
        }
    return s->dish_count;
}

ConsumptionSummary card_summary(const Canteen *s, int id)
{
    ConsumptionSummary result = {0, 0}; /* 按成交金额累计的结果 */
    int i;                              /* 流水下标 */
    for (i = 0; i < s->record_count; ++i)
        if (s->records[i].card_id == id)
        {
            ++result.count;
            result.amount += s->records[i].amount;
        }
    return result;
}

ConsumptionSummary dish_summary(const Canteen *s, int id)
{
    ConsumptionSummary result = {0, 0}; /* 按成交金额累计的结果 */
    int i;                              /* 流水下标 */
    for (i = 0; i < s->record_count; ++i)
        if (s->records[i].dish_id == id)
        {
            ++result.count;
            result.amount += s->records[i].amount;
        }
    return result;
}
