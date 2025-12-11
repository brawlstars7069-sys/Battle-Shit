#include "MainWindow.h"
#include <QFile>
#include <QTextStream>
#include "GameWindow.h"
#include <QPainter>
#include <QStyleOption>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setObjectName("menuWindow");

    // При желании можно оставить стили, но фон теперь рисуется вручную
    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString style = stream.readAll();
        this->setStyleSheet(style);
        file.close();
    }

    setupUI();
    setWindowTitle("Морской Бой - Главное Меню");
    resize(500, 450); // Чуть больше стартовый размер
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(15);
    layout->setContentsMargins(40, 40, 40, 40);

    // Заголовок
    QLabel *titleLabel = new QLabel("МОРСКОЙ БОЙ", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: white; "
                              "background-color: rgba(0,0,0,100); border-radius: 10px; padding: 10px;");

    // Создаем кнопки
    QPushButton *btnSingle = new QPushButton("Одиночная игра", this);
    QPushButton *btnMulti = new QPushButton("Мультиплеер", this);
    QPushButton *btnSettings = new QPushButton("Настройки", this);
    QPushButton *btnExit = new QPushButton("Выход", this);

    // Общий стиль для кнопок меню
    QString btnStyle =
        "QPushButton { "
        "  background-color: #ecf0f1; color: #2c3e50; font-size: 16px; font-weight: bold; "
        "  border: 2px solid #bdc3c7; border-radius: 8px; padding: 10px; "
        "}"
        "QPushButton:hover { background-color: #bdc3c7; border-color: #95a5a6; }";

    btnSingle->setStyleSheet(btnStyle);
    btnMulti->setStyleSheet(btnStyle);
    btnSettings->setStyleSheet(btnStyle);
    btnExit->setStyleSheet(btnStyle);

    // Добавляем элементы в Layout
    // Используем Stretch, чтобы центрировать блок кнопок по вертикали
    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addSpacing(20); // Отступ под заголовком
    layout->addWidget(btnSingle);
    layout->addWidget(btnMulti);
    layout->addWidget(btnSettings);
    layout->addWidget(btnExit);
    layout->addStretch(1);

    // Подключаем сигналы к слотам
    connect(btnSingle, &QPushButton::clicked, this, &MainWindow::onSinglePlayerClicked);
    connect(btnMulti, &QPushButton::clicked, this, &MainWindow::onMultiplayerClicked);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

// РИСУЕМ ФОН (Океан и пиксельные корабли)
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 1. Рисуем Океан (Градиент)
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0.0, QColor(0, 168, 255));   // Светло-голубой сверху
    gradient.setColorAt(1.0, QColor(0, 70, 140));    // Темно-синий снизу
    p.fillRect(rect(), gradient);

    // 2. Рисуем волны (простые линии)
    p.setPen(QPen(QColor(255, 255, 255, 30), 2));
    for (int y = 50; y < height(); y += 40) {
        for (int x = 0; x < width(); x += 60) {
            // Сдвиг волн через ряд
            int offsetX = (y % 80 == 0) ? 30 : 0;
            p.drawLine(x + offsetX, y, x + 20 + offsetX, y);
        }
    }

    // 3. Рисуем пиксельные кораблики на фоне
    // Они будут масштабироваться относительно размера окна, чтобы "картинка была одинаковой"

    // Кораблик 1 (Слева)
    int ship1X = width() * 0.15;
    int ship1Y = height() * 0.6;
    int scale1 = width() / 80; // Размер пикселя зависит от ширины окна

    p.setBrush(QColor(50, 50, 50));
    p.setPen(Qt::NoPen);

    // Корпус
    p.drawRect(ship1X, ship1Y, scale1 * 5, scale1);       // Низ
    p.drawRect(ship1X + scale1, ship1Y - scale1, scale1 * 3, scale1); // Верх
    p.drawRect(ship1X + scale1 * 2, ship1Y - scale1 * 2, scale1, scale1); // Труба

    // Кораблик 2 (Справа, поменьше)
    int ship2X = width() * 0.75;
    int ship2Y = height() * 0.3;
    int scale2 = width() / 120;

    p.setBrush(QColor(200, 50, 50)); // Красный враг
    p.drawRect(ship2X, ship2Y, scale2 * 4, scale2);
    p.drawRect(ship2X + scale2, ship2Y - scale2, scale2 * 2, scale2);
}

void MainWindow::onSinglePlayerClicked()
{
    GameWindow *game = new GameWindow();
    this->hide();

    connect(game, &GameWindow::backToMenu, this, [=]() {
        this->show();
    });

    game->setAttribute(Qt::WA_DeleteOnClose);
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
