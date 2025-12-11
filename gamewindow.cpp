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

// --- Виджет корабля в магазине (ТЕПЕРЬ ГРАФИЧЕСКИЙ) ---
class DraggableShipLabel : public QLabel {
public:
    int shipId, size;
    DraggableShipLabel(int id, int s, QWidget* p=nullptr) : QLabel(p), shipId(id), size(s) {
        // Убираем текст, задаем размер
        // Ширина зависит от размера корабля (30px на клетку) + небольшой отступ
        setFixedSize(s * 30 + 10, 40);
        setCursor(Qt::OpenHandCursor);
        // Прозрачный фон для виджета, чтобы было видно только корабль
        setStyleSheet("background-color: transparent;");
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // Рисуем корабль горизонтально в центре виджета
        // Используем статический метод из BoardWidget для единого стиля
        // Смещаем на (5, 5) чтобы был отступ от края виджета
        QRect shipRect(5, 5, size * 30, 30);
        BoardWidget::drawShipShape(p, size, Orientation::Horizontal, shipRect, false, false);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();
            mimeData->setText(QString("%1:%2").arg(shipId).arg(0));
            drag->setMimeData(mimeData);

            // Генерируем pixmap корабля для перетаскивания
            QPixmap pixmap(size * 30, 30);
            pixmap.fill(Qt::transparent);
            QPainter p(&pixmap);
            BoardWidget::drawShipShape(p, size, Orientation::Horizontal, QRect(0,0,size*30,30), false, false);
            p.end();

            drag->setPixmap(pixmap);
            drag->setHotSpot(QPoint(15, 15)); // Хватаем за "нос"

            if (drag->exec(Qt::MoveAction) == Qt::MoveAction) this->hide();
        }
    }
};

GameWindow::GameWindow(QWidget *parent) : QWidget(parent), isBattleStarted(false), isGameOver(false)
{
    setWindowTitle("Морской Бой");
    resize(1000, 700);
    initShips();
    setupUI();
}

GameWindow::~GameWindow() {
    qDeleteAll(playerShips);
    qDeleteAll(enemyShips);
}

// --- ОТРИСОВКА ФОНА ИГРОВОГО ОКНА ---
void GameWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    // Морской градиент
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0.0, QColor(20, 40, 60));    // Темное небо/море сверху
    gradient.setColorAt(1.0, QColor(0, 80, 120));    // Глубокое море снизу
    p.fillRect(rect(), gradient);

    // Сетка радара (декорация на фоне)
    p.setPen(QPen(QColor(255, 255, 255, 10), 1));
    for (int i = 0; i < width(); i += 40) p.drawLine(i, 0, i, height());
    for (int i = 0; i < height(); i += 40) p.drawLine(0, i, width(), i);
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
        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
            enemyTargetQueue.append(QPoint(nx, ny));
        }
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
    headerWidget->setStyleSheet("background-color: rgba(0, 0, 0, 150); border-bottom: 2px solid #555;");
    headerWidget->setFixedHeight(60);

    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *gameTitle = new QLabel("Одиночная игра", this);
    gameTitle->setStyleSheet("color: white; font-weight: bold; font-size: 18px;");

    exitToMenuBtn = new QPushButton("Выход в меню", this);
    exitToMenuBtn->setCursor(Qt::PointingHandCursor);
    exitToMenuBtn->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; border-radius: 4px; padding: 6px 15px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e74c3c; }"
        );
    connect(exitToMenuBtn, &QPushButton::clicked, this, &GameWindow::onExitToMenuClicked);

    headerLayout->addWidget(gameTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(exitToMenuBtn);

    globalLayout->addWidget(headerWidget);


    // Game Content
    QWidget *gameContentWidget = new QWidget(this);
    gameContentWidget->setStyleSheet("background: transparent;"); // Прозрачный, чтобы видеть paintEvent окна
    QHBoxLayout *mainLayout = new QHBoxLayout(gameContentWidget);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(20);

    // Player Board
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *playerTitle = new QLabel("ВАШ ФЛОТ");
    playerTitle->setAlignment(Qt::AlignCenter);
    playerTitle->setStyleSheet("font-weight: bold; color: white; font-size: 16px; margin-bottom: 5px; background-color: rgba(0,0,0,100); padding: 5px; border-radius: 5px;");

    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true);
    playerBoard->setShowShips(true);
    playerBoard->setupSizePolicy();

    leftLayout->addStretch(1);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(1);

    // Center Panel (Shop & Info)
    QWidget *centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    centerWidget->setStyleSheet("background-color: rgba(255, 255, 255, 220); border-radius: 10px;"); // Полупрозрачная панель
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(15, 15, 15, 15);

    infoLabel = new QLabel("Перетащите корабли\nна своё поле");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setMinimumHeight(60);
    infoLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; border-bottom: 1px solid #ccc; padding-bottom: 10px;");

    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsLayout = new QVBoxLayout(shipsSetupPanel);
    shipsLayout->setSpacing(5);
    // Добавляем скролл, если кораблей много, но у нас влезет
    for (Ship* s : playerShips) {
        DraggableShipLabel *shipLabel = new DraggableShipLabel(s->id, s->size);
        shipsLayout->addWidget(shipLabel, 0, Qt::AlignCenter);
    }

    startBattleBtn = new QPushButton("В БОЙ!");
    startBattleBtn->setMinimumHeight(50);
    startBattleBtn->setCursor(Qt::PointingHandCursor);
    startBattleBtn->setStyleSheet("QPushButton { background-color: #27ae60; color: white; font-size: 18px; border-radius: 8px; font-weight: bold; } QPushButton:hover { background-color: #2ecc71; }");
    connect(startBattleBtn, &QPushButton::clicked, this, &GameWindow::onStartBattleClicked);

    finishGameBtn = new QPushButton("Результат");
    finishGameBtn->setMinimumHeight(50);
    finishGameBtn->setStyleSheet("background-color: #2980b9; color: white; font-size: 16px; border-radius: 8px; font-weight: bold;");
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &GameWindow::onFinishGameClicked);

    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addStretch(); // Пружина между магазином и кнопкой
    centerLayout->addWidget(startBattleBtn);
    centerLayout->addWidget(finishGameBtn);

    // Enemy Board
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *enemyTitle = new QLabel("РАДАР ПРОТИВНИКА");
    enemyTitle->setAlignment(Qt::AlignCenter);
    enemyTitle->setStyleSheet("font-weight: bold; color: white; font-size: 16px; margin-bottom: 5px; background-color: rgba(150,0,0,100); padding: 5px; border-radius: 5px;");

    enemyBoard = new BoardWidget(this);
    enemyBoard->setShips(enemyShips);
    enemyBoard->setEnemy(true); // Включаем темный стиль для врага
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
        infoLabel->setText("ВАШ ХОД!\nСтреляйте по радару");
        infoLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2980b9; border-bottom: 1px solid #ccc;");
        enemyBoard->setActive(true);
        playerBoard->setActive(false);
    } else {
        infoLabel->setText("ХОД ПРОТИВНИКА...\nОжидайте");
        infoLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #c0392b; border-bottom: 1px solid #ccc;");
        enemyBoard->setActive(false);
        playerBoard->setActive(true);
    }
}

