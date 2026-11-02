#include "window.h"

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
