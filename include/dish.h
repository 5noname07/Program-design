#ifndef CANTEEN_DISH_H
#define CANTEEN_DISH_H

#include "canteen_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* 菜品管理模块。s为系统状态，id为菜品编号。
     * 查询包含已停用菜品：返回下标，不存在返回-1。
     * 变更返回NULL成功，否则返回错误说明；失败不改变状态。 */
    int dish_index(const Canteen *s, int id);
    /* name为最多63字节、不含空白的UTF-8菜名，cents为正整数分值。
     * 未使用编号新增，已有在售编号修改；不允许复用停用编号。
     * 界面负责区分新增与修改入口，本模块不负责输出或保存。 */
    const char *put_dish(Canteen *s, int id, const char *name, int cents);
    /* 仅停用菜单项，保留历史销量和关联流水。 */
    const char *delete_dish(Canteen *s, int id);

#ifdef __cplusplus
}
#endif
#endif
