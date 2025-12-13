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

// --- Реализация AvatarWidget ---
AvatarWidget::AvatarWidget(bool isPlayer, QWidget *parent)
    : QWidget(parent), isPlayer(isPlayer)
{
    setFixedSize(80, 80);
}

void AvatarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    int w = width();
    int h = height();
    int s = w / 8;

    // Рамка
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(230, 220, 210));
    p.drawRect(0, 0, w, h);

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

GameWindow::GameWindow(QWidget *parent) : QWidget(parent), isBattleStarted(false), isGameOver(false), isAnimating(false)
{
    setWindowTitle("Морской Бой");
    resize(1000, 750);
    setMouseTracking(true);
    this->installEventFilter(this); // Ловим мышь глобально для параллакса

    // Таймер тряски
    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);
    connect(shakeTimer, &QTimer::timeout, this, &GameWindow::updateShake);

    initShips();

    hitPhrases << "БАБАХ!" << "ПОЛУЧИ!" << "В ЯБЛОЧКО!" << "ЕСТЬ ПРОБИТИЕ!" << "ХА-ХА!";
    killPhrases << "НА ДНО!" << "БУЛЬ-БУЛЬ!" << "КОРМ ДЛЯ РЫБ!" << "МИНУС ОДИН!" << "ПРОЩАЙ!";
    missPhrases << "УПС..." << "МИМО!" << "МАЗИЛА!" << "В МОЛОКО" << "ЭХ...";

    setupUI();
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

    // СИГНАЛ УДАРА ОТ БОТА
    connect(playerBoard, &BoardWidget::missileImpact, this, &GameWindow::onMissileImpact);

    leftLayout->addLayout(playerAvatarLayout);
    leftLayout->addSpacing(10);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(1);

    // --- ЦЕНТРАЛЬНАЯ КОЛОНКА (МАГАЗИН) ---
    centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    centerWidget->installEventFilter(this);
    centerWidget->setMouseTracking(true);
    centerWidget->setStyleSheet("background-color: rgba(255, 250, 240, 200); border: 2px dashed #aaa; border-radius: 5px;");
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(15, 15, 15, 15);

    infoLabel = new QLabel("РАССТАВЬТЕ\nКОРАБЛИ");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setMinimumHeight(60);
    infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; border-bottom: 2px solid #ccc; padding-bottom: 10px; font-family: 'Courier New';");

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

    finishGameBtn = new QPushButton("РЕЗУЛЬТАТ");
    finishGameBtn->setMinimumHeight(50);
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &GameWindow::onFinishGameClicked);

    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addStretch();
    centerLayout->addWidget(randomPlaceBtn);
    centerLayout->addWidget(startBattleBtn);
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

    // --- ИСПРАВЛЕНИЕ: Гарантируем подключение сигналов ---
    // 1. Клик по доске вызывает проверку и старт анимации
    connect(enemyBoard, &BoardWidget::cellClicked, this, &GameWindow::onPlayerBoardClick);
    // 2. Окончание анимации вызывает расчет попадания
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

    // СКРЫВАЕМ ЦЕНТРАЛЬНУЮ ПАНЕЛЬ
    centerWidget->setVisible(false);

    playerAvatar->show();
    enemyAvatar->show();

    playerBoard->setEditable(false);
    // ВАЖНО: Включаем доску противника, чтобы она могла принимать клики
    enemyBoard->setEnabled(true);
    exitToMenuBtn->setText("СДАТЬСЯ");

    isBattleStarted = true;
    isAnimating = false; // Сбрасываем флаг анимации, чтобы разрешить ввод
    isPlayerTurn = (QRandomGenerator::global()->bounded(2) == 0);

    if (isPlayerTurn) playerMessage->showMessage("Я НАЧИНАЮ!");
    else enemyMessage->showMessage("МОЙ ХОД!");

    updateTurnVisuals();
    if(!isPlayerTurn) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
}

// Клик игрока: ЗАПУСКАЕМ АНИМАЦИЮ
void GameWindow::onPlayerBoardClick(int x, int y) {
    // Проверка: бой идет, ход игрока, игра не окончена, анимация не идет
    if(!isBattleStarted || !isPlayerTurn || isGameOver || isAnimating) return;

    // Проверка: можно ли стрелять в эту клетку
    if (!enemyBoard->canShootAt(x, y)) return;

    isAnimating = true; // Блокируем ввод
    enemyBoard->setActive(false); // Визуально гасим
    enemyBoard->animateShot(x, y); // Старт анимации
}

// Обработка попадания (после анимации)
void GameWindow::onMissileImpact(int x, int y, bool isHit) {
    BoardWidget* targetBoard = qobject_cast<BoardWidget*>(sender());
    if (!targetBoard) return;

    int res = targetBoard->receiveShot(x, y);
    isAnimating = false; // Разблокируем ввод

    if (res > 0) shakeScreen();

    if (targetBoard == enemyBoard) { // Игрок стрелял (попал в enemyBoard)
        if (res == 0) { // Промах
            playerMessage->showMessage(getRandomPhrase(missPhrases));
            isPlayerTurn = false;
            updateTurnVisuals();
            QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        } else if (res > 0) { // Попал или убил
            if (res == 1) playerMessage->showMessage(getRandomPhrase(hitPhrases));
            if (res == 2) playerMessage->showMessage(getRandomPhrase(killPhrases));
            checkGameStatus();
            if (!isGameOver) enemyBoard->setActive(true); // Возвращаем активность для следующего выстрела
        }
    }
    else if (targetBoard == playerBoard) { // Бот стрелял (попал в playerBoard)
        if (res == 0) { // Промах
            enemyMessage->showMessage(getRandomPhrase(missPhrases));
            isPlayerTurn = true;
            updateTurnVisuals();
        } else if (res > 0) { // Попал или убил
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

void GameWindow::enemyTurn() {
    if(isPlayerTurn || !isBattleStarted || isGameOver) return;

    int x, y;
    bool valid = false;
    int attempts = 0;

    // Пытаемся найти валидную цель
    while (!valid && attempts < 100) {
        attempts++;
        if (!enemyTargetQueue.isEmpty()) {
            QPoint target = enemyTargetQueue.first();
            x = target.x(); y = target.y();
            if (x < 0 || x > 9 || y < 0 || y > 9 || !playerBoard->canShootAt(x, y)) {
                enemyTargetQueue.removeFirst(); // Удаляем невалидную цель
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

    if (valid) {
        playerBoard->animateShot(x, y);
    } else {
        // Если совсем не нашли (редкий случай), пропускаем ход или пробуем снова
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

    // Скрываем ненужные элементы панели
    shipsSetupPanel->hide();
    startBattleBtn->hide();
    randomPlaceBtn->hide();
    infoLabel->show(); // Убеждаемся, что лейбл виден

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
