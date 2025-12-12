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

// --- DraggableShipLabel оставляем без изменений, но используем цвета из BoardWidget ---
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

GameWindow::GameWindow(QWidget *parent) : QWidget(parent), isBattleStarted(false), isGameOver(false)
{
    setWindowTitle("Морской Бой");
    resize(1000, 700);

    // Включаем трекинг мыши для всего окна
    setMouseTracking(true);

    initShips();
    setupUI();
}

GameWindow::~GameWindow() {
    qDeleteAll(playerShips);
    qDeleteAll(enemyShips);
}

void GameWindow::mouseMoveEvent(QMouseEvent *event) {
    mousePos = event->pos();
    update(); // Перерисовка фона при движении
}

// --- НОВЫЙ ФОН: БУМАГА + ВОЛНЫ ---
void GameWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // 1. Фон (Бумага)
    p.fillRect(rect(), QColor(248, 240, 227));

    int w = width();
    int h = height();
    int pixelSize = 4;

    // Смещение "на резинке"
    int shiftX = (mousePos.x() - w/2) * 0.03; // Чуть меньше амплитуда чем в меню
    int shiftY = (mousePos.y() - h/2) * 0.03;

    // 2. Декоративные кораблики (чуть более темные чем бумага)
    auto drawPixelShip = [&](int x, int y, int size) {
        p.setBrush(QColor(180, 170, 160));
        p.setPen(Qt::NoPen);
        int s = size;
        int px = x + shiftX * 0.5; // Параллакс
        int py = y + shiftY * 0.5;

        p.drawRect(px, py, s*6, s);
        p.drawRect(px + s, py-s, s*4, s);
        p.drawRect(px + s*2, py-s*2, s, s);
    };

    drawPixelShip(100, 100, 4);
    drawPixelShip(w - 150, h - 100, 5);
    drawPixelShip(w/2 - 50, h - 50, 3);

    // 3. Пиксельные волны (Серые)
    p.setBrush(QColor(160, 160, 160));
    for (int y = 0; y < h; y += 50) {
        int rowShift = shiftX * (float(y)/h);
        // Рисуем пунктир
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
    // Делаем хедер чуть прозрачным коричневым, чтобы сочетался с бежевым
    headerWidget->setStyleSheet("background-color: rgba(60, 50, 40, 200); border-bottom: 2px solid #555;");
    headerWidget->setFixedHeight(60);

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *gameTitle = new QLabel("БОЙ", this); // Сократил для стиля
    gameTitle->setStyleSheet("color: #f0e6d2; font-weight: bold; font-size: 24px; font-family: 'Courier New';");

    exitToMenuBtn = new QPushButton("ВЫХОД", this);
    exitToMenuBtn->setCursor(Qt::PointingHandCursor);
    // Стиль кнопки переопределен в QSS, здесь задаем только уникальные цвета если надо
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
    gameContentWidget->setMouseTracking(true); // Важно для фона
    gameContentWidget->setStyleSheet("background: transparent;");

    QHBoxLayout *mainLayout = new QHBoxLayout(gameContentWidget);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(20);

    // Player Board
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *playerTitle = new QLabel("ВАШ ФЛОТ");
    playerTitle->setAlignment(Qt::AlignCenter);
    playerTitle->setStyleSheet("font-weight: bold; color: #333; font-size: 18px; margin-bottom: 5px;");

    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true);
    playerBoard->setShowShips(true);
    playerBoard->setupSizePolicy();

    leftLayout->addStretch(1);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(1);

    // Center Panel
    QWidget *centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    // Полупрозрачный "лист бумаги" для панели
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
    centerLayout->addWidget(startBattleBtn);
    centerLayout->addWidget(finishGameBtn);

    // Enemy Board
    QVBoxLayout *rightLayout = new QVBoxLayout();
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
    connect(enemyBoard, &BoardWidget::cellClicked, this, &GameWindow::onPlayerBoardClick);

    rightLayout->addStretch(1);
    rightLayout->addWidget(enemyTitle);
    rightLayout->addWidget(enemyBoard);
    rightLayout->addStretch(1);

    mainLayout->addLayout(leftLayout, 2);
    mainLayout->addWidget(centerWidget, 0);
    mainLayout->addLayout(rightLayout, 2);

    globalLayout->addWidget(gameContentWidget);
}

void GameWindow::onExitToMenuClicked()
{
    emit backToMenu();
    this->close();
}

void GameWindow::updateTurnVisuals() {
    if (isGameOver) return;
    if (isPlayerTurn) {
        infoLabel->setText("ВАШ ХОД");
        infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2980b9; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
        enemyBoard->setActive(true);
        playerBoard->setActive(false);
    } else {
        infoLabel->setText("ЖДИТЕ...");
        infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #c0392b; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
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

    shipsSetupPanel->hide();
    startBattleBtn->hide();
    playerBoard->setEditable(false);
    enemyBoard->setEnabled(true);
    exitToMenuBtn->setText("СДАТЬСЯ");

    isBattleStarted = true;
    isPlayerTurn = (QRandomGenerator::global()->bounded(2) == 0);
    updateTurnVisuals();
    if(!isPlayerTurn) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
}

void GameWindow::onPlayerBoardClick(int x, int y) {
    if(!isBattleStarted || !isPlayerTurn || isGameOver) return;
    int res = enemyBoard->receiveShot(x, y);
    if (res == -1) return;
    if (res == 0) {
        isPlayerTurn = false;
        updateTurnVisuals();
        QTimer::singleShot(800, this, &GameWindow::enemyTurn);
    } else checkGameStatus();
}

void GameWindow::enemyTurn() {
    if(isPlayerTurn || !isBattleStarted || isGameOver) return;
    int x, y, res = -1;
    while (res == -1) {
        if (!enemyTargetQueue.isEmpty()) {
            QPoint target = enemyTargetQueue.takeFirst();
            x = target.x(); y = target.y();
            if (x < 0 || x > 9 || y < 0 || y > 9) continue;
        } else {
            x = QRandomGenerator::global()->bounded(10);
            y = QRandomGenerator::global()->bounded(10);
        }
        res = playerBoard->receiveShot(x, y);
        if (res == 0) {
            isPlayerTurn = true;
            updateTurnVisuals();
        } else if (res > 0) {
            if (res == 1) {
                shipHitPoints.append(QPoint(x, y));
                if (shipHitPoints.size() == 1) addInitialTargets(x, y);
                else determineNextTargetLine();
            } else if (res == 2) {
                enemyTargetQueue.clear();
                shipHitPoints.clear();
            }
            checkGameStatus();
            if(!isGameOver) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
            break;
        }
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
    if (playerWon) {
        infoLabel->setText("ПОБЕДА!");
        infoLabel->setStyleSheet("color: #27ae60; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ");
        infoLabel->setStyleSheet("color: #c0392b; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
    }
    finishGameBtn->show();
}

void GameWindow::onFinishGameClicked() {
    emit backToMenu();
    this->close();
}
