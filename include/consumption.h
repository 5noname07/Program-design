#ifndef CANTEEN_CONSUMPTION_H
#define CANTEEN_CONSUMPTION_H

#include "canteen_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* 消费模块：s为系统状态，card_id为付款卡号，dish_id为菜品编号。
     * 每次购买一份，同时更新余额、销量并产生一笔历史流水。
     * 返回NULL成功，否则返回错误说明；失败不改变状态。
     * 本模块不负责界面与磁盘保存，调用者在成功后提交存档。 */
    const char *purchase(Canteen *s, int card_id, int dish_id);

#ifdef __cplusplus
}
#endif
#endif
