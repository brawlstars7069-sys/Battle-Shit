#include "gamewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QTimer>
#include <QRandomGenerator>
#include <QPainter>
#include <QCursor>
#include <QRegion>

// --- Реализация AvatarWidget ---
AvatarWidget::AvatarWidget(bool isPlayer, QWidget *parent)
    : QWidget(parent), isPlayer(isPlayer)
{
    setFixedSize(80, 80);
}

void AvatarWidget::setAvatarImage(const QString &path) {
    if (!path.isEmpty()) {
        avatarImage.load(path);
        update();
    }
}

void AvatarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();
    int s = w / 8;

    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(230, 220, 210));
    p.drawRect(0, 0, w, h);

    if (!avatarImage.isNull() && isPlayer) {
        p.drawPixmap(4, 4, w-8, h-8, avatarImage);
    } else {
        QColor silhouetteColor = isPlayer ? QColor(40, 60, 100) : QColor(100, 40, 40);
        p.setBrush(silhouetteColor);
        p.setPen(Qt::NoPen);

        p.drawRect(2.5*s, 1.5*s, 3*s, 3*s); // Голова
        p.drawRect(1*s, 5*s, 6*s, 3*s);     // Плечи

        p.setBrush(QColor(240, 230, 200));
        p.drawRect(3*s, 2.5*s, 0.8*s, 0.8*s); // Левый глаз
        p.drawRect(4.2*s, 2.5*s, 0.8*s, 0.8*s); // Правый глаз
        p.drawRect(3.2*s, 3.8*s, 1.6*s, 0.5*s); // Рот
    }
}

// --- Реализация MessageBubble ---
MessageBubble::MessageBubble(QWidget *parent) : QLabel(parent) {
    setFixedSize(160, 60);
    setAlignment(Qt::AlignCenter);
    setWordWrap(true);
    setStyleSheet("background-color: transparent; border: none; color: transparent;");

    hideTimer = new QTimer(this);
    hideTimer->setSingleShot(true);
    connect(hideTimer, &QTimer::timeout, this, &MessageBubble::hideMessage);
}

void MessageBubble::showMessage(const QString &text) {
    setText(text);
    setStyleSheet(
        "background-color: #fff; color: #000; border: 2px dashed #000; "
        "padding: 5px; font-family: 'Courier New'; font-weight: bold; font-size: 14px;"
        );
    hideTimer->start(2000);
}

void MessageBubble::hideMessage() {
    setText("");
    setStyleSheet("background-color: transparent; border: none; color: transparent;");
}

// --- Реализация ManaBar ---
ManaBar::ManaBar(QWidget *parent) : QWidget(parent), currentMana(0) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(30);

    // Таймер для тряски при 100% маны
    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30); // 30 мс интервал
    connect(shakeTimer, &QTimer::timeout, this, &ManaBar::updateShake);
}

void ManaBar::setMana(int mana) {
    currentMana = std::clamp(mana, 0, 100);

    // Если мана полная - включаем тряску, если нет - выключаем
    if (currentMana >= 100) {
        if (!shakeTimer->isActive()) shakeTimer->start();
    } else {
        if (shakeTimer->isActive()) {
            shakeTimer->stop();
            shakeOffset = QPoint(0, 0);
        }
    }
    update();
}

void ManaBar::updateShake() {
    // Небольшая тряска: -1..1 пиксель
    int dx = QRandomGenerator::global()->bounded(3) - 1;
    int dy = QRandomGenerator::global()->bounded(3) - 1;
    shakeOffset = QPoint(dx, dy);
    update();
}

void ManaBar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Применяем смещение тряски
    p.translate(shakeOffset);

    int w = width();
    int h = height();

    // Функция для отрисовки формы "пилюли" с лесенкой
    auto drawPillShape = [&](int width, int height) {
        // Центр (самая широкая часть)
        p.drawRect(8, 0, width - 16, height);
        // Ступеньки к краям (лесенка)
        p.drawRect(5, 1, width - 10, height - 2);
        p.drawRect(3, 2, width - 6, height - 4);
        p.drawRect(2, 3, width - 4, height - 6);
        p.drawRect(0, 5, width, height - 10);
    };

    // 1. Рисуем фон (темная подложка)
    p.setBrush(QColor(40, 40, 40));
    p.setPen(Qt::NoPen);
    drawPillShape(w, h);

    // 2. Рисуем заполнение маны
    if (currentMana > 0) {
        int fillW = (w) * (float(currentMana) / 100.0);

        QColor manaColor(52, 152, 219);
        if (currentMana == 100) manaColor = QColor(241, 196, 15);

        // Создаем маску (Region) в форме пилюли, чтобы обрезать прямоугольник заполнения
        QRegion clipReg;
        clipReg += QRect(8, 0, w - 16, h);
        clipReg += QRect(5, 1, w - 10, h - 2);
        clipReg += QRect(3, 2, w - 6, h - 4);
        clipReg += QRect(2, 3, w - 4, h - 6);
        clipReg += QRect(0, 5, w, h - 10);

        p.setClipRegion(clipReg);

        p.setBrush(manaColor);
        p.drawRect(0, 0, fillW, h);

        // Рисуем блик сверху для объема
        p.setBrush(QColor(255, 255, 255, 50));
        p.drawRect(0, 2, fillW, h/3);

        p.setClipping(false);
    }

    // 3. Белая обводка (пиксельная)
    // Чтобы сделать обводку, можно нарисовать ту же форму, но линиями,
    // или просто оставить как есть, так как фон темный.
    // Добавим легкий контур внутри для стиля
    /*
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(255,255,255,100), 2));
    // Упрощенный контур
    p.drawRect(8, 2, w-16, h-4);
    */

    p.setPen(Qt::white);
    QFont f = p.font();
    f.setBold(true);
    f.setFamily("Courier New");
    p.setFont(f);
    p.drawText(rect(), Qt::AlignCenter, QString("%1 / 100 MP").arg(currentMana));
}

