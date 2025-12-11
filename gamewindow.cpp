#include "gamewindow.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QTimer>
#include <QRandomGenerator>

// --- Виджет корабля в магазине ---
class DraggableShipLabel : public QLabel {
public:
    int shipId, size;
    DraggableShipLabel(int id, int s, QWidget* p=nullptr) : QLabel(p), shipId(id), size(s) {
        setText(QString("%1-палубный").arg(size));
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        setAlignment(Qt::AlignCenter);
        setStyleSheet("background-color: #e0e0e0; border: 1px solid #999; border-radius: 3px; font-weight: bold;");
        setFixedHeight(35);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();
            mimeData->setText(QString("%1:%2").arg(shipId).arg(0));
            drag->setMimeData(mimeData);

            // Визуализация перетаскивания из магазина
            QPixmap pixmap = this->grab();
            drag->setPixmap(pixmap);
            drag->setHotSpot(event->pos());

            if (drag->exec(Qt::MoveAction) == Qt::MoveAction) this->hide();
        }
    }
};

GameWindow::GameWindow(QWidget *parent) : QWidget(parent), isBattleStarted(false), isGameOver(false)
{
    setWindowTitle("Морской Бой");
    resize(880, 600); // Оптимальный стартовый размер
    initShips();
    setupUI();
}

GameWindow::~GameWindow() {
    qDeleteAll(playerShips);
    qDeleteAll(enemyShips);
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
    // Возможные сдвиги: влево, вправо, вверх, вниз
    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    // Чтобы бот стрелял более "умно", можно добавлять в начало списка (prepend),
    // чтобы он добивал текущую область, а не переключался на старые цели.
    for(int i=0; i<4; ++i) {
        int nx = x + dx[i];
        int ny = y + dy[i];

        // Проверяем границы поля (0-9)
        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
            // Добавляем в список целей
            enemyTargetQueue.append(QPoint(nx, ny));
        }
    }
}

void GameWindow::determineNextTargetLine() {
    if (shipHitPoints.size() < 2) return;

    // Берем первую и последнюю точку для определения ориентации
    QPoint p1 = shipHitPoints.first();
    QPoint p2 = shipHitPoints.last();

    bool isHorizontal = (p1.y() == p2.y());
    bool isVertical = (p1.x() == p2.x());

    // Корабль не может быть одновременно H и V (если только это не 1-палубный, но size >= 2)
    // В теории, если мы попали в две диагональные клетки 4-палубника, то isHorizontal и isVertical будут false.
    // Однако, бот всегда попадает по соседним клеткам, пока не найдет линию.

    if (isHorizontal) {
        // Убеждаемся, что все точки лежат на одной горизонтали
        int y = p1.y();

        // Находим крайние X координаты среди пораженных частей
        int minX = 10, maxX = -1;
        for(const auto& p : shipHitPoints) {
            if (p.x() < minX) minX = p.x();
            if (p.x() > maxX) maxX = p.x();
        }

        // Очищаем старые, случайные цели и добавляем только те, что по линии
        enemyTargetQueue.clear();

        // Цель слева
        enemyTargetQueue.append(QPoint(minX - 1, y));
        // Цель справа
        enemyTargetQueue.append(QPoint(maxX + 1, y));

    } else if (isVertical) {
        // Убеждаемся, что все точки лежат на одной вертикали
        int x = p1.x();

        // Находим крайние Y координаты среди пораженных частей
        int minY = 10, maxY = -1;
        for(const auto& p : shipHitPoints) {
            if (p.y() < minY) minY = p.y();
            if (p.y() > maxY) maxY = p.y();
        }

        // Очищаем старые, случайные цели
        enemyTargetQueue.clear();

        // Цель сверху
        enemyTargetQueue.append(QPoint(x, minY - 1));
        // Цель снизу
        enemyTargetQueue.append(QPoint(x, maxY + 1));
    }
    // Если корабли имеют форму "Г", то тут будет баг. Но в "Морском бое" они прямые,
    // поэтому логика isHorizontal / isVertical будет работать, как только найдутся 2 точки по одной линии.
}

