#include "MainWindow.h"
#include <QFile>
#include <QTextStream>
#include "GameWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // 1. Присваиваем окну имя объекта, чтобы стили QSS могли его таргетировать.
    setObjectName("menuWindow");

    // Загрузка файла стилей QSS
    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString style = stream.readAll();

        // 2. Применяем стили ТОЛЬКО к этому окну (this), а не ко всему приложению (qApp).
        this->setStyleSheet(style);
        file.close();
    }

    setupUI();
    setWindowTitle("Морской Бой - Главное Меню");
    resize(400, 350);
}

void MainWindow::setupUI()
{
    // ... (остальной код setupUI без изменений)
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Создаем кнопки
    QPushButton *btnSingle = new QPushButton("Одиночная игра", this);
    QPushButton *btnMulti = new QPushButton("Мультиплеер", this);
    QPushButton *btnSettings = new QPushButton("Настройки", this);
    QPushButton *btnExit = new QPushButton("Выход", this);

    // Применение стилей не требуется, так как QSS-селектор (QWidget#menuWindow QPushButton)
    // автоматически находит эти кнопки, потому что они являются потомками окна "menuWindow".

    // --- РАСПОЛОЖЕНИЕ КНОПОК ---

    layout->addStretch();
    layout->addWidget(btnSingle, 0, Qt::AlignLeft);
    layout->addWidget(btnMulti, 0, Qt::AlignLeft);
    layout->addWidget(btnSettings, 0, Qt::AlignLeft);
    layout->addWidget(btnExit, 0, Qt::AlignLeft);
    layout->addStretch();

    // Подключаем сигналы к слотам
    connect(btnSingle, &QPushButton::clicked, this, &MainWindow::onSinglePlayerClicked);
    connect(btnMulti, &QPushButton::clicked, this, &MainWindow::onMultiplayerClicked);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

void MainWindow::onSinglePlayerClicked()
{
    GameWindow *game = new GameWindow(); // Создаем окно игры без родителя, как отдельное окно

    // Скрываем меню
    this->hide();

    // Когда игра закончится и нажмут кнопку выхода:
    connect(game, &GameWindow::backToMenu, this, [=]() {
        this->show();       // Показать меню обратно
        // game удалится сам при закрытии, если у него стоит Qt::WA_DeleteOnClose
        // или можно удалить его явно: game->deleteLater();
    });

    // Также полезно обработать простое закрытие окна (крестик)
    // чтобы меню вернулось, даже если не доиграли
    // Для этого можно переопределить closeEvent в GameWindow, но для кнопки достаточно этого.

    game->setAttribute(Qt::WA_DeleteOnClose); // Удалить память при закрытии
    game->show();
}

void MainWindow::onMultiplayerClicked()
{
    QMessageBox::information(this, "В разработке", "Мультиплеер пока не доступен.");
}

void MainWindow::onSettingsClicked()
{
    QMessageBox::information(this, "В разработке", "Настройки пока не доступны.");
}

void MainWindow::onExitClicked()
{
    QApplication::quit();
}