// --- Реализация AbilityWidget ---
AbilityWidget::AbilityWidget(int type, int cost, const QString &iconPath, QWidget *parent)
    : QWidget(parent), type(type), cost(cost), isAvailable(false)
{
    setFixedSize(60, 60);
    setCursor(Qt::ArrowCursor);

    if (!iconPath.isEmpty()) {
        iconPixmap.load(iconPath);
    }

    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);
    connect(shakeTimer, &QTimer::timeout, this, &AbilityWidget::updateShake);
}

void AbilityWidget::setAvailable(bool available) {
    if (isAvailable != available) {
        isAvailable = available;
        setCursor(isAvailable ? Qt::PointingHandCursor : Qt::ArrowCursor);

        if (!isAvailable) {
            shakeTimer->stop();
            shakeOffset = QPoint(0, 0);
        } else {
            if (underMouse()) {
                shakeTimer->start();
            }
        }
        update();
    }
}

void AbilityWidget::enterEvent(QEnterEvent *event) {
    if (isAvailable) {
        shakeTimer->start();
    }
    QWidget::enterEvent(event);
}

void AbilityWidget::leaveEvent(QEvent *event) {
    shakeTimer->stop();
    shakeOffset = QPoint(0, 0);
    update();
    QWidget::leaveEvent(event);
}

void AbilityWidget::updateShake() {
    int dx = QRandomGenerator::global()->bounded(5) - 2;
    int dy = QRandomGenerator::global()->bounded(5) - 2;
    shakeOffset = QPoint(dx, dy);
    update();
}

void AbilityWidget::mousePressEvent(QMouseEvent *event) {
    if (isAvailable && event->button() == Qt::LeftButton) {
        emit clicked(type);
    }
}

void AbilityWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Трясем все содержимое
    p.translate(shakeOffset);

    int w = width();
    int h = height();

    // Цвета фона
    QColor baseColor = isAvailable ? QColor(220, 220, 220) : QColor(80, 80, 80);

    // 1. Фон
    p.setBrush(baseColor);
    p.setPen(Qt::NoPen);
    p.drawRect(4, 4, w-8, h-8); // Чуть меньше рамки

    // 2. Иконка
    if (!iconPixmap.isNull()) {
        if (!isAvailable) p.setOpacity(0.3);
        p.drawPixmap(rect().adjusted(6, 6, -6, -6), iconPixmap);
        p.setOpacity(1.0);
    } else {
        // Заглушка (рисование фигур), если нет картинки
        QColor iconColor;
        if (!isAvailable) iconColor = QColor(120, 120, 120);
        else {
            if (type == 1) iconColor = QColor(46, 204, 113);
            else if (type == 2) iconColor = QColor(230, 126, 34);
            else iconColor = QColor(231, 76, 60);
        }
        p.setBrush(iconColor);
        p.setPen(QPen(Qt::black, 2));
        if (type == 1) {
            p.drawEllipse(w*0.2, h*0.3, w*0.3, h*0.3);
            p.drawEllipse(w*0.5, h*0.4, w*0.3, h*0.3);
        } else if (type == 2) {
            p.drawEllipse(w*0.2, h*0.2, w*0.6, h*0.6);
            p.setBrush(Qt::red); p.drawEllipse(w*0.5, h*0.3, 5, 5);
        } else if (type == 3) {
            p.drawRect(w*0.3, h*0.2, 10, 30); p.drawRect(w*0.5, h*0.3, 10, 30);
        }
    }

    // 3. Пиксельная рамка (Frame)
    // Внешняя черная обводка
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(Qt::black, 2));
    p.drawRect(0, 0, w, h);

    // Внутренняя белая рамка (блик)
    p.setPen(QPen(Qt::white, 2));
    p.drawRect(2, 2, w-4, h-4);

    // Внутренняя черная рамка
    p.setPen(QPen(Qt::black, 2));
    p.drawRect(4, 4, w-8, h-8);

    // Цена маны
    p.setPen(isAvailable ? Qt::black : Qt::white);
    QFont f = p.font();
    f.setPixelSize(10);
    f.setBold(true);
    p.setFont(f);
    p.drawText(rect().adjusted(0,0,-6,-6), Qt::AlignBottom | Qt::AlignRight, QString::number(cost));
}


