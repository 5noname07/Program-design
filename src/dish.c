#include "dish.h"
#include "validation.h"
#include <string.h>

int dish_index(const Canteen *s, int id)
{
    int i; /* 包括停用菜品，保证历史编号不被重新使用 */
    for (i = 0; i < s->dish_count; ++i)
        if (s->dishes[i].id == id)
            return i;
    return -1;
}

const char *put_dish(Canteen *s, int id, const char *name, int cents)
{
    int i = dish_index(s, id); /* 已有编号则修改，否则新增 */
    Dish *d;                   /* 当前菜品 */
    if (id <= 0 || !valid_name(name) || cents <= 0)
        return "编号、名称或价格无效";
    if (i >= 0 && !s->dishes[i].active)
        return "该编号已停用，请使用新编号";
    if (i < 0)
    {
        if (s->dish_count == MAX_DISHES)
            return "菜品容量已满";
        i = s->dish_count++;
        s->dishes[i].sales = 0;
    }
    d = &s->dishes[i];
    d->id = id;
    strcpy(d->name, name);
    d->price = cents;
    d->active = 1;
    return NULL;
}

const char *delete_dish(Canteen *s, int id)
{
    int i = dish_index(s, id); /* 使用软删除保护历史记录 */
    if (i < 0 || !s->dishes[i].active)
        return "菜品不存在或已经删除";
    s->dishes[i].active = 0;
    return NULL;
}
