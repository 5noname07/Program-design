#include "console.h"

/* 程序入口：可传入独立的数据文件路径，业务逻辑均在对应模块实现。 */
int main(int argc, char **argv)
{
    return console_run(argc > 1 ? argv[1] : "data/canteen.txt");
}