// --- DraggableShipLabel ---
class DraggableShipLabel : public QLabel {
public:
    int shipId, size;
    DraggableShipLabel(int id, int s, QWidget* p=nullptr) : QLabel(p), shipId(id), size(s) {
        setFixedSize(s * 30 + 10, 40);
        setCursor(Qt::OpenHandCursor);
        setStyleSheet("background-color: transparent;");
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QRect shipRect(5, 5, size * 30, 30);
        BoardWidget::drawShipShape(p, size, Orientation::Horizontal, shipRect, false, false);
    }
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();
            mimeData->setText(QString("%1:%2").arg(shipId).arg(0));
            drag->setMimeData(mimeData);

            QPixmap pixmap(size * 30, 30);
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            BoardWidget::drawShipShape(p, size, Orientation::Horizontal, QRect(0,0,size*30,30), false, false);
            p.end();

            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(15, 15));
            if (drag->exec(Qt::MoveAction) == Qt::MoveAction) this->hide();
        }
    }
};

// --- GameWindow ---

GameWindow::GameWindow(const QString &playerAvatarPath, QWidget *parent)
    : QWidget(parent), isBattleStarted(false), isGameOver(false), isAnimating(false), playerMana(0), currentPlayerAvatarPath(playerAvatarPath)
{
    setWindowTitle("Морской Бой");
    resize(1000, 750);
    setMouseTracking(true);
    this->installEventFilter(this);

    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);
    connect(shakeTimer, &QTimer::timeout, this, &GameWindow::updateShake);

    initShips();

    hitPhrases << "БАБАХ!" << "ПОЛУЧИ!" << "В ЯБЛОЧКО!" << "ЕСТЬ ПРОБИТИЕ!" << "ХА-ХА!";
    killPhrases << "НА ДНО!" << "БУЛЬ-БУЛЬ!" << "КОРМ ДЛЯ РЫБ!" << "МИНУС ОДИН!" << "ПРОЩАЙ!";
    missPhrases << "УПС..." << "МИМО!" << "МАЗИЛА!" << "В МОЛОКО" << "ЭХ...";

    setupUI();

    if (!currentPlayerAvatarPath.isEmpty()) {
        playerAvatar->setAvatarImage(currentPlayerAvatarPath);
    }
}

GameWindow::~GameWindow() {
    qDeleteAll(playerShips);
    qDeleteAll(enemyShips);
}

QString GameWindow::getRandomPhrase(const QStringList &list) {
    if (list.isEmpty()) return "";
    int index = QRandomGenerator::global()->bounded(list.size());
    return list[index];
}

bool GameWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        QPoint globalPos = QCursor::pos();
        mousePos = this->mapFromGlobal(globalPos);
        update();
    }
    return QWidget::eventFilter(watched, event);
}

void GameWindow::resizeEvent(QResizeEvent *event) {
    if (rpsOverlay) {
        rpsOverlay->resize(this->size());
    }
    QWidget::resizeEvent(event);
}

void GameWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    p.fillRect(rect(), QColor(248, 240, 227));

    int w = width();
    int h = height();
    int pixelSize = 4;

    int shiftX = (mousePos.x() - w/2) * 0.03;
    int shiftY = (mousePos.y() - h/2) * 0.03;

    auto drawPixelShip = [&](int x, int y, int size) {
        p.setBrush(QColor(180, 170, 160));
        p.setPen(Qt::NoPen);
        int s = size;
        int px = x + shiftX * 0.5;
        int py = y + shiftY * 0.5;
        p.drawRect(px, py, s*6, s);
        p.drawRect(px + s, py-s, s*4, s);
        p.drawRect(px + s*2, py-s*2, s, s);
    };

    drawPixelShip(100, 120, 4);
    drawPixelShip(w - 150, h - 120, 5);

    p.setBrush(QColor(160, 160, 160));
    for (int y = 0; y < h; y += 50) {
        int rowShift = shiftX * (float(y)/h);
        for (int x = -50; x < w + 50; x += 30) {
            int finalX = (x + rowShift) % (w + 100);
            if (finalX < -50) finalX += (w + 100);
            p.drawRect(finalX, y + shiftY * 0.2, pixelSize, pixelSize);
            p.drawRect(finalX + pixelSize, y + pixelSize + shiftY * 0.2, pixelSize, pixelSize);
        }
    }
}

