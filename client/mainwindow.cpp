#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include "gamewindow.h"
#include <QPainter>
#include <cmath>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), backgroundOffset(0), backgroundOffsetY(0), isUserRegistered(false)
{
    setObjectName("menuWindow");

    setMouseTracking(true);
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

    // Инициализация окна регистрации
    loginWindow = new LoginWindow(nullptr); // nullptr, чтобы окно было независимым
    connect(loginWindow, &LoginWindow::registrationSuccessful, this, &MainWindow::onRegistrationFinished);

    // ЗАПУСК ВО ВЕСЬ ЭКРАН
    showFullScreen();
}

void MainWindow::setBackgroundOffset(float offset) {
    backgroundOffset = offset;
    update();
}

void MainWindow::setBackgroundOffsetY(float offset) {
    backgroundOffsetY = offset;
    update();
}

void MainWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    central->setMouseTracking(true);
    setCentralWidget(central);

    menuContainer = new QWidget(central);
    menuContainer->setMouseTracking(true);

    settingsContainer = new QWidget(central);
    settingsContainer->setMouseTracking(true);

    multiplayerContainer = new QWidget(central);
    multiplayerContainer->setMouseTracking(true);

    setupMenuContainer();
    setupSettingsContainer();
    setupMultiplayerContainer();

    // Исходные позиции
    menuContainer->move(0, 0);
    settingsContainer->move(-width(), 0);
    multiplayerContainer->move(0, height());
}

void MainWindow::setupMenuContainer() {
    QVBoxLayout *layout = new QVBoxLayout(menuContainer);
    layout->setSpacing(15);
    layout->setContentsMargins(40, 40, 40, 40);

    QLabel *titleLabel = new QLabel("МОРСКОЙ БОЙ", menuContainer);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 36px; font-weight: bold; color: #2c2c2c; "
        "background-color: transparent; border: 4px dashed #555; padding: 15px;"
        );

    QPushButton *btnSingle = new QPushButton("ОДИНОЧНАЯ ИГРА", menuContainer);
    QPushButton *btnMulti = new QPushButton("МУЛЬТИПЛЕЕР", menuContainer);
    QPushButton *btnSettings = new QPushButton("НАСТРОЙКИ", menuContainer);
    QPushButton *btnExit = new QPushButton("ВЫХОД", menuContainer);

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

