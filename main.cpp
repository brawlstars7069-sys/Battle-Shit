#include <QApplication>
#include <QScreen>
#include "LoginWindow.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Устанавливаем иконку приложения и базовый стиль (если нужно)
    a.setApplicationName("Морской Бой");

    // Создаем наши окна
    LoginWindow loginWindow;
    MainWindow mainWindow;

    // Центрируем окно регистрации на экране
    const QScreen *screen = QGuiApplication::primaryScreen();
    loginWindow.move(screen->geometry().center() - loginWindow.rect().center());

    // Логика перехода:
    // Когда LoginWindow испускает сигнал registrationSuccessful,
    // мы закрываем окно регистрации и показываем главное меню.
    QObject::connect(&loginWindow, &LoginWindow::registrationSuccessful, [&]() {
        loginWindow.close();

        // Также центрируем главное меню
        mainWindow.move(screen->geometry().center() - mainWindow.rect().center());
        mainWindow.show();
    });

    // Показываем только окно регистрации при старте
    loginWindow.show();

    return a.exec();
}