void GameWindow::initShips() {
    int id = 0;
    auto createFleet = [&](QVector<Ship*>& fleet) {
        fleet.push_back(new Ship(id++, 4));
        fleet.push_back(new Ship(id++, 3));
        fleet.push_back(new Ship(id++, 3));
        fleet.push_back(new Ship(id++, 2));
        fleet.push_back(new Ship(id++, 2));
        fleet.push_back(new Ship(id++, 2));
        fleet.push_back(new Ship(id++, 1));
        fleet.push_back(new Ship(id++, 1));
        fleet.push_back(new Ship(id++, 1));
        fleet.push_back(new Ship(id++, 1));
    };
    createFleet(playerShips);
    createFleet(enemyShips);
}

void GameWindow::addInitialTargets(int x, int y) {
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};
    for(int i=0; i<4; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) enemyTargetQueue.append(QPoint(nx, ny));
    }
}

void GameWindow::determineNextTargetLine() {
    if (shipHitPoints.size() < 2) return;
    QPoint p1 = shipHitPoints.first();
    QPoint p2 = shipHitPoints.last();
    bool isHorizontal = (p1.y() == p2.y());
    bool isVertical = (p1.x() == p2.x());

    if (isHorizontal) {
        int y = p1.y();
        int minX = 10, maxX = -1;
        for(const auto& p : shipHitPoints) {
            if (p.x() < minX) minX = p.x();
            if (p.x() > maxX) maxX = p.x();
        }
        enemyTargetQueue.clear();
        enemyTargetQueue.append(QPoint(minX - 1, y));
        enemyTargetQueue.append(QPoint(maxX + 1, y));
    } else if (isVertical) {
        int x = p1.x();
        int minY = 10, maxY = -1;
        for(const auto& p : shipHitPoints) {
            if (p.y() < minY) minY = p.y();
            if (p.y() > maxY) maxY = p.y();
        }
        enemyTargetQueue.clear();
        enemyTargetQueue.append(QPoint(x, minY - 1));
        enemyTargetQueue.append(QPoint(x, maxY + 1));
    }
}