void MainWindow::setupSettingsContainer() {
    QVBoxLayout *mainLayout = new QVBoxLayout(settingsContainer);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QPushButton *btnBack = new QPushButton("НАЗАД", settingsContainer);
    btnBack->setFixedSize(100, 40);
    btnBack->setStyleSheet("background-color: #c0392b; color: white;");
    connect(btnBack, &QPushButton::clicked, this, &MainWindow::onBackFromSettingsClicked);

    QLabel *settingsTitle = new QLabel("НАСТРОЙКИ", settingsContainer);
    settingsTitle->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    settingsTitle->setAlignment(Qt::AlignCenter);

    topLayout->addWidget(btnBack);
    topLayout->addStretch();
    topLayout->addWidget(settingsTitle);
    topLayout->addStretch();
    QWidget *dummy = new QWidget(); dummy->setFixedSize(100, 40); topLayout->addWidget(dummy);

    QVBoxLayout *centerLayout = new QVBoxLayout();
    centerLayout->setAlignment(Qt::AlignCenter);

    QLabel *avatarLabel = new QLabel("ВАШ АВАТАР:", settingsContainer);
    avatarLabel->setStyleSheet("font-weight: bold; font-size: 18px;");
    avatarLabel->setAlignment(Qt::AlignCenter);

    currentAvatarPreview = new QLabel(settingsContainer);
    currentAvatarPreview->setFixedSize(120, 120);
    currentAvatarPreview->setStyleSheet("border: 4px solid #333; background-color: rgba(255,255,255,150);");
    currentAvatarPreview->setAlignment(Qt::AlignCenter);
    currentAvatarPreview->setText("NONE");

    btnChangeAvatar = new QPushButton("СМЕНИТЬ АВАТАР", settingsContainer);
    btnChangeAvatar->setFixedWidth(200);
    connect(btnChangeAvatar, &QPushButton::clicked, this, &MainWindow::onChangeAvatarClicked);

    centerLayout->addWidget(avatarLabel);
    centerLayout->addSpacing(15);
    centerLayout->addWidget(currentAvatarPreview);
    centerLayout->addSpacing(25);
    centerLayout->addWidget(btnChangeAvatar);

    mainLayout->addLayout(topLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(centerLayout);
    mainLayout->addStretch();

    avatarSelectionWidget = new QWidget(settingsContainer);
    avatarSelectionWidget->setMouseTracking(true);
    avatarSelectionWidget->hide();
    avatarSelectionWidget->setStyleSheet("background-color: rgba(248, 240, 227, 230);");

    QVBoxLayout *overlayLayout = new QVBoxLayout(avatarSelectionWidget);
    overlayLayout->setAlignment(Qt::AlignCenter);

    QLabel *overlayTitle = new QLabel("ВЫБЕРИТЕ ПЕРСОНАЖА", avatarSelectionWidget);
    overlayTitle->setStyleSheet("font-size: 20px; font-weight: bold; color: #333; background: transparent;");
    overlayTitle->setAlignment(Qt::AlignCenter);

    QWidget *gridContainer = new QWidget(avatarSelectionWidget);
    gridContainer->setStyleSheet("background: transparent;");
    QGridLayout *grid = new QGridLayout(gridContainer);
    grid->setSpacing(15);

    QStringList avatars;
    avatars << ":/avatars/CRking.png" << ":/avatars/2.png" << ":/avatars/3.png" << ":/avatars/4.png";

    for(int i=0; i<4; ++i) {
        QPushButton *btn = new QPushButton(avatarSelectionWidget);
        btn->setFixedSize(80, 80);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { border: 2px solid #555; background-color: #fff; } QPushButton:hover { border: 4px solid #333; }");

        QString path = (i < avatars.size()) ? avatars[i] : "";
        QPixmap pix(80, 80);

        if (QFile::exists(path)) {
            pix.load(path);
        } else {
            QColor col = QColor::fromHsv((i * 60) % 360, 150, 200);
            pix.fill(col);
            QPainter p(&pix);
            p.drawText(pix.rect(), Qt::AlignCenter, QString::number(i+1));
        }

        QIcon icon(pix);
        btn->setIcon(icon);
        btn->setIconSize(QSize(60, 60));
        btn->setProperty("avatarPath", path.isEmpty() ? QString("color:%1").arg(i) : path);

        connect(btn, &QPushButton::clicked, this, [=](){
            QString p = btn->property("avatarPath").toString();
            onAvatarSelected(p);
        });
        grid->addWidget(btn, 0, i);
    }

    QPushButton *btnCancel = new QPushButton("ОТМЕНА", avatarSelectionWidget);
    btnCancel->setFixedWidth(150);
    btnCancel->setStyleSheet("background-color: #95a5a6; color: white;");
    connect(btnCancel, &QPushButton::clicked, this, &MainWindow::onChangeAvatarClicked);

    overlayLayout->addStretch();
    overlayLayout->addWidget(overlayTitle);
    overlayLayout->addSpacing(20);
    overlayLayout->addWidget(gridContainer);
    overlayLayout->addSpacing(30);
    overlayLayout->addWidget(btnCancel);
    overlayLayout->addStretch();
}

void MainWindow::setupMultiplayerContainer() {
    QVBoxLayout *mainLayout = new QVBoxLayout(multiplayerContainer);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    QHBoxLayout *topLayout = new QHBoxLayout();

    QPushButton *btnBack = new QPushButton("НАЗАД", multiplayerContainer);
    btnBack->setFixedSize(100, 40);
    btnBack->setStyleSheet("background-color: #c0392b; color: white;");
    connect(btnBack, &QPushButton::clicked, this, &MainWindow::onBackFromMultiplayerClicked);

    QLabel *title = new QLabel("СПИСОК СЕРВЕРОВ", multiplayerContainer);
    title->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    title->setAlignment(Qt::AlignCenter);

    QWidget *dummy = new QWidget();
    dummy->setFixedSize(100, 40);

    topLayout->addWidget(btnBack);
    topLayout->addStretch();
    topLayout->addWidget(title);
    topLayout->addStretch();
    topLayout->addWidget(dummy);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(20);

    QPushButton *btnCreate = new QPushButton("СОЗДАТЬ СЕРВЕР", multiplayerContainer);
    btnCreate->setStyleSheet("background-color: #27ae60; color: white; min-width: 150px;");

    QPushButton *btnConnect = new QPushButton("ПОДКЛЮЧИТЬСЯ", multiplayerContainer);
    btnConnect->setStyleSheet("background-color: #2980b9; color: white; min-width: 150px;");

    connect(btnCreate, &QPushButton::clicked, this, &MainWindow::onCreateServerClicked);
    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    actionsLayout->addStretch();
    actionsLayout->addWidget(btnCreate);
    actionsLayout->addWidget(btnConnect);
    actionsLayout->addStretch();

    serverListWidget = new QListWidget(multiplayerContainer);
    serverListWidget->setStyleSheet(
        "QListWidget { "
        "   background-color: rgba(255, 255, 255, 180); "
        "   border: 3px solid #555; "
        "   font-family: 'Courier New'; "
        "   font-size: 16px; "
        "}"
        "QListWidget::item { padding: 10px; }"
        "QListWidget::item:selected { background-color: #3498db; color: white; }"
        );

    serverListWidget->addItem("Server #1 - [Map: Classic] - (1/2)");
    serverListWidget->addItem("Server #2 - [Map: Foggy] - (0/2)");
    serverListWidget->addItem("MegaBattle 2025 - (WAITING)");

    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(actionsLayout);
    mainLayout->addWidget(serverListWidget);
    mainLayout->addSpacing(10);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QSize s = event->size();

    if (avatarSelectionWidget) {
        avatarSelectionWidget->resize(s);
    }

    if (menuContainer->pos() == QPoint(0,0)) {
        menuContainer->resize(s);
        settingsContainer->resize(s);
        settingsContainer->move(-s.width(), 0);
        multiplayerContainer->resize(s);
        multiplayerContainer->move(0, s.height());

        if (backgroundOffset != 0) setBackgroundOffset(0);
        if (backgroundOffsetY != 0) setBackgroundOffsetY(0);
    }
    else if (settingsContainer->pos() == QPoint(0,0)) {
        settingsContainer->resize(s);
        menuContainer->resize(s);
        menuContainer->move(s.width(), 0);
        multiplayerContainer->resize(s);
        multiplayerContainer->move(0, s.height());
    }
    else if (multiplayerContainer->pos() == QPoint(0,0)) {
        multiplayerContainer->resize(s);
        menuContainer->resize(s);
        menuContainer->move(0, -s.height());
        settingsContainer->resize(s);
        settingsContainer->move(-s.width(), 0);
    }
    else {
        menuContainer->resize(s);
        settingsContainer->resize(s);
        multiplayerContainer->resize(s);
    }

    QMainWindow::resizeEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    mousePos = event->pos();
    update();
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), QColor(248, 240, 227));

    int w = width();
    int h = height();
    int pixelSize = 4;
    int shiftX = (mousePos.x() - w/2) * 0.05;
    int shiftY = (mousePos.y() - h/2) * 0.05;

    int cycleWidth = w + 200;
    int cycleHeight = h + 200;

    auto drawPixelShip = [&](int originalX, int originalY, int size, QColor color) {
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        int s = size;
        int shipShiftX = shiftX * 0.5;

        int currentX = originalX + (int)backgroundOffset;
        int wrappedX = ((currentX % cycleWidth) + cycleWidth) % cycleWidth;
        wrappedX -= 100;

        int currentY = originalY + (int)backgroundOffsetY;
        int wrappedY = ((currentY % cycleHeight) + cycleHeight) % cycleHeight;
        wrappedY -= 100;

        int finalX = wrappedX + shipShiftX;
        int finalY = wrappedY;

        p.drawRect(finalX, finalY, s*6, s);
        p.drawRect(finalX + s, finalY-s, s*4, s);
        p.drawRect(finalX + s*2, finalY-s*2, s, s);
    };

    drawPixelShip(w*0.1, h*0.2, 5, QColor(200, 190, 180));
    drawPixelShip(w*0.8, h*0.6, 6, QColor(180, 170, 160));
    drawPixelShip(w*0.2, h*0.8, 4, QColor(190, 180, 170));

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(169, 169, 169));

    for (int y = -100; y < h + 100; y += 40) {
        int bgShiftY = (int)backgroundOffsetY;
        int currentY = y + bgShiftY;

        int wrappedY = ((currentY % cycleHeight) + cycleHeight) % cycleHeight;
        wrappedY -= 100;

        int rowShift = shiftX * (float(wrappedY)/h);

        for (int x = -50; x < w + 50; x += 20) {
            int bgShiftX = (int)backgroundOffset;
            int finalX = x + rowShift + bgShiftX;
            int waveCycle = w + 100;
            finalX = (finalX % waveCycle + waveCycle) % waveCycle;

            if (finalX > w + 50) finalX -= waveCycle;
            if (finalX > w) finalX -= waveCycle;

            p.drawRect(finalX, wrappedY + shiftY * 0.2, pixelSize, pixelSize);
            p.drawRect(finalX + pixelSize, wrappedY + pixelSize + shiftY * 0.2, pixelSize, pixelSize);
        }
    }
}

