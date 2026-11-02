#ifndef CANTEEN_CARD_H
#define CANTEEN_CARD_H

#include "canteen_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* 学生卡管理模块。s为系统状态，id为卡号。
     * 查询返回数组下标，不存在返回-1；变更返回NULL表示成功，否则返回错误说明。
     * 所有失败操作均保持原状态。本模块不负责文件保存或界面输出。 */
    int card_index(const Canteen *s, int id);
    /* name为非空UTF-8姓名，最多63字节且不含空白；新卡余额为0。 */
    const char *register_card(Canteen *s, int id, const char *name);
    /* cents为正整数分值；挂失卡仍允许充值。 */
    const char *recharge(Canteen *s, int id, int cents);
    /* lost仅接受0正常、1挂失。 */
    const char *set_lost(Canteen *s, int id, int lost);

#ifdef __cplusplus
}
#endif
#endif