void GameWindow::setupUI() {
    QVBoxLayout *globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(0);

    // Header
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: rgba(60, 50, 40, 200); border-bottom: 2px solid #555;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 0, 20, 0);
    QLabel *gameTitle = new QLabel("БОЙ", this);
    gameTitle->setStyleSheet("color: #f0e6d2; font-weight: bold; font-size: 24px; font-family: 'Courier New';");
    exitToMenuBtn = new QPushButton("ВЫХОД", this);
    exitToMenuBtn->setCursor(Qt::PointingHandCursor);
    exitToMenuBtn->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; border: 2px solid #922b21; }"
        "QPushButton:hover { background-color: #e74c3c; }"
        );
    connect(exitToMenuBtn, &QPushButton::clicked, this, &GameWindow::onExitToMenuClicked);
    headerLayout->addWidget(gameTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(exitToMenuBtn);
    globalLayout->addWidget(headerWidget);

    // Game Content
    QWidget *gameContentWidget = new QWidget(this);
    gameContentWidget->installEventFilter(this);
    gameContentWidget->setStyleSheet("background: transparent;");
    gameContentWidget->setMouseTracking(true);

    QHBoxLayout *mainLayout = new QHBoxLayout(gameContentWidget);
    mainLayout->setContentsMargins(30, 10, 30, 20);
    mainLayout->setSpacing(20);

    // --- ЛЕВАЯ КОЛОНКА (ИГРОК) ---
    QVBoxLayout *leftLayout = new QVBoxLayout();

    playerAvatar = new AvatarWidget(true, this);
    playerAvatar->hide();
    playerMessage = new MessageBubble(this);

    QVBoxLayout *playerAvatarLayout = new QVBoxLayout();
    playerAvatarLayout->addWidget(playerMessage, 0, Qt::AlignCenter);
    playerAvatarLayout->addWidget(playerAvatar, 0, Qt::AlignCenter);

    QLabel *playerTitle = new QLabel("ВАШ ФЛОТ");
    playerTitle->setAlignment(Qt::AlignCenter);
    playerTitle->setStyleSheet("font-weight: bold; color: #333; font-size: 18px; margin-bottom: 5px;");

    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true);
    playerBoard->setShowShips(true);
    playerBoard->setupSizePolicy();
    playerBoard->installEventFilter(this);

    connect(playerBoard, &BoardWidget::missileImpact, this, &GameWindow::onMissileImpact);

    leftLayout->addLayout(playerAvatarLayout);
    leftLayout->addSpacing(10);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(1);

    // --- ЦЕНТРАЛЬНАЯ КОЛОНКА ---
    centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    centerWidget->installEventFilter(this);
    centerWidget->setMouseTracking(true);

    // --- ОБНОВЛЕННЫЙ СТИЛЬ ЦЕНТРАЛЬНОГО ВИДЖЕТА ---
    centerWidget->setStyleSheet(
        "background-color: #f0e6d2; "      // Цвет плотной бумаги/пергамента
        "border: 4px solid #2c3e50; "       // Жирная темная рамка
        "border-radius: 0px;"               // Никаких скруглений!
        );

    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(15, 15, 15, 15);

    infoLabel = new QLabel("РАССТАВЬТЕ\nКОРАБЛИ");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setMinimumHeight(60);
    infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; border-bottom: 2px solid #ccc; padding-bottom: 10px; font-family: 'Courier New';");

    // 1. Панель расстановки
    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsLayout = new QVBoxLayout(shipsSetupPanel);
    shipsLayout->setSpacing(5);
    for (Ship* s : playerShips) {
        DraggableShipLabel *shipLabel = new DraggableShipLabel(s->id, s->size);
        shipsLayout->addWidget(shipLabel, 0, Qt::AlignCenter);
    }

    randomPlaceBtn = new QPushButton("АВТО-РАССТАНОВКА");
    randomPlaceBtn->setMinimumHeight(40);
    randomPlaceBtn->setCursor(Qt::PointingHandCursor);
    randomPlaceBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; font-size: 16px; font-weight: bold; border: 2px solid #2980b9; }"
        "QPushButton:hover { background-color: #5dade2; }"
        );
    connect(randomPlaceBtn, &QPushButton::clicked, this, &GameWindow::onRandomPlaceClicked);

    startBattleBtn = new QPushButton("В БОЙ!");
    startBattleBtn->setMinimumHeight(50);
    startBattleBtn->setCursor(Qt::PointingHandCursor);
    startBattleBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; font-size: 20px; font-weight: bold; border: 2px solid #1e8449; }"
        "QPushButton:hover { background-color: #2ecc71; }"
        );
    connect(startBattleBtn, &QPushButton::clicked, this, &GameWindow::onStartBattleClicked);

    // 2. Панель способностей
    battlePanel = new QWidget();
    QVBoxLayout *battlePanelLayout = new QVBoxLayout(battlePanel);
    battlePanelLayout->setAlignment(Qt::AlignCenter);
    battlePanelLayout->setSpacing(15);

    manaBar = new ManaBar(battlePanel);

    QWidget *abilitiesContainer = new QWidget(battlePanel);
    QVBoxLayout *absLayout = new QVBoxLayout(abilitiesContainer);
    absLayout->setSpacing(10);
    absLayout->setAlignment(Qt::AlignCenter);

    // --- ЗДЕСЬ УКАЗЫВАЮТСЯ ПУТИ К КАРТИНКАМ ---
    ability1 = new AbilityWidget(1, 60, ":/images/fog.png");
    ability2 = new AbilityWidget(2, 80, ":/images/radar.png");
    ability3 = new AbilityWidget(3, 100, ":/images/airstrike.png");

    connect(ability1, &AbilityWidget::clicked, this, &GameWindow::onAbilityClicked);
    connect(ability2, &AbilityWidget::clicked, this, &GameWindow::onAbilityClicked);
    connect(ability3, &AbilityWidget::clicked, this, &GameWindow::onAbilityClicked);

    absLayout->addWidget(ability1);
    absLayout->addWidget(ability2);
    absLayout->addWidget(ability3);

    battlePanelLayout->addWidget(new QLabel("МАНА"));
    battlePanelLayout->addWidget(manaBar);
    battlePanelLayout->addSpacing(10);
    battlePanelLayout->addWidget(new QLabel("СПОСОБНОСТИ"));
    battlePanelLayout->addWidget(abilitiesContainer);

    battlePanel->hide();

    finishGameBtn = new QPushButton("РЕЗУЛЬТАТ");
    finishGameBtn->setMinimumHeight(50);
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &GameWindow::onFinishGameClicked);

    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addWidget(randomPlaceBtn);
    centerLayout->addWidget(startBattleBtn);
    centerLayout->addWidget(battlePanel);
    centerLayout->addStretch();
    centerLayout->addWidget(finishGameBtn);

    // --- ПРАВАЯ КОЛОНКА (ВРАГ) ---
    QVBoxLayout *rightLayout = new QVBoxLayout();

    enemyAvatar = new AvatarWidget(false, this);
    enemyAvatar->hide();
    enemyMessage = new MessageBubble(this);

    QVBoxLayout *enemyAvatarLayout = new QVBoxLayout();
    enemyAvatarLayout->addWidget(enemyMessage, 0, Qt::AlignCenter);
    enemyAvatarLayout->addWidget(enemyAvatar, 0, Qt::AlignCenter);

    QLabel *enemyTitle = new QLabel("ПРОТИВНИК");
    enemyTitle->setAlignment(Qt::AlignCenter);
    enemyTitle->setStyleSheet("font-weight: bold; color: #333; font-size: 18px; margin-bottom: 5px;");

    enemyBoard = new BoardWidget(this);
    enemyBoard->setShips(enemyShips);
    enemyBoard->setEnemy(true);
    enemyBoard->setEditable(false);
    enemyBoard->setShowShips(false);
    enemyBoard->setEnabled(false);
    enemyBoard->setupSizePolicy();
    enemyBoard->installEventFilter(this);

    connect(enemyBoard, &BoardWidget::cellClicked, this, &GameWindow::onPlayerBoardClick);
    connect(enemyBoard, &BoardWidget::missileImpact, this, &GameWindow::onMissileImpact);

    rightLayout->addLayout(enemyAvatarLayout);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(enemyTitle);
    rightLayout->addWidget(enemyBoard);
    rightLayout->addStretch(1);

    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addWidget(centerWidget, 0, Qt::AlignTop);
    mainLayout->addLayout(rightLayout, 2);

    globalLayout->addWidget(gameContentWidget);
}