// --- АНИМАЦИИ ---

void MainWindow::onSettingsClicked() {
    settingsContainer->move(-width(), 0);
    settingsContainer->show();

    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(0, 0));
    animMenu->setEndValue(QPoint(width(), 0));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animSettings = new QPropertyAnimation(settingsContainer, "pos", this);
    animSettings->setDuration(500);
    animSettings->setStartValue(QPoint(-width(), 0));
    animSettings->setEndValue(QPoint(0, 0));
    animSettings->setEasingCurve(QEasingCurve::InOutQuad);

    animBackground = new QPropertyAnimation(this, "backgroundOffset", this);
    animBackground->setDuration(500);
    animBackground->setStartValue(backgroundOffset);
    animBackground->setEndValue((float)width());
    animBackground->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animSettings->start(QAbstractAnimation::DeleteWhenStopped);
    animBackground->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onBackFromSettingsClicked() {
    if (avatarSelectionWidget->isVisible()) avatarSelectionWidget->hide();

    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(width(), 0));
    animMenu->setEndValue(QPoint(0, 0));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animSettings = new QPropertyAnimation(settingsContainer, "pos", this);
    animSettings->setDuration(500);
    animSettings->setStartValue(QPoint(0, 0));
    animSettings->setEndValue(QPoint(-width(), 0));
    animSettings->setEasingCurve(QEasingCurve::InOutQuad);

    animBackground = new QPropertyAnimation(this, "backgroundOffset", this);
    animBackground->setDuration(500);
    animBackground->setStartValue(backgroundOffset);
    animBackground->setEndValue(0.0f);
    animBackground->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animSettings->start(QAbstractAnimation::DeleteWhenStopped);
    animBackground->start(QAbstractAnimation::DeleteWhenStopped);
}

