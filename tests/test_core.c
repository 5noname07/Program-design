#include "canteen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
/* 不使用assert：Release构建也必须实际执行测试表达式。 */
#define CHECK(x)                                                                              \
    do                                                                                        \
    {                                                                                         \
        if (!(x))                                                                             \
        {                                                                                     \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x);                              \
            exit(1);                                                                          \
        }                                                                                     \
    } while (0)
static Canteen s, loaded, before; /* 测试状态、加载结果、失败前快照 */
int main(void)
{
    int cents; /* 金额解析输出 */
    FILE *f;   /* 损坏文件构造句柄 */
    CHECK(parse_money("12.30", &cents) && cents == 1230);
    CHECK(!parse_money("1.234", &cents));
    CHECK(!parse_money("-1", &cents));
    CHECK(!parse_money("nan", &cents));
    CHECK(!parse_money("0", &cents));
    CHECK(!parse_money("999999999999999999999", &cents));
    CHECK(parse_money("21474836.47", &cents) && cents == INT_MAX);
    CHECK(!parse_money("21474836.48", &cents));
    CHECK(!parse_money("21474837", &cents));
    CHECK(parse_integer("2147483647", &cents) && cents == INT_MAX);
    CHECK(!parse_integer("2147483648", &cents));
    CHECK(!parse_integer("999999999999999999999", &cents));
    CHECK(valid_timestamp("2024-02-29T23:59:59"));
    CHECK(!valid_timestamp("2025-02-29T23:59:59"));
    CHECK(!valid_timestamp("2026-11-02T24:00:00"));
    CHECK(!register_card(&s, 1001, "张三"));
    CHECK(register_card(&s, 1001, "重复"));
    CHECK(!put_dish(&s, 1, "米饭", 150));
    CHECK(!recharge(&s, 1001, 1000));
    CHECK(!purchase(&s, 1001, 1));
    {
        time_t now = time(NULL);            /* 刚完成交易的电脑时间 */
        struct tm *local = localtime(&now); /* 本地年月日 */
        char today[11];                     /* 与实时日期比较，不写死测试日期 */
        CHECK(local && strftime(today, sizeof(today), "%Y-%m-%d", local) == 10);
        CHECK(valid_timestamp(s.records[0].time));
        CHECK(strncmp(today, s.records[0].time, 10) == 0);
    }
    CHECK(s.cards[0].balance == 850 && s.dishes[0].sales == 1 && s.record_count == 1);
    CHECK(!set_lost(&s, 1001, 1));
    before = s;
    CHECK(purchase(&s, 1001, 1));
    CHECK(memcmp(&before, &s, sizeof(s)) == 0);
    CHECK(!set_lost(&s, 1001, 0));
    CHECK(!put_dish(&s, 1, "新米饭", 900));
    CHECK(purchase(&s, 1001, 1));
    CHECK(s.records[0].amount == 150 && !strcmp(s.records[0].name, "米饭"));
    CHECK(!delete_dish(&s, 1));
    CHECK(purchase(&s, 1001, 1));
    CHECK(put_dish(&s, 1, "复用", 100));
    CHECK(save_data(&s, "test_state.txt"));
    CHECK(load_data(&loaded, "test_state.txt"));
    CHECK(loaded.cards[0].balance == 850 && loaded.record_count == 1 &&
          loaded.dishes[0].active == 0);
    before = loaded;
    f = fopen("test_bad.txt", "w");
    CHECK(f);
    fputs("CANTEEN1 999999 0 0", f);
    fclose(f);
    CHECK(!load_data(&loaded, "test_bad.txt"));
    f = fopen("test_bad.txt", "w");
    CHECK(f);
    fputs("CANTEEN1 1 1 1\n1 Test 0 0\n1 Rice 100 1 1\n1 1 Rice 100 2025-02-29T12:00:00\n", f);
    fclose(f);
    CHECK(!load_data(&loaded, "test_bad.txt"));
    CHECK(memcmp(&loaded, &before, sizeof(loaded)) == 0);
    CHECK(memcmp(&loaded, &before, sizeof(loaded)) == 0);
    CHECK(!save_data(&s, "missing_directory/state.txt"));
    {
        void *lock = lock_data("test_state.txt"); /* 首个实例持锁 */
        void *second;                             /* 模拟另一个界面打开相同存档 */
        CHECK(lock);
        second = lock_data("test_state.txt");
        CHECK(!second);
        unlock_data(lock);
        second = lock_data("test_state.txt");
        CHECK(second);
        unlock_data(second);
    }
    f = fopen("test_bad.txt", "w");
    CHECK(f);
    fputs("CANTEEN1 999999999999999999999999999999 0 0", f);
    fclose(f);
    CHECK(!load_data(&loaded, "test_bad.txt"));
    /* 独立统计模块必须按历史成交价汇总，且不能改变业务数据。 */
    {
        ConsumptionSummary card = card_summary(&s, 1001); /* 学生消费汇总 */
        ConsumptionSummary dish = dish_summary(&s, 1);    /* 删除菜品仍保留收入 */
        int order[MAX_DISHES];                            /* 排名下标 */
        CHECK(card.count == 1 && card.amount == 150);
        CHECK(dish.count == 1 && dish.amount == 150);
        CHECK(card_summary(&s, 9999).count == 0);
        CHECK(!put_dish(&s, 3, "菜三", 100));
        CHECK(!put_dish(&s, 2, "菜二", 100));
        before = s;
        CHECK(sales_ranking(&s, order) == 3);
        CHECK(s.dishes[order[0]].id == 1);
        CHECK(s.dishes[order[1]].id == 2 && s.dishes[order[2]].id == 3);
        CHECK(memcmp(&s, &before, sizeof(s)) == 0);
    }
    remove("test_state.txt");
    remove("test_bad.txt");
    puts("All core tests passed.");
    return 0;
}
