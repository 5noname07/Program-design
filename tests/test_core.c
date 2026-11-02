#include "canteen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    CHECK(!register_card(&s, 1001, "张三"));
    CHECK(register_card(&s, 1001, "重复"));
    CHECK(!put_dish(&s, 1, "米饭", 150));
    CHECK(!recharge(&s, 1001, 1000));
    CHECK(!purchase(&s, 1001, 1));
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
    CHECK(memcmp(&loaded, &before, sizeof(loaded)) == 0);
    CHECK(!save_data(&s, "missing_directory/state.txt"));
    remove("test_state.txt");
    remove("test_bad.txt");
    puts("All core tests passed.");
    return 0;
}