// --- МУЛЬТИПЛЕЕР И ВХОД ---

void MainWindow::onMultiplayerClicked() {
    // ПРОВЕРКА РЕГИСТРАЦИИ
    if (!isUserRegistered) {
        // Если не вошел - показываем окно входа и прерываем анимацию
        loginWindow->show();
        // Можно сделать модальным, чтобы пользователь не мог кликать в главное меню
        loginWindow->raise();
        loginWindow->activateWindow();
        return;
    }

    startMultiplayerAnimation();
}

void MainWindow::startMultiplayerAnimation() {
    multiplayerContainer->move(0, height());
    multiplayerContainer->show();

    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(0, 0));
    animMenu->setEndValue(QPoint(0, -height()));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animMultiplayer = new QPropertyAnimation(multiplayerContainer, "pos", this);
    animMultiplayer->setDuration(500);
    animMultiplayer->setStartValue(QPoint(0, height()));
    animMultiplayer->setEndValue(QPoint(0, 0));
    animMultiplayer->setEasingCurve(QEasingCurve::InOutQuad);

    animBackgroundY = new QPropertyAnimation(this, "backgroundOffsetY", this);
    animBackgroundY->setDuration(500);
    animBackgroundY->setStartValue(backgroundOffsetY);
    animBackgroundY->setEndValue((float)-height());
    animBackgroundY->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animMultiplayer->start(QAbstractAnimation::DeleteWhenStopped);
    animBackgroundY->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onRegistrationFinished() {
    // Вызывается сигналом из LoginWindow при успешном входе
    isUserRegistered = true;
    startMultiplayerAnimation();
}