void GameWindow::onRandomPlaceClicked() {
    if (playerBoard->autoPlaceShips()) {
        QList<QLabel*> labels = shipsSetupPanel->findChildren<QLabel*>();
        for(auto label : labels) {
            label->hide();
        }
        playerBoard->update();
    } else {
        QMessageBox::warning(this, "Ошибка", "Не удалось расставить корабли. Попробуйте еще раз.");
    }
}

void GameWindow::onExitToMenuClicked()
{
    emit backToMenu();
    this->close();
}

void GameWindow::updateTurnVisuals() {
    if (isGameOver) return;
    if (isPlayerTurn) {
        playerAvatar->setStyleSheet("border: 2px solid yellow;");
        enemyAvatar->setStyleSheet("border: none;");
        enemyBoard->setActive(true);
        playerBoard->setActive(false);
        // Если активирован кластерный режим, меняем курсор или сообщение
        if (isClusterMode) enemyMessage->showMessage("АВИАУДАР ГОТОВ!");
    } else {
        enemyAvatar->setStyleSheet("border: 2px solid yellow;");
        playerAvatar->setStyleSheet("border: none;");
        enemyBoard->setActive(false);
        playerBoard->setActive(true);
    }
}

void GameWindow::onStartBattleClicked() {
    for(auto s : playerShips) {
        if(!s->isPlaced()) {
            QMessageBox::warning(this, "Внимание", "Расставьте флот!");
            return;
        }
    }

    if(!enemyBoard->autoPlaceShips()) enemyBoard->autoPlaceShips();

    centerWidget->setVisible(false);

    rpsOverlay = new RPSWidget(this);
    connect(rpsOverlay, &RPSWidget::gameFinished, this, &GameWindow::startGameAfterRPS);
    rpsOverlay->show();
}

void GameWindow::startGameAfterRPS(bool playerFirst) {
    rpsOverlay = nullptr;

    centerWidget->setVisible(true);
    shipsSetupPanel->hide();
    startBattleBtn->hide();
    randomPlaceBtn->hide();
    battlePanel->show();

    resetMana();

    infoLabel->setText("БОЙ!");
    infoLabel->setStyleSheet("color: #e74c3c; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");

    playerAvatar->show();
    enemyAvatar->show();

    playerBoard->setEditable(false);
    enemyBoard->setEnabled(true);
    exitToMenuBtn->setText("СДАТЬСЯ");

    isBattleStarted = true;
    isAnimating = false;
    isPlayerTurn = playerFirst;

    if (isPlayerTurn) playerMessage->showMessage("Я НАЧИНАЮ!");
    else enemyMessage->showMessage("МОЙ ХОД!");

    updateTurnVisuals();
    if(!isPlayerTurn) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
}

// --- ЛОГИКА СПОСОБНОСТЕЙ ---

void GameWindow::onAbilityClicked(int type) {
    if (!isPlayerTurn || isAnimating || isGameOver) return;

    if (type == 1 && playerMana >= 60) activateFog();
    else if (type == 2 && playerMana >= 80) activateRadar();
    else if (type == 3 && playerMana >= 100) activateCluster();
}

void GameWindow::activateFog() {
    isFogActive = true;
    addMana(-60);
    playerBoard->setFog(true); // Визуальный туман на поле игрока
    playerMessage->showMessage("ТУМАН!");
}

