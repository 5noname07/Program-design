#ifndef CANTEEN_H
#define CANTEEN_H
/* 兼容原有调用的聚合入口。新模块应按需包含专门的头文件。
 * 头文件只声明接口，函数实现在src中各自的.c文件，不能#include .c。 */
#include "canteen_types.h"
#include "card.h"
#include "dish.h"
#include "consumption.h"
#include "storage.h"
#include "validation.h"
#include "statistics.h"
#endif