void GameWindow::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);

    // --- ЛЕВАЯ КОЛОНКА (ИГРОК) ---
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QLabel *playerTitle = new QLabel("ВАШ ФЛОТ");
    playerTitle->setAlignment(Qt::AlignCenter);
    playerTitle->setStyleSheet("font-weight: bold; color: #333;");

    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true);
    playerBoard->setShowShips(true);
    playerBoard->setupSizePolicy();

    // Смещение чуть ниже центра: сверху stretch больше, чем снизу
    leftLayout->addStretch(3);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(2); // Меньший коэффициент толкает виджеты вниз

    // --- ЦЕНТРАЛЬНАЯ КОЛОНКА (ИНФО) ---
    // Создаем фиксированный контейнер, чтобы ширина не менялась от текста
    QWidget *centerWidget = new QWidget();
    centerWidget->setFixedWidth(220); // Фиксированная ширина центра
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);

    infoLabel = new QLabel("Расставьте корабли\n(Перетащите их)");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setMinimumHeight(80); // Чтобы 2-3 строки текста не меняли высоту
    infoLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50;");

    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsLayout = new QVBoxLayout(shipsSetupPanel);
    for (Ship* s : playerShips) {
        DraggableShipLabel *shipLabel = new DraggableShipLabel(s->id, s->size);
        shipsLayout->addWidget(shipLabel);
    }

    startBattleBtn = new QPushButton("В БОЙ!");
    startBattleBtn->setMinimumHeight(50);
    startBattleBtn->setStyleSheet("background-color: #4CAF50; color: white; font-size: 16px; border-radius: 5px;");
    connect(startBattleBtn, &QPushButton::clicked, this, &GameWindow::onStartBattleClicked);

    finishGameBtn = new QPushButton("В меню");
    finishGameBtn->setMinimumHeight(50);
    finishGameBtn->setStyleSheet("background-color: #f44336; color: white; font-size: 16px; border-radius: 5px;");
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &GameWindow::onFinishGameClicked);

    centerLayout->addStretch(1);
    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addWidget(startBattleBtn);
    centerLayout->addWidget(finishGameBtn);
    centerLayout->addStretch(1);

    // --- ПРАВАЯ КОЛОНКА (ВРАГ) ---
    QVBoxLayout *rightLayout = new QVBoxLayout();
    QLabel *enemyTitle = new QLabel("РАДАР ПРОТИВНИКА");
    enemyTitle->setAlignment(Qt::AlignCenter);
    enemyTitle->setStyleSheet("font-weight: bold; color: #333;");

    enemyBoard = new BoardWidget(this);
    enemyBoard->setShips(enemyShips);
    enemyBoard->setEditable(false);
    enemyBoard->setShowShips(false);
    enemyBoard->setEnabled(false);
    enemyBoard->setupSizePolicy();
    connect(enemyBoard, &BoardWidget::cellClicked, this, &GameWindow::onPlayerBoardClick);

    rightLayout->addStretch(3);
    rightLayout->addWidget(enemyTitle);
    rightLayout->addWidget(enemyBoard);
    rightLayout->addStretch(2);

    // Добавляем всё в главный слой с пропорциями
    mainLayout->addLayout(leftLayout, 2);   // 40% ширины
    mainLayout->addWidget(centerWidget, 0); // Фиксированно (из-за setFixedWidth)
    mainLayout->addLayout(rightLayout, 2);  // 40% ширины
}

void GameWindow::updateTurnVisuals() {
    if (isGameOver) return;
    if (isPlayerTurn) {
        infoLabel->setText("ВАШ ХОД!");
        enemyBoard->setActive(true); // Выделяем поле, куда НАДО стрелять
        playerBoard->setActive(false);
    } else {
        infoLabel->setText("ХОД ПРОТИВНИКА...");
        enemyBoard->setActive(false);
        playerBoard->setActive(true);
    }
}

