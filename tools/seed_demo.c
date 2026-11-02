#include "canteen.h"
#include <stdio.h>
#include <stdlib.h>

/* 演示数据生成工具：不覆盖已有文件，消费时间由purchase读取电脑实时时间。
 * 用法：seed_demo 新文件路径。项目正常运行不依赖本工具。 */
static Canteen state; /* 演示用独立状态 */
int main(int argc, char **argv)
{
    static const char *names[] = {"张三", "李四", "王五", "赵六",
                                  "陈晨", "刘洋", "周宁"}; /* 姓名样本 */
    static const char *dishes[] = {
        "红烧肉",   "番茄炒蛋", "青菜",   "米饭",     "鸡腿",    "牛肉面",
        "鱼香肉丝", "宫保鸡丁", "土豆丝", "麻婆豆腐", "蒸蛋",    "紫菜汤",
        "糖醋里脊", "清蒸鱼",   "炒饭",   "炒面",     "馄饨",    "水饺",
        "包子",     "馒头",     "豆浆",   "鸡蛋",     "小米粥",  "南瓜粥",
        "玉米",     "红薯",     "咖喱鸡", "香菇鸡",   "西兰花",  "冬瓜汤",
        "萝卜排骨", "虾仁",     "酸菜鱼", "牛肉粉",   "凉拌黄瓜"}; /* 35种菜品 */
    static const int prices[] = {1200, 600, 300, 100, 800, 1500}; /* 原六道菜保留价格 */
    char name[64];                                                /* 自动补充的学生姓名 */
    int i;                                                        /* 循环计数 */
    FILE *existing;                                               /* 防止误覆盖 */
    if (argc != 2)
    {
        puts("用法：seed_demo 新文件路径");
        return 1;
    }
    existing = fopen(argv[1], "r");
    if (existing)
    {
        fclose(existing);
        puts("文件已经存在，拒绝覆盖。");
        return 1;
    }
    for (i = 0; i < 35; ++i)
    {
        if (i < 7)
            snprintf(name, sizeof(name), "%s", names[i]);
        else
            snprintf(name, sizeof(name), "同学%02d", i + 1);
        if (register_card(&state, 1001 + i, name) || recharge(&state, 1001 + i, 20000))
            return 1;
        if (put_dish(&state, i + 1, dishes[i], i < 6 ? prices[i] : 200 + (i % 12) * 100))
            return 1;
    }
    /* 每种菜至少有一次消费，便于展示排行；不伪造过去年月的时间。 */
    for (i = 0; i < 40; ++i)
        if (purchase(&state, 1001 + (i % 35), 1 + (i % 35)))
            return 1;
    if (set_lost(&state, 1003, 1) || !save_data(&state, argv[1]))
        return 1;
    printf("已生成35张卡、35种菜品、40笔实时消费：%s\n", argv[1]);
    return 0;
}
