#ifndef CANTEEN_WINDOW_H
#define CANTEEN_WINDOW_H
#include "card.h"
#include "dish.h"
#include "consumption.h"
#include "statistics.h"
#include "storage.h"
#include "validation.h"
#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileInfo>
#include <memory>
#include <functional>
#include <stdexcept>

/* Qt只负责交互；所有扣款、校验和存储均调用C核心。无需moc或.ui生成文件。 */
class Window : public QWidget
{
    std::unique_ptr<Canteen> state{new Canteen{}}; // 堆上分配，避免大数组耗尽栈
    QString path;                                  // 数据文件路径
    int role = 0, studentId = 0;                   // 0未登录，1管理员，2学生；当前学生卡号
    QLabel *summary;                               // 当前身份和余额
    QTableWidget *table;                           // 菜单、卡信息或流水展示区
    QHBoxLayout *actions;                          // 根据身份动态建立的操作按钮

    // 金额显示统一保留两位小数，业务数据仍然是整数分。
    QString money(long long cents);
    void message(const QString &text);
    // 取消输入返回空字符串或-1，业务层会拒绝无效输入。
    QString text(const QString &title, bool secret = false);
    int number(const QString &title);
    int amount();
    void button(const QString &label, std::function<void()> callback);
    // 事务边界：在调用C业务函数前备份，保存失败时回滚内存。
    void change(std::function<const char *()> operation);
    void headers(const QStringList &names);
    void row(const QStringList &values);
    void dishes(const QString &keyword = QString());
    void records();
    void cards();
    void ranking();
    void editDish(bool create);
    void rebuild();

  public:
    explicit Window(QString file);
};
#endif
