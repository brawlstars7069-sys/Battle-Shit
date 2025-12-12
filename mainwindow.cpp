#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include "gamewindow.h"
#include <QPainter>
#include <cmath> // для sin/cos

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setObjectName("menuWindow");

    // Включаем отслеживание мыши даже без нажатия кнопок
    setMouseTracking(true);
    // Важно включить и для centralWidget, иначе он перехватывает события
    if(centralWidget()) centralWidget()->setMouseTracking(true);

    QFile file(":/styles/styles.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        QString style = stream.readAll();
        this->setStyleSheet(style);
        file.close();
    }

    setupUI();
    setWindowTitle("Морской Бой - 8-BIT EDITION");
    resize(600, 500);
}

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    // Включаем трекинг для центрального виджета, чтобы фон реагировал на мышь поверх кнопок
    centralWidget->setMouseTracking(true);
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setSpacing(15);
    layout->setContentsMargins(40, 40, 40, 40);

    // Заголовок в стиле "печать"
    QLabel *titleLabel = new QLabel("МОРСКОЙ БОЙ", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 36px; font-weight: bold; color: #2c2c2c; "
        "background-color: transparent; border: 4px dashed #555; padding: 15px;"
        );

    QPushButton *btnSingle = new QPushButton("ОДИНОЧНАЯ ИГРА", this);
    QPushButton *btnMulti = new QPushButton("МУЛЬТИПЛЕЕР", this);
    QPushButton *btnSettings = new QPushButton("НАСТРОЙКИ", this);
    QPushButton *btnExit = new QPushButton("ВЫХОД", this);

    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addSpacing(30);
    layout->addWidget(btnSingle, 0, Qt::AlignCenter);
    layout->addWidget(btnMulti, 0, Qt::AlignCenter);
    layout->addWidget(btnSettings, 0, Qt::AlignCenter);
    layout->addWidget(btnExit, 0, Qt::AlignCenter);
    layout->addStretch(1);

    connect(btnSingle, &QPushButton::clicked, this, &MainWindow::onSinglePlayerClicked);
    connect(btnMulti, &QPushButton::clicked, this, &MainWindow::onMultiplayerClicked);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(btnExit, &QPushButton::clicked, this, &MainWindow::onExitClicked);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    mousePos = event->pos();
    update(); // Перерисовываем фон при движении мыши
}

// --- ОТРИСОВКА ФОНА (БУМАГА + ПИКСЕЛЬНЫЕ ВОЛНЫ) ---
void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false); // Отключаем сглаживание для пиксельного стиля

    // 1. Фон "Бумага"
    p.fillRect(rect(), QColor(248, 240, 227)); // Светло-бежевый (Linen/OldLace)

    int w = width();
    int h = height();
    int pixelSize = 4; // Размер одного "пикселя"

    // Вычисляем смещение от центра экрана до мыши
    // Коэффициент 0.05 делает движение "медленным" (как на резинке/параллакс)
    int shiftX = (mousePos.x() - w/2) * 0.05;
    int shiftY = (mousePos.y() - h/2) * 0.05;

    // 2. Декоративные кораблики на фоне
    // Рисуем их ДО волн, чтобы они были как бы "в море"
    auto drawPixelShip = [&](int x, int y, int size, QColor color) {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        int s = size; // масштабирование пикселя корабля

        // Смещаем кораблики тоже, но с другим коэффициентом (эффект глубины)
        int shipShiftX = shiftX * 0.5;

        // Простой силуэт
        p.drawRect(x + shipShiftX, y, s*6, s);        // Низ
        p.drawRect(x + s + shipShiftX, y-s, s*4, s);  // Середина
        p.drawRect(x + s*2 + shipShiftX, y-s*2, s, s);// Труба
    };

    drawPixelShip(w*0.1, h*0.2, 5, QColor(200, 190, 180)); // Очень бледный
    drawPixelShip(w*0.8, h*0.6, 6, QColor(180, 170, 160));
    drawPixelShip(w*0.2, h*0.8, 4, QColor(190, 180, 170));

    // 3. Пиксельные волны
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(169, 169, 169)); // Темно-серый (DarkGray)

    // Рисуем ряды волн
    for (int y = 50; y < h; y += 40) {
        // Сдвиг волны по фазе + сдвиг от мыши
        // Чем ниже волна, тем сильнее она может реагировать на мышь (эффект перспективы)
        int rowShift = shiftX * (float(y)/h);

        for (int x = -50; x < w + 50; x += 20) {
            // Рисуем "гребень" волны - просто квадратик или лесенка
            // Используем синус для легкого изгиба? Нет, просто пунктир в стиле 8-бит

            // Если (x + rowShift) попадает в сетку
            int finalX = x + rowShift;
            // Зацикливаем волны, чтобы не уезжали бесконечно
            finalX = (finalX % (w + 100));
            if (finalX < -50) finalX += (w + 100);

            // Рисуем пиксельный "кубик" волны
            p.drawRect(finalX, y + shiftY * 0.2, pixelSize, pixelSize);
            p.drawRect(finalX + pixelSize, y + pixelSize + shiftY * 0.2, pixelSize, pixelSize);
        }
    }
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

void MainWindow::onMultiplayerClicked() { QMessageBox::information(this, "Инфо", "Скоро..."); }
void MainWindow::onSettingsClicked() { QMessageBox::information(this, "Инфо", "Скоро..."); }
void MainWindow::onExitClicked() { QApplication::quit(); }
