#ifndef CANTEEN_CONSOLE_H
#define CANTEEN_CONSOLE_H

#ifdef __cplusplus
extern "C"
{
#endif

    /* 字符界面入口：path为数据文件路径。
     * 返回0表示正常退出，1表示加载失败；内部负责菜单循环与保存错误提示。 */
    int console_run(const char *path);

#ifdef __cplusplus
}
#endif
#endif
