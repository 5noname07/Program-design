#ifndef CANTEEN_STORAGE_H
#define CANTEEN_STORAGE_H

#include "canteen_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* 存储模块：s为系统状态，path为本地数据文件路径。
     * 返回1成功，0失败。加载失败不改变s；保存不修改s。
     * 加载会验证编号、关联关系与销量；保存以临时文件替换原文件。
     * 本模块不弹窗，不退出进程，由调用者决定错误处理。 */
    int load_data(Canteen *s, const char *path);
    int save_data(const Canteen *s, const char *path);
    /* 进程级独占访问：path为数据文件，返回锁句柄；失败返回NULL。
     * 界面启动后持有直到退出，防止字符版与Qt同时覆盖同一份数据。 */
    void *lock_data(const char *path);
    void unlock_data(void *lock);

#ifdef __cplusplus
}
#endif
#endif