void GameWindow::onStartBattleClicked() {
    for(auto s : playerShips) {
        if(!s->isPlaced()) {
            QMessageBox::warning(this, "Внимание", "Сначала расставьте все корабли на поле!");
            return;
        }
    }

    if(!enemyBoard->autoPlaceShips()) enemyBoard->autoPlaceShips();

    shipsSetupPanel->hide();
    startBattleBtn->hide();
    playerBoard->setEditable(false);
    enemyBoard->setEnabled(true);
    exitToMenuBtn->setText("Сдаться");

    isBattleStarted = true;
    isPlayerTurn = (QRandomGenerator::global()->bounded(2) == 0);

    updateTurnVisuals();
    if(!isPlayerTurn) {
        QTimer::singleShot(800, this, &GameWindow::enemyTurn);
    }
}

void GameWindow::onPlayerBoardClick(int x, int y) {
    if(!isBattleStarted || !isPlayerTurn || isGameOver) return;
    int res = enemyBoard->receiveShot(x, y);
    if (res == -1) return;
    if (res == 0) {
        isPlayerTurn = false;
        updateTurnVisuals();
        QTimer::singleShot(800, this, &GameWindow::enemyTurn);
    } else {
        checkGameStatus();
    }
}

void GameWindow::enemyTurn() {
    if(isPlayerTurn || !isBattleStarted || isGameOver) return;

    int x, y;
    int res = -1;

    while (res == -1) {
        if (!enemyTargetQueue.isEmpty()) {
            QPoint target = enemyTargetQueue.takeFirst();
            x = target.x();
            y = target.y();
            if (x < 0 || x > 9 || y < 0 || y > 9) continue;
        }
        else {
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

    if (res == 0) {
        // infoLabel обновляется в updateTurnVisuals
    }
}

void GameWindow::checkGameStatus() {
    if (enemyBoard->isAllDestroyed()) {
        endGame(true);
    } else if (playerBoard->isAllDestroyed()) {
        endGame(false);
    }
}

void GameWindow::endGame(bool playerWon) {
    isGameOver = true;
    isBattleStarted = false;
    enemyBoard->setShowShips(true);
    enemyBoard->update();
    enemyBoard->setEnabled(false);
    exitToMenuBtn->setText("В меню");

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!\nВы уничтожили весь флот!");
        infoLabel->setStyleSheet("color: green; font-size: 20px; font-weight: bold; padding: 10px;");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ.\nВаш флот разбит.");
        infoLabel->setStyleSheet("color: red; font-size: 20px; font-weight: bold; padding: 10px;");
    }

    finishGameBtn->show();
}

void GameWindow::onFinishGameClicked() {
    emit backToMenu();
    this->close();
}
