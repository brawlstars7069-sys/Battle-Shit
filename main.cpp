#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Настройка приложения
    a.setApplicationName("Морской Бой");
    a.setApplicationVersion("1.0.0");

    MainWindow w;

    return a.exec();
}