void GameWindow::activateRadar() {
    // Ищем живой корабль врага
    QVector<QPoint> possibleCells;
    for(int x=0; x<10; ++x) {
        for(int y=0; y<10; ++y) {
            // Клетка с кораблем, которая еще не подбита (не Hit)
            if (enemyBoard->hasShipAt(x, y) && enemyBoard->getCellState(x, y) != Hit) {
                possibleCells.append(QPoint(x, y));
            }
        }
    }

    if (!possibleCells.isEmpty()) {
        int idx = QRandomGenerator::global()->bounded(possibleCells.size());
        radarCell = possibleCells[idx];

        isRadarActive = true;
        addMana(-80);

        enemyBoard->setHighlight(radarCell);
        playerMessage->showMessage("РАДАР: ЦЕЛЬ!");
    } else {
        playerMessage->showMessage("НЕКОГО ИСКАТЬ!");
    }
}

void GameWindow::activateCluster() {
    isClusterMode = true;
    addMana(-100);
    playerMessage->showMessage("КЛАСТЕРНЫЙ УДАР!");
    updateTurnVisuals();
}

void GameWindow::onPlayerBoardClick(int x, int y) {
    if(!isBattleStarted || !isPlayerTurn || isGameOver || isAnimating) return;
    if (!enemyBoard->canShootAt(x, y) && !isClusterMode) return;

    // Сброс подсветки радара, если попали в ту же клетку
    if (isRadarActive && x == radarCell.x() && y == radarCell.y()) {
        isRadarActive = false;
        radarCell = QPoint(-1, -1);
        enemyBoard->setHighlight(QPoint(-1, -1));
    }

    if (isClusterMode) {
        // Запуск серии ударов
        isClusterMode = false; // Режим одноразовый
        isClusterExecuting = true;
        clusterQueue.clear();
        clusterHitsCount = 0;

        // Центр
        clusterQueue.append(QPoint(x, y));
        // Вокруг (8 клеток)
        int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};

        for(int i=0; i<8; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            // Добавляем, даже если там уже стреляли (для красоты анимации),
            // но важно проверять границы
            if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
                clusterQueue.append(QPoint(nx, ny));
            }
        }

        isAnimating = true;
        enemyBoard->setActive(false);
        processClusterShot();
    } else {
        // Обычный выстрел
        isAnimating = true;
        enemyBoard->setActive(false);
        enemyBoard->animateShot(x, y);
    }
}

void GameWindow::processClusterShot() {
    if (clusterQueue.isEmpty()) {
        // Серия закончена
        isClusterExecuting = false;
        isAnimating = false;

        // Если было хотя бы одно попадание - продолжаем ход
        if (clusterHitsCount > 0) {
            playerMessage->showMessage("СЕРИЯ: УСПЕХ!");
            enemyBoard->setActive(true);
            checkGameStatus();
        } else {
            // Иначе промах
            playerMessage->showMessage(getRandomPhrase(missPhrases));
            addMana(20);
            isPlayerTurn = false;
            updateTurnVisuals();
            QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        }
        return;
    }

    QPoint target = clusterQueue.takeFirst();
    // Запускаем анимацию следующего выстрела
    // Даже если там уже стреляли, animateShot отработает, а receiveShot вернет -1
    enemyBoard->animateShot(target.x(), target.y());
}

void GameWindow::onMissileImpact(int x, int y, bool isHit) {
    BoardWidget* targetBoard = qobject_cast<BoardWidget*>(sender());
    if (!targetBoard) return;

    int res = targetBoard->receiveShot(x, y);
    // res: -1 (already), 0 (miss), 1 (hit), 2 (kill)

    if (res > 0) shakeScreen();

    // --- ЛОГИКА ИГРОКА ---
    if (targetBoard == enemyBoard) {

        if (isClusterExecuting) {
            if (res > 0) clusterHitsCount++;
            // Рекурсивный вызов следующего выстрела с небольшой задержкой
            QTimer::singleShot(150, this, &GameWindow::processClusterShot);
            return;
        }

        // Обычный выстрел
        isAnimating = false;

        if (res == 0) { // Промах
            playerMessage->showMessage(getRandomPhrase(missPhrases));
            addMana(20);
            isPlayerTurn = false;
            updateTurnVisuals();
            QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        } else if (res > 0) { // Попал
            resetMana();
            if (res == 1) playerMessage->showMessage(getRandomPhrase(hitPhrases));
            if (res == 2) playerMessage->showMessage(getRandomPhrase(killPhrases));
            checkGameStatus();
            if (!isGameOver) enemyBoard->setActive(true);
        } else {
            // Если выстрел в уже простреленную клетку (по ошибке), просто разблокируем
            if (!isGameOver) enemyBoard->setActive(true);
        }
    }
    // --- ЛОГИКА БОТА ---
    else if (targetBoard == playerBoard) {
        if (res == 0 || res == -1) { // Промах (или удар в уже битую клетку из-за тумана)
            enemyMessage->showMessage(getRandomPhrase(missPhrases));

            // Если это был ход бота и активен туман -> туман спадает
            if (isFogActive) {
                isFogActive = false;
                playerBoard->setFog(false);
                playerMessage->showMessage("ТУМАН РАССЕЯЛСЯ");
            }

            isPlayerTurn = true;
            updateTurnVisuals();
        } else if (res > 0) { // Попал
            if (res == 1) enemyMessage->showMessage(getRandomPhrase(hitPhrases));
            if (res == 2) {
                enemyMessage->showMessage(getRandomPhrase(killPhrases));
                enemyTargetQueue.clear();
                shipHitPoints.clear();
            } else {
                if (res == 1) {
                    shipHitPoints.append(QPoint(x, y));
                    if (shipHitPoints.size() == 1) addInitialTargets(x, y);
                    else determineNextTargetLine();
                }
            }
            checkGameStatus();
            if(!isGameOver) QTimer::singleShot(1500, this, &GameWindow::enemyTurn);
        }
    }
}

