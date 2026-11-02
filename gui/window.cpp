#include "window.h"

QString Window::money(long long cents)
{
    return QString::number(cents / 100) + "." +
           QString::number(cents % 100).rightJustified(2, '0');
}

void Window::message(const QString &text)
{
    QMessageBox::information(this, "食堂管理", text);
}

QString Window::text(const QString &title, bool secret)
{
    bool ok = false; // 对话框是否确认
    QString value = QInputDialog::getText(
        this, title, title, secret ? QLineEdit::Password : QLineEdit::Normal, QString(), &ok);
    inputCanceled = !ok;
    return value;
}

int Window::number(const QString &title)
{
    bool ok = false;                                                                // 取消标记
    int value = QInputDialog::getInt(this, title, title, 1, 1, 2147483647, 1, &ok); // 用户输入
    return ok ? value : -1;
}

int Window::amount()
{
    int cents = 0;                                                // C解析器输出
    QByteArray bytes = text("金额（元，最多两位小数）").toUtf8(); // 保证输入存活至函数结束
    if (inputCanceled)
        return -1;
    if (!parse_money(bytes.constData(), &cents))
    {
        message("金额须为正数，最多两位小数且不能超限。");
        return -1;
    }
    return cents;
}

void Window::button(const QString &label, std::function<void()> callback)
{
    auto *b = new QPushButton(label, this); // Qt父对象负责释放
    b->setMinimumHeight(34);
    actions->addWidget(b);
    connect(b, &QPushButton::clicked, this, callback);
}

void Window::change(std::function<const char *()> operation)
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

void Window::headers(const QStringList &names)
{
    table->clear();
    table->setRowCount(0);
    table->setColumnCount(names.size());
    table->setHorizontalHeaderLabels(names);
}

void Window::row(const QStringList &values)
{
    int r = table->rowCount(); // 新行位置
    table->insertRow(r);
    for (int i = 0; i < values.size(); ++i)
        table->setItem(r, i, new QTableWidgetItem(values[i]));
}

void Window::dishes(const QString &keyword)
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

void Window::records()
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

void Window::cards()
{
    headers({"卡号", "姓名", "余额（元）", "状态", "消费笔数", "累计消费（元）"});
    for (int i = 0; i < state->card_count; ++i)
    {
        const Card &c = state->cards[i];                             // 当前卡
        ConsumptionSummary result = card_summary(state.get(), c.id); // 复用C统计接口
        int count = result.count;                                    // 消费笔数
        long long total = result.amount;                             // 实际成交金额
        row({QString::number(c.id), QString::fromUtf8(c.name), money(c.balance),
             c.lost ? "挂失" : "正常", QString::number(count), money(total)});
    }
}

void Window::ranking()
{
    int order[MAX_DISHES];                        // C统计模块输出的排序下标
    int size = sales_ranking(state.get(), order); // 有效排名数
    headers({"排名", "编号", "菜名", "状态", "销量", "实收（元）"});
    int rank = 0; // 当前名次
    for (int position = 0; position < size; ++position)
    {
        const Dish &d = state->dishes[order[position]];
        long long total = dish_summary(state.get(), d.id).amount; // 复用C统计接口
        row({QString::number(++rank), QString::number(d.id), QString::fromUtf8(d.name),
             d.active ? "在售" : "已删除", QString::number(d.sales), money(total)});
    }
}

void Window::editDish(bool create)
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
    if (cents < 0)
        return;
    change([&] { return put_dish(state.get(), id, name.constData(), cents); });
}

void Window::rebuild()
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
                   QString expected = qEnvironmentVariable("CANTEEN_ADMIN_PASSWORD",
                                                           "admin123"); // 环境变量覆盖演示密码
                   QString password = text("管理员密码", true);         // 取消登录直接返回
                   if (inputCanceled)
                       return;
                   if (password == expected)
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
                   int id = number("就餐卡号"), i = card_index(state.get(), id); // 登录对象
                   if (id < 0)
                       return;
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
    button("查询菜品",
           [this]
           {
               QString keyword = text("菜名关键字");
               if (!inputCanceled)
                   dishes(keyword);
           });
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
                       change([&]
                              { return register_card(state.get(), id, name.constData()); });
               });
        button("充值",
               [this]
               {
                   int id = number("卡号");
                   if (id < 0)
                       return;
                   int cents = amount();
                   if (cents < 0)
                       return;
                   change([&] { return recharge(state.get(), id, cents); });
               });
        button("挂失/解挂",
               [this]
               {
                   int id = number("卡号"), i = card_index(state.get(), id); // 目标卡
                   if (id < 0)
                       return;
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
                   if (id > 0 &&
                       QMessageBox::question(this, "确认", "删除后无法恢复该编号，继续？") ==
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
                   if (id < 0)
                       return;
                   int index = dish_index(state.get(), id); // 成交确认前验证菜品
                   if (index < 0 || !state->dishes[index].active)
                   {
                       message("菜品不存在或已下架。");
                       return;
                   }
                   const Dish &dish = state->dishes[index]; // 用当前价格展示确认详情
                   QString details = QString("菜品：%1\n数量：1份\n扣款：%2元\n确认购买？")
                                         .arg(QString::fromUtf8(dish.name))
                                         .arg(money(dish.price));
                   if (QMessageBox::question(this, "确认购买", details) == QMessageBox::Yes)
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

Window::Window(QString file) : path(std::move(file))
{
    dataLock = lock_data(path.toLocal8Bit().constData());
    if (!dataLock)
        throw std::runtime_error("数据文件已被其他程序占用，或目录不可写。");
    if (!load_data(state.get(), path.toLocal8Bit().constData()))
    {
        unlock_data(dataLock);
        dataLock = nullptr;
        throw std::runtime_error("数据文件读取失败，请检查路径和格式。");
    }
    setWindowTitle("食堂自动化管理系统 · 课题15");
    resize(1120, 820);
    setMinimumSize(800, 700);
    auto *layout = new QVBoxLayout(this);                 // 主布局
    auto *title = new QLabel("食堂自动化管理系统", this); // 界面标题
    title->setStyleSheet("font-size:26px;font-weight:bold;color:#164e63;padding:12px;");
    summary = new QLabel(this);
    auto *body = new QHBoxLayout;      // 侧栏与内容区
    auto *sidebar = new QWidget(this); // 固定宽度操作区
    sidebar->setFixedWidth(160);
    actions = new QVBoxLayout(sidebar);
    actions->setAlignment(Qt::AlignTop);
    table = new QTableWidget(this);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(title);
    layout->addWidget(summary);
    body->addWidget(sidebar);
    body->addWidget(table, 1);
    layout->addLayout(body, 1);
    auto *clock = new QLabel(this); // 电脑实时时钟，每秒刷新
    auto updateClock = [clock]
    {
        clock->setText("电脑时间：" +
                       QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    };
    auto *timer = new QTimer(this); // 父对象负责释放计时器
    connect(timer, &QTimer::timeout, this, updateClock);
    timer->start(1000);
    updateClock();
    layout->addWidget(clock);
    setStyleSheet("QWidget{font-family:'Microsoft YaHei';font-size:13px;} "
                  "QPushButton{padding:8px;background:#e0f2fe;border:1px solid "
                  "#bae6fd;border-radius:4px;} "
                  "QHeaderView::section{background:#164e63;color:white;padding:8px;}");
    rebuild();
}

Window::~Window()
{
    unlock_data(dataLock);
}
