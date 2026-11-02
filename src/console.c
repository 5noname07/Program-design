#include "console.h"
#include "card.h"
#include "dish.h"
#include "consumption.h"
#include "storage.h"
#include "validation.h"
#include "statistics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <windows.h>
#endif
static Canteen state, backup; /* 当前状态、保存失败时用于回滚的副本 */
static const char *data_path; /* 启动参数指定的数据文件 */

/* 读取整行；超长行全部丢弃，EOF按正常退出处理。 */
static void input(const char *prompt, char *out, int size)
{
    int ch; /* 用于清除超长行 */
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(out, size, stdin))
    {
        puts("\n输入结束。");
        exit(0);
    }
    if (!strchr(out, '\n') && !feof(stdin))
    {
        while ((ch = getchar()) != '\n' && ch != EOF)
        {
        }
        out[0] = 0;
        puts("输入过长，请重试。");
        return;
    }
    out[strcspn(out, "\r\n")] = 0;
}
/* 只接受完整非负整数，失败返回-1供上层拒绝操作。 */
static int number(const char *prompt)
{
    char text[80], *end; /* 输入缓冲区及转换结束位置 */
    long n;              /* 转换后的数值 */
    input(prompt, text, sizeof(text));
    if (!*text || strspn(text, "0123456789") != strlen(text))
        return -1;
    n = strtol(text, &end, 10);
    return *end || n < 0 || n > INT_MAX ? -1 : (int)n;
}
static int money(void)
{
    char text[80]; /* 元单位的金额文本 */
    int cents;     /* 解析后的分值 */
    input("金额（元）：", text, sizeof(text));
    return parse_money(text, &cents) ? cents : -1;
}
/* 每次变更立即保存；磁盘失败则恢复操作前状态，不允许出现假扣款。 */
static void commit(const char *error)
{
    if (error)
    {
        printf("操作失败：%s\n", error);
        return;
    }
    if (!save_data(&state, data_path))
    {
        state = backup;
        puts("保存失败，操作已撤销。请检查目录权限和磁盘空间。");
    }
    else
        puts("操作成功，已保存。");
}
static void menu_dishes(void)
{
    int i; /* 菜品下标 */
    puts("编号  菜名  单价（元）  销量");
    for (i = 0; i < state.dish_count; ++i)
    {
        const Dish *d = &state.dishes[i]; /* 当前菜品 */
        if (d->active)
            printf("%d  %s  %d.%02d  %d\n", d->id, d->name, d->price / 100, d->price % 100,
                   d->sales);
    }
}
/* 仅排序下标，避免改动数组和历史数据；同销量时按编号升序。 */
static void statistics(void)
{
    int order[MAX_DISHES], i; /* 排名下标与遍历位置 */
    sales_ranking(&state, order);
    puts("销售排行（含已删除菜品，收入按成交价统计）");
    for (i = 0; i < state.dish_count; ++i)
    {
        const Dish *d = &state.dishes[order[i]];                 /* 当前排名菜品 */
        ConsumptionSummary result = dish_summary(&state, d->id); /* 成交汇总 */
        long long revenue = result.amount;                       /* 实际收入分值 */
        printf("%d. %d %s %s 销量%d 收入%lld.%02lld\n", i + 1, d->id, d->name,
               d->active ? "在售" : "已删除", d->sales, revenue / 100, revenue % 100);
    }
    puts("学生卡使用情况：");
    for (i = 0; i < state.card_count; ++i)
    {
        ConsumptionSummary result = card_summary(&state, state.cards[i].id); /* 学生汇总 */
        int count = result.count;                                            /* 消费笔数 */
        long long total = result.amount;                                     /* 消费金额分值 */
        printf("%d %s %s 余额%d.%02d 消费%d笔 合计%lld.%02lld\n", state.cards[i].id,
               state.cards[i].name, state.cards[i].lost ? "挂失" : "正常",
               state.cards[i].balance / 100, state.cards[i].balance % 100, count, total / 100,
               total % 100);
    }
}
static void admin(void)
{
    char password[80], name[64];                             /* 密码和姓名/菜名输入 */
    const char *expected = getenv("CANTEEN_ADMIN_PASSWORD"); /* 可由环境变量覆盖演示密码 */
    int op, id, amount, status, i; /* 菜单项、编号、金额、状态、下标 */
    input("管理员密码：", password, sizeof(password));
    if (strcmp(password, expected ? expected : "admin123") != 0)
    {
        puts("密码错误。");
        return;
    }
    for (;;)
    {
        puts("\n管理员：1办卡 2充值 3挂失/解挂 4新增菜品 5修改菜品 6删除菜品 7查询菜单 8统计 "
             "0退出登录");
        op = number("选择：");
        if (op == 0)
            return;
        backup = state;
        switch (op)
        {
        case 1:
            id = number("卡号：");
            input("姓名（不含空格）：", name, sizeof(name));
            commit(register_card(&state, id, name));
            break;
        case 2:
            id = number("卡号：");
            amount = money();
            commit(recharge(&state, id, amount));
            break;
        case 3:
            id = number("卡号：");
            status = number("0解挂 / 1挂失：");
            commit(set_lost(&state, id, status));
            break;
        case 4:
        case 5:
            id = number("菜品编号：");
            i = dish_index(&state, id);
            if ((op == 4 && i >= 0) || (op == 5 && (i < 0 || !state.dishes[i].active)))
            {
                puts("新增不能重号，修改需要已有在售菜品。");
                break;
            }
            input("菜名：", name, sizeof(name));
            amount = money();
            commit(put_dish(&state, id, name, amount));
            break;
        case 6:
            id = number("删除的菜品编号：");
            commit(delete_dish(&state, id));
            break;
        case 7:
            menu_dishes();
            input("输入名称关键字（留空显示全部）：", name, sizeof(name));
            for (i = 0; i < state.dish_count; ++i)
                if (state.dishes[i].active && strstr(state.dishes[i].name, name))
                    printf("%d %s %d.%02d\n", state.dishes[i].id, state.dishes[i].name,
                           state.dishes[i].price / 100, state.dishes[i].price % 100);
            break;
        case 8:
            statistics();
            break;
        default:
            puts("无效选项，请重试。");
        }
    }
}
static void student(void)
{
    int id = number("就餐卡号："), c = card_index(&state, id), op, i,
        d; /* 登录编号、卡下标、菜单项、流水下标、菜品号 */
    if (c < 0 || state.cards[c].lost)
    {
        puts("卡不存在或已挂失，拒绝登录。");
        return;
    }
    for (;;)
    {
        printf("\n%s，余额%d.%02d元\n", state.cards[c].name, state.cards[c].balance / 100,
               state.cards[c].balance % 100);
        puts("学生：1浏览菜品 2购买一份 3本卡消费记录 0退出登录");
        op = number("选择：");
        if (op == 0)
            return;
        if (op == 1)
            menu_dishes();
        else if (op == 2)
        {
            d = number("菜品编号：");
            backup = state;
            commit(purchase(&state, id, d));
        }
        else if (op == 3)
        {
            for (i = 0; i < state.record_count; ++i)
            {
                const Record *r = &state.records[i]; /* 只显示当前学生流水 */
                if (r->card_id == id)
                    printf("%s 编号%d %s %d.%02d元\n", r->time, r->dish_id, r->name,
                           r->amount / 100, r->amount % 100);
            }
        }
        else
            puts("无效选项，请重试。");
    }
}
int console_run(const char *path)
{
    int op; /* 身份选择 */
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    data_path = path;
    if (!load_data(&state, data_path))
    {
        puts("数据文件无法读取或格式损坏。请检查文件；程序不会覆盖原文件。");
        return 1;
    }
    puts("欢迎使用食堂自动化管理系统");
    for (;;)
    {
        puts("\n1管理员登录 2学生登录 0退出系统");
        op = number("选择：");
        if (op == 0)
            return 0;
        if (op == 1)
            admin();
        else if (op == 2)
            student();
        else
            puts("无效选项，请重试。");
    }
}