void GameWindow::addMana(int amount) {
    playerMana += amount;
    if (playerMana > 100) playerMana = 100;
    manaBar->setMana(playerMana);
    updateAbilitiesState();
}

void GameWindow::resetMana() {
    playerMana = 0;
    manaBar->setMana(playerMana);
    updateAbilitiesState();
}

void GameWindow::updateAbilitiesState() {
    ability1->setAvailable(playerMana >= ability1->getCost());
    ability2->setAvailable(playerMana >= ability2->getCost());
    ability3->setAvailable(playerMana >= ability3->getCost());
}

void GameWindow::enemyTurn() {
    if(isPlayerTurn || !isBattleStarted || isGameOver) return;

    int x, y;
    bool valid = false;
    int attempts = 0;

    // --- ЛОГИКА ТУМАНА ---
    if (isFogActive) {
        // Бот стреляет абсолютно случайно, может попасть в уже битую клетку
        // Он "забыл" карту
        x = QRandomGenerator::global()->bounded(10);
        y = QRandomGenerator::global()->bounded(10);
        // Мы НЕ проверяем canShootAt, так как он не видит старых выстрелов
        valid = true;
    }
    // --- ОБЫЧНАЯ ЛОГИКА ---
    else {
        while (!valid && attempts < 100) {
            attempts++;
            if (!enemyTargetQueue.isEmpty()) {
                QPoint target = enemyTargetQueue.first();
                x = target.x(); y = target.y();
                if (x < 0 || x > 9 || y < 0 || y > 9 || !playerBoard->canShootAt(x, y)) {
                    enemyTargetQueue.removeFirst();
                    continue;
                }
            } else {
                x = QRandomGenerator::global()->bounded(10);
                y = QRandomGenerator::global()->bounded(10);
            }

            if (playerBoard->canShootAt(x, y)) {
                valid = true;
                if (!enemyTargetQueue.isEmpty()) enemyTargetQueue.removeFirst();
            }
        }
    }

    if (valid) {
        playerBoard->animateShot(x, y);
    } else {
        // Если совсем некуда стрелять (редкий случай конца игры), передаем ход
        isPlayerTurn = true;
        updateTurnVisuals();
    }
}

void GameWindow::shakeScreen() {
    if (shakeFrames > 0) return;
    originalPos = this->pos();
    shakeFrames = 10;
    shakeTimer->start();
}

void GameWindow::updateShake() {
    if (shakeFrames > 0) {
        int dx = QRandomGenerator::global()->bounded(10) - 5;
        int dy = QRandomGenerator::global()->bounded(10) - 5;
        this->move(originalPos + QPoint(dx, dy));
        shakeFrames--;
    } else {
        this->move(originalPos);
        shakeTimer->stop();
    }
}

void GameWindow::checkGameStatus() {
    if (enemyBoard->isAllDestroyed()) endGame(true);
    else if (playerBoard->isAllDestroyed()) endGame(false);
}

void GameWindow::endGame(bool playerWon) {
    isGameOver = true;
    isBattleStarted = false;
    enemyBoard->setShowShips(true);
    enemyBoard->update();
    enemyBoard->setEnabled(false);
    exitToMenuBtn->setText("В МЕНЮ");

    shipsSetupPanel->hide();
    startBattleBtn->hide();
    randomPlaceBtn->hide();
    battlePanel->hide();

    infoLabel->show();

    finishGameBtn->show();
    centerWidget->setVisible(true);

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!");
        infoLabel->setStyleSheet("color: #27ae60; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
        playerMessage->showMessage("ПОБЕДА!");
        enemyMessage->showMessage("НЕЕЕЕТ!");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ");
        infoLabel->setStyleSheet("color: #c0392b; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
        playerMessage->showMessage("КАК ТАК?!");
        enemyMessage->showMessage("ЛЕГКО!");
    }
}

void GameWindow::onFinishGameClicked() {
    emit backToMenu();
    this->close();
}
