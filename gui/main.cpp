#include "canteen.h"
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
#include <algorithm>
#include <vector>
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
    QString money(long long cents)
    {
        return QString::number(cents / 100) + "." +
               QString::number(cents % 100).rightJustified(2, '0');
    }
    void message(const QString &text)
    {
        QMessageBox::information(this, "食堂管理", text);
    }
    // 取消输入返回空字符串或-1，业务层会拒绝无效输入。
    QString text(const QString &title, bool secret = false)
    {
        return QInputDialog::getText(this, title, title,
                                     secret ? QLineEdit::Password : QLineEdit::Normal);
    }
    int number(const QString &title)
    {
        bool ok = false; // 取消标记
        int value =
            QInputDialog::getInt(this, title, title, 1, 1, 2147483647, 1, &ok); // 用户输入
        return ok ? value : -1;
    }
    int amount()
    {
        int cents = 0;                                                // C解析器输出
        QByteArray bytes = text("金额（元，最多两位小数）").toUtf8(); // 保证输入存活至函数结束
        return parse_money(bytes.constData(), &cents) ? cents : -1;
    }
    void button(const QString &label, std::function<void()> callback)
    {
        auto *b = new QPushButton(label, this); // Qt父对象负责释放
        actions->addWidget(b);
        connect(b, &QPushButton::clicked, this, callback);
    }
    // 事务边界：在调用C业务函数前备份，保存失败时回滚内存。
    void change(std::function<const char *()> operation)
    {
        auto backup = std::make_unique<Canteen>(*state); // 操作前状态
        const char *error = operation();                 // C业务执行结果
        if (error)
        {
            message(QString::fromUtf8(error));
            return;
        }
        if (!save_data(state.get(), path.toLocal8Bit().constData()))
        {
            *state = *backup;
            message("保存失败，操作已经撤销。");
        }
        else
            message("操作成功，已保存。");
        dishes();
    }
    void headers(const QStringList &names)
    {
        table->clear();
        table->setRowCount(0);
        table->setColumnCount(names.size());
        table->setHorizontalHeaderLabels(names);
    }
    void row(const QStringList &values)
    {
        int r = table->rowCount(); // 新行位置
        table->insertRow(r);
        for (int i = 0; i < values.size(); ++i)
            table->setItem(r, i, new QTableWidgetItem(values[i]));
    }
    void dishes(const QString &keyword = QString())
    {
        headers({"编号", "菜品", "价格（元）", "销量"});
        for (int i = 0; i < state->dish_count; ++i)
        {
            const Dish &d = state->dishes[i]; // 当前菜品
            if (d.active && QString::fromUtf8(d.name).contains(keyword))
                row({QString::number(d.id), QString::fromUtf8(d.name), money(d.price),
                     QString::number(d.sales)});
        }
        if (role == 2)
        {
            const Card &c = state->cards[card_index(state.get(), studentId)]; // 当前学生卡
            summary->setText(QString("学生：%1  卡号：%2  余额：%3元")
                                 .arg(QString::fromUtf8(c.name))
                                 .arg(c.id)
                                 .arg(money(c.balance)));
        }
    }
    void records()
    {
        headers({"卡号", "购买时间", "菜品编号", "成交菜名", "金额（元）"});
        for (int i = 0; i < state->record_count; ++i)
        {
            const Record &r = state->records[i]; // 当前流水
            if (role == 1 || r.card_id == studentId)
                row({QString::number(r.card_id), QString::fromUtf8(r.time),
                     QString::number(r.dish_id), QString::fromUtf8(r.name), money(r.amount)});
        }
    }
    void cards()
    {
        headers({"卡号", "姓名", "余额（元）", "状态", "消费笔数", "累计消费（元）"});
        for (int i = 0; i < state->card_count; ++i)
        {
            const Card &c = state->cards[i]; // 当前卡
            int count = 0;
            long long total = 0; // 消费次数与总金额
            for (int j = 0; j < state->record_count; ++j)
                if (state->records[j].card_id == c.id)
                {
                    ++count;
                    total += state->records[j].amount;
                }
            row({QString::number(c.id), QString::fromUtf8(c.name), money(c.balance),
                 c.lost ? "挂失" : "正常", QString::number(count), money(total)});
        }
    }
    void ranking()
    {
        std::vector<int> order; // 只排序下标，不改动业务数组
        for (int i = 0; i < state->dish_count; ++i)
            order.push_back(i);
        std::sort(order.begin(), order.end(),
                  [this](int a, int b)
                  {
                      const Dish &x = state->dishes[a], &y = state->dishes[b]; // 比较菜品
                      return x.sales == y.sales ? x.id < y.id : x.sales > y.sales;
                  });
        headers({"排名", "编号", "菜名", "状态", "销量", "实收（元）"});
        int rank = 0; // 当前名次
        for (int i : order)
        {
            const Dish &d = state->dishes[i];
            long long total = 0; // 当前菜品与实收
            for (int j = 0; j < state->record_count; ++j)
                if (state->records[j].dish_id == d.id)
                    total += state->records[j].amount;
            row({QString::number(++rank), QString::number(d.id), QString::fromUtf8(d.name),
                 d.active ? "在售" : "已删除", QString::number(d.sales), money(total)});
        }
    }
    void editDish(bool create)
    {
        int id = number("菜品编号"); // 需要新增或修改的编号
        if (id < 0)
            return;
        int i = dish_index(state.get(), id); // 现有菜品下标
        if ((create && i >= 0) || (!create && (i < 0 || !state->dishes[i].active)))
        {
            message("新增不能重号，修改需要已有在售菜品。");
            return;
        }
        QByteArray name = text("菜名（不含空白）").toUtf8(); // UTF-8菜名
        if (name.isEmpty())
            return;
        int cents = amount(); // 菜品单价
        change([&] { return put_dish(state.get(), id, name.constData(), cents); });
    }
    void rebuild()
    {
        while (auto *item = actions->takeAt(0))
        {
            delete item->widget();
            delete item;
        }
        summary->setText(role == 1 ? "管理员已登录" : "请选择身份登录");
        table->setRowCount(0);
        if (role == 0)
        {
            table->clear();
            table->setColumnCount(0);
            button("管理员登录",
                   [this]
                   {
                       QString expected = qEnvironmentVariable(
                           "CANTEEN_ADMIN_PASSWORD", "admin123"); // 环境变量覆盖演示密码
                       if (text("管理员密码", true) == expected)
                       {
                           role = 1;
                           rebuild();
                       }
                       else
                           message("密码错误。");
                   });
            button("学生登录",
                   [this]
                   {
                       int id = number("就餐卡号"),
                           i = card_index(state.get(), id); // 登录对象
                       if (i < 0 || state->cards[i].lost)
                       {
                           message("卡不存在或已挂失。");
                           return;
                       }
                       studentId = id;
                       role = 2;
                       rebuild();
                   });
            return;
        }
        button("菜单", [this] { dishes(); });
        button("查询菜品", [this] { dishes(text("菜名关键字")); });
        if (role == 1)
        {
            button("办卡",
                   [this]
                   {
                       int id = number("新卡号");
                       if (id < 0)
                           return;
                       QByteArray name = text("学生姓名").toUtf8(); // 输入姓名
                       if (!name.isEmpty())
                           change(
                               [&]
                               { return register_card(state.get(), id, name.constData()); });
                   });
            button("充值",
                   [this]
                   {
                       int id = number("卡号");
                       if (id < 0)
                           return;
                       int cents = amount();
                       change([&] { return recharge(state.get(), id, cents); });
                   });
            button("挂失/解挂",
                   [this]
                   {
                       int id = number("卡号"), i = card_index(state.get(), id); // 目标卡
                       if (i < 0)
                       {
                           message("卡不存在。");
                           return;
                       }
                       bool lost = !state->cards[i].lost; // 目标状态
                       if (QMessageBox::question(this, "确认",
                                                 lost ? "确认挂失该卡？" : "确认解除挂失？") ==
                           QMessageBox::Yes)
                           change([&] { return set_lost(state.get(), id, lost); });
                   });
            button("新增菜品", [this] { editDish(true); });
            button("修改菜品", [this] { editDish(false); });
            button("删除菜品",
                   [this]
                   {
                       int id = number("删除菜品编号");
                       if (id > 0 && QMessageBox::question(this, "确认",
                                                           "删除后无法恢复该编号，继续？") ==
                                         QMessageBox::Yes)
                           change([&] { return delete_dish(state.get(), id); });
                   });
            button("销售排行", [this] { ranking(); });
            button("学生卡统计", [this] { cards(); });
        }
        else
        {
            button("购买一份",
                   [this]
                   {
                       int id = number("购买菜品编号");
                       if (id > 0 &&
                           QMessageBox::question(this, "确认购买", "确认购买一份并扣款？") ==
                               QMessageBox::Yes)
                           change([&] { return purchase(state.get(), studentId, id); });
                   });
        }
        button("消费记录", [this] { records(); });
        button("退出登录",
               [this]
               {
                   role = 0;
                   studentId = 0;
                   rebuild();
               });
        dishes();
    }

  public:
    explicit Window(QString file) : path(std::move(file))
    {
        if (!load_data(state.get(), path.toLocal8Bit().constData()))
            throw std::runtime_error("数据文件读取失败，请检查路径和格式。");
        setWindowTitle("食堂自动化管理系统 · 课题15");
        resize(1280, 720);
        auto *layout = new QVBoxLayout(this);                 // 主布局
        auto *title = new QLabel("食堂自动化管理系统", this); // 界面标题
        title->setStyleSheet("font-size:26px;font-weight:bold;color:#164e63;padding:12px;");
        summary = new QLabel(this);
        actions = new QHBoxLayout;
        table = new QTableWidget(this);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setAlternatingRowColors(true);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        layout->addWidget(title);
        layout->addWidget(summary);
        layout->addLayout(actions);
        layout->addWidget(table);
        setStyleSheet("QWidget{font-family:'Microsoft YaHei';font-size:13px;} "
                      "QPushButton{padding:8px;background:#e0f2fe;border:1px solid "
                      "#bae6fd;border-radius:4px;} "
                      "QHeaderView::section{background:#164e63;color:white;padding:8px;}");
        rebuild();
    }
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv); // Qt事件循环
    try
    {
        Window window(argc > 1 ? QString::fromLocal8Bit(argv[1])
                               : QString("data/canteen.txt")); // 主窗口
        window.show();
        return app.exec();
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(nullptr, "启动失败", QString::fromUtf8(e.what()));
        return 1;
    }
}