void GameWindow::onStartBattleClicked() {
    // Проверка: все ли корабли на поле?
    for(auto s : playerShips) {
        if(!s->isPlaced()) {
            QMessageBox::warning(this, "Внимание", "Сначала расставьте все корабли на поле!");
            return;
        }
        updateTurnVisuals();
    }

    // Расстановка врага
    if(!enemyBoard->autoPlaceShips()) enemyBoard->autoPlaceShips();

    shipsSetupPanel->hide();
    startBattleBtn->hide();
    playerBoard->setEditable(false);
    enemyBoard->setEnabled(true);

    isBattleStarted = true;
    isPlayerTurn = (QRandomGenerator::global()->bounded(2) == 0);

    if(isPlayerTurn) infoLabel->setText("ВАШ ХОД!");
    else {
        infoLabel->setText("Ход противника...");
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

    // Цикл для гарантированного выстрела:
    // если цель из очереди уже была обстреляна (результат -1), берем следующую.
    while (res == -1) {
        if (!enemyTargetQueue.isEmpty()) {
            // 1. Берем цель из очереди (для добивания)
            QPoint target = enemyTargetQueue.takeFirst();
            x = target.x();
            y = target.y();

            // Пропускаем цели, вышедшие за пределы поля.
            if (x < 0 || x > 9 || y < 0 || y > 9) {
                continue;
            }
        }
        else {
            // 2. Если очередь пуста — стреляем случайно (ищем новый корабль)
            x = QRandomGenerator::global()->bounded(10);
            y = QRandomGenerator::global()->bounded(10);
        }

        // Делаем выстрел. playerBoard->receiveShot вернет -1, если клетка уже обстреляна.
        res = playerBoard->receiveShot(x, y);

        // Если res == -1, цикл повторяется: берется следующая цель из очереди, или генерируется новая случайная.
        if (res == -1 && enemyTargetQueue.isEmpty()) {
            // Если и случайный выстрел попал в уже обстрелянную клетку,
            // мы продолжим цикл, пока не найдем свободную.
            // Это немного неэффективно, но гарантирует, что выстрел будет сделан.
        }
        if (res == 0) {
            isPlayerTurn = true;
            updateTurnVisuals();
        } else {
            checkGameStatus();
            if(!isGameOver) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        }
    }

    // Обработка результата выстрела
    if (res == 0) {
        // --- ПРОМАХ ---
        infoLabel->setText("Враг промахнулся. Ваш ход!");
        isPlayerTurn = true;
    }
    else {
        // --- ПОПАДАНИЕ или УБИЙСТВО ---

        if (res == 1) {
            // РАНЕН
            infoLabel->setText("Враг попал! (Уточняет линию огня)");
            shipHitPoints.append(QPoint(x, y));

            if (shipHitPoints.size() == 1) {
                // Первый удар: добавляем 4 соседа для поиска линии
                addInitialTargets(x, y);
            }
            else {
                // Второй удар и далее: уточняем линию и чистим очередь
                determineNextTargetLine();
            }

            // Бот стреляет снова
            checkGameStatus();
            if(!isGameOver) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        }
        else if (res == 2) {
            // УБИТ
            infoLabel->setText("Враг уничтожил ваш корабль!");

            // Очищаем память: корабль добит, очередь и точки попадания сбрасываются
            enemyTargetQueue.clear();
            shipHitPoints.clear();

            // Бот стреляет снова, чтобы начать искать новый корабль
            checkGameStatus();
            if(!isGameOver) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
        }
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

    // ИЗМЕНЕНИЕ 2: Раскрываем расположение кораблей противника
    enemyBoard->setShowShips(true);
    enemyBoard->update(); // Перерисовываем, чтобы показать корабли

    enemyBoard->setEnabled(false);

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!\nВы уничтожили весь флот!");
        infoLabel->setStyleSheet("color: green; font-size: 20px; font-weight: bold;");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ.\nВаш флот разбит.");
        infoLabel->setStyleSheet("color: red; font-size: 20px; font-weight: bold;");
    }

    finishGameBtn->show();
}

void GameWindow::onFinishGameClicked() {
    emit backToMenu(); // Сигнализируем главному окну
    this->close();
}
