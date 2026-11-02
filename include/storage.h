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

#ifdef __cplusplus
}
#endif
#endif