void MainWindow::onBackFromMultiplayerClicked() {
    animMenu = new QPropertyAnimation(menuContainer, "pos", this);
    animMenu->setDuration(500);
    animMenu->setStartValue(QPoint(0, -height()));
    animMenu->setEndValue(QPoint(0, 0));
    animMenu->setEasingCurve(QEasingCurve::InOutQuad);

    animMultiplayer = new QPropertyAnimation(multiplayerContainer, "pos", this);
    animMultiplayer->setDuration(500);
    animMultiplayer->setStartValue(QPoint(0, 0));
    animMultiplayer->setEndValue(QPoint(0, height()));
    animMultiplayer->setEasingCurve(QEasingCurve::InOutQuad);

    animBackgroundY = new QPropertyAnimation(this, "backgroundOffsetY", this);
    animBackgroundY->setDuration(500);
    animBackgroundY->setStartValue(backgroundOffsetY);
    animBackgroundY->setEndValue(0.0f);
    animBackgroundY->setEasingCurve(QEasingCurve::InOutQuad);

    animMenu->start(QAbstractAnimation::DeleteWhenStopped);
    animMultiplayer->start(QAbstractAnimation::DeleteWhenStopped);
    animBackgroundY->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::onCreateServerClicked() {
    QMessageBox::information(this, "Сервер", "Создание сервера пока недоступно.");
}

void MainWindow::onConnectClicked() {
    if (serverListWidget->currentItem()) {
        QString serverName = serverListWidget->currentItem()->text();
        QMessageBox::information(this, "Подключение", "Попытка подключения к: " + serverName);
    } else {
        QMessageBox::warning(this, "Ошибка", "Выберите сервер из списка!");
    }
}

void MainWindow::onChangeAvatarClicked() {
    if (avatarSelectionWidget->isVisible()) {
        avatarSelectionWidget->hide();
    } else {
        avatarSelectionWidget->show();
        avatarSelectionWidget->raise();
    }
}

void MainWindow::onAvatarSelected(const QString &path) {
    selectedAvatarPath = path;
    if (path.startsWith("color:")) {
        int idx = path.split(":")[1].toInt();
        QPixmap pix(100, 100);
        pix.fill(QColor::fromHsv((idx * 60) % 360, 150, 200));
        currentAvatarPreview->setPixmap(pix);
    } else {
        QPixmap pix(path);
        if (!pix.isNull()) {
            currentAvatarPreview->setPixmap(pix.scaled(110, 110, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    avatarSelectionWidget->hide();
}

void MainWindow::onSinglePlayerClicked()
{
    GameWindow *game = new GameWindow(selectedAvatarPath);
    this->hide();
    connect(game, &GameWindow::backToMenu, this, [=]() {
        this->show();
    });
    game->setAttribute(Qt::WA_DeleteOnClose);
    game->show();
}

void MainWindow::onExitClicked() { QApplication::quit(); }
