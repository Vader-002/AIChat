#include "mainwindow.h"
#include "DatabaseManager.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("MyCompany");
    QApplication::setApplicationName("AIChat");

    if (!DatabaseManager::init()) {                              // ← 新增
        QMessageBox::critical(nullptr, "启动失败",
                              "无法初始化数据库，程序无法运行。");
        return 1;
    }

    MainWindow w;
    w.show();
    return QApplication::exec();
}
