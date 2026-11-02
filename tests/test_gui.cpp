#include "window.h"
#include <QTemporaryDir>
#include <QFile>
#include <QEventLoop>
#include <QDir>
#include <deque>
#include <cstdio>

#define CHECK(x)                                                                              \
    do                                                                                        \
    {                                                                                         \
        if (!(x))                                                                             \
            qFatal("GUI check failed: %s (line %d)", #x, __LINE__);                           \
    } while (0)
static Canteen state; // 测试独立存档，不触碰演示文件

int main(int argc, char **argv)
{
    QApplication app(argc, argv); // 使用真实Widgets，通过定时器操作模态输入框
    QTemporaryDir directory(QDir::currentPath() + "/gui-test-XXXXXX"); // 工作区内自动清理
    CHECK(directory.isValid());
    QString path = directory.filePath("gui_test.txt"); // 临时文件
    CHECK(!register_card(&state, 1001, "测试学生"));
    CHECK(!register_card(&state, 1002, "挂失学生"));
    CHECK(!recharge(&state, 1001, 5000));
    CHECK(!set_lost(&state, 1002, 1));
    CHECK(!put_dish(&state, 1, "红烧肉", 1200));
    CHECK(save_data(&state, path.toLocal8Bit().constData()));
    {
        Window window(path); // 窗口生命周期包含文件锁
        window.show();
        app.processEvents();
        std::deque<QString> answers; // 当前操作需要的输入序列
        bool cancelPurchase = false; // 用于验证取消购买不扣款
        QTimer driver;               // 在嵌套模态循环中也能自动回应对话框
        QObject::connect(
            &driver, &QTimer::timeout,
            [&]
            {
                QWidget *modal = QApplication::activeModalWidget(); // 当前模态对话框
                if (auto *input = qobject_cast<QInputDialog *>(modal))
                {
                    CHECK(!answers.empty());
                    QString value = answers.front();
                    answers.pop_front(); // 当前输入
                    if (input->inputMode() == QInputDialog::IntInput)
                        input->setIntValue(value.toInt());
                    else
                        input->setTextValue(value);
                    input->accept();
                }
                else if (auto *box = qobject_cast<QMessageBox *>(modal))
                {
                    if (box->standardButtons().testFlag(QMessageBox::Yes))
                        box->button(cancelPurchase ? QMessageBox::No : QMessageBox::Yes)
                            ->click();
                    else
                        box->accept();
                }
            });
        driver.start(20);
        QTimer watchdog; // 防止界面回归导致自动测试无限等待
        watchdog.setSingleShot(true);
        QObject::connect(&watchdog, &QTimer::timeout,
                         [] { qFatal("GUI operation timed out"); });
        auto click = [&](const QString &label, std::initializer_list<QString> values = {})
        {
            CHECK(answers.empty());
            for (const auto &value : values)
                answers.push_back(value);
            QPushButton *target = nullptr; // 按钮查找结果
            for (auto *b : window.findChildren<QPushButton *>())
                if (b->text() == label)
                    target = b;
            CHECK(target);
            watchdog.start(8000);
            target->click();
            watchdog.stop();
            app.processEvents();
            CHECK(answers.empty());
        };
        click("管理员登录", {"wrong"}); // 错误密码不能进入管理员界面
        CHECK(window.findChildren<QPushButton *>().size() == 2);
        click("管理员登录", {qEnvironmentVariable("CANTEEN_ADMIN_PASSWORD", "admin123")});
        click("充值", {"1001", "10.00"});
        CHECK(load_data(&state, path.toLocal8Bit().constData()) &&
              state.cards[0].balance == 6000);
        click("学生卡统计");
        CHECK(window.findChild<QTableWidget *>()->rowCount() == 2);
        CHECK(window.grab().save("gui_admin.png"));
        click("退出登录");
        click("学生登录", {"1002"}); // 挂失拒绝登录
        CHECK(window.findChildren<QPushButton *>().size() == 2);
        click("学生登录", {"1001"});
        CHECK(window.grab().save("gui_student.png"));
        cancelPurchase = true;
        click("购买一份", {"1"});
        CHECK(load_data(&state, path.toLocal8Bit().constData()) && state.record_count == 0);
        cancelPurchase = false;
        click("购买一份", {"1"});
        CHECK(load_data(&state, path.toLocal8Bit().constData()));
        CHECK(state.cards[0].balance == 4800 && state.record_count == 1 &&
              state.dishes[0].sales == 1);
        click("消费记录");
        CHECK(window.findChild<QTableWidget *>()->rowCount() == 1);
        click("退出登录");
        click("管理员登录", {qEnvironmentVariable("CANTEEN_ADMIN_PASSWORD", "admin123")});
        click("销售排行");
        CHECK(window.findChild<QTableWidget *>()->item(0, 4)->text() == "1");
    }
    Window reopened(path); // 前一窗口退出释放锁，可以重新启动并读取结果
    CHECK(load_data(&state, path.toLocal8Bit().constData()) && state.cards[0].balance == 4800);
    puts("Qt GUI integration tests passed.");
    return 0;
}
