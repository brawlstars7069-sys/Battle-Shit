#include "multiplayergamewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QRandomGenerator>
#include <QEvent>
#include <QMouseEvent>
#include <QCursor>

MultiplayerGameWindow::MultiplayerGameWindow(NetworkClient *client, bool isHost, const QString &playerAvatarPath, QWidget *parent)
    : QWidget(parent), netClient(client), isHost(isHost), currentPlayerAvatarPath(playerAvatarPath),
    isPlayerTurn(false), isBattleStarted(false), isGameOver(false), isAnimating(false),
    iAmReady(false), opponentIsReady(false), myRPSShape(RPSType::None), opponentRPSShape(RPSType::None), rpsOverlay(nullptr)
{
    setWindowTitle("Морской Бой - Мультиплеер");
    resize(1000, 750);
    setMouseTracking(true);
    this->installEventFilter(this);

    // Фразы
    hitPhrases << "БАБАХ!" << "ПОЛУЧИ!" << "ЕСТЬ!" << "ПРОБИТИЕ!";
    killPhrases << "НА ДНО!" << "УНИЧТОЖЕН!" << "МИНУС ОДИН!";
    missPhrases << "МИМО!" << "В МОЛОКО" << "НЕ ПОПАЛ";

    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);
    connect(shakeTimer, &QTimer::timeout, this, &MultiplayerGameWindow::updateShake);

    // Подключение к сигналам сети
    connect(netClient, &NetworkClient::opponentReady, this, &MultiplayerGameWindow::onOpponentReady);
    connect(netClient, &NetworkClient::opponentRPS, this, &MultiplayerGameWindow::onOpponentRPS);
    connect(netClient, &NetworkClient::opponentFired, this, &MultiplayerGameWindow::onOpponentFired);
    connect(netClient, &NetworkClient::fireResultReceived, this, &MultiplayerGameWindow::onFireResultReceived);
    connect(netClient, &NetworkClient::chatMessageReceived, this, &MultiplayerGameWindow::onChatMessageReceived);
    connect(netClient, &NetworkClient::disconnected, this, &MultiplayerGameWindow::onExitToMenuClicked);
    connect(netClient, &NetworkClient::turnChanged, this, &MultiplayerGameWindow::onTurnChanged);

    initShips();
    setupUI();

    if (!currentPlayerAvatarPath.isEmpty()) {
        playerAvatar->setAvatarImage(currentPlayerAvatarPath);
    }
}

MultiplayerGameWindow::~MultiplayerGameWindow() {
    qDeleteAll(playerShips);
    qDeleteAll(enemyShips);
}

void MultiplayerGameWindow::initShips() {
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

void MultiplayerGameWindow::setupUI() {
    QVBoxLayout *globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(0);

    // --- Header ---
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: rgba(60, 50, 40, 200); border-bottom: 2px solid #555;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 0, 20, 0);

    QLabel *gameTitle = new QLabel("СЕТЕВАЯ ИГРА", this);
    gameTitle->setStyleSheet("color: #f0e6d2; font-weight: bold; font-size: 24px; font-family: 'Courier New';");

    exitToMenuBtn = new QPushButton("СДАТЬСЯ", this);
    exitToMenuBtn->setCursor(Qt::PointingHandCursor);
    exitToMenuBtn->setStyleSheet(
        "QPushButton { background-color: #c0392b; color: white; border: 2px solid #922b21; }"
        "QPushButton:hover { background-color: #e74c3c; }"
        );
    connect(exitToMenuBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onExitToMenuClicked);

    headerLayout->addWidget(gameTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(exitToMenuBtn);
    globalLayout->addWidget(headerWidget);

    // --- Game Content ---
    QWidget *gameContentWidget = new QWidget(this);
    gameContentWidget->installEventFilter(this);
    gameContentWidget->setStyleSheet("background: transparent;");
    gameContentWidget->setMouseTracking(true);

    QHBoxLayout *mainLayout = new QHBoxLayout(gameContentWidget);
    mainLayout->setContentsMargins(30, 10, 30, 20);
    mainLayout->setSpacing(20);

    // 1. ЛЕВАЯ КОЛОНКА (ИГРОК)
    QVBoxLayout *leftLayout = new QVBoxLayout();

    playerAvatar = new AvatarWidget(true, this);
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
    connect(playerBoard, &BoardWidget::missileImpact, this, &MultiplayerGameWindow::onMissileImpact);

    leftLayout->addLayout(playerAvatarLayout);
    leftLayout->addSpacing(10);
    leftLayout->addWidget(playerTitle);
    leftLayout->addWidget(playerBoard);
    leftLayout->addStretch(1);

    // 2. ЦЕНТРАЛЬНАЯ КОЛОНКА
    centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    centerWidget->installEventFilter(this);
    centerWidget->setMouseTracking(true);
    centerWidget->setStyleSheet(
        "background-color: #f0e6d2; "
        "border: 4px solid #2c3e50; "
        "border-radius: 0px;"
        );

    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);
    centerLayout->setContentsMargins(15, 15, 15, 15);

    infoLabel = new QLabel("РАССТАВЬТЕ\nКОРАБЛИ");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    infoLabel->setMinimumHeight(60);
    infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; border-bottom: 2px solid #ccc; padding-bottom: 10px; font-family: 'Courier New';");

    // Панель расстановки
    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsLayout = new QVBoxLayout(shipsSetupPanel);
    shipsLayout->setSpacing(10);

    randomPlaceBtn = new QPushButton("АВТО-РАССТАНОВКА");
    randomPlaceBtn->setMinimumHeight(40);
    randomPlaceBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; font-size: 16px; font-weight: bold; border: 2px solid #2980b9; }"
        "QPushButton:hover { background-color: #5dade2; }"
        );
    connect(randomPlaceBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onRandomPlaceClicked);

    startBattleBtn = new QPushButton("ГОТОВ К БОЮ");
    startBattleBtn->setMinimumHeight(50);
    startBattleBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; font-size: 20px; font-weight: bold; border: 2px solid #1e8449; }"
        "QPushButton:hover { background-color: #2ecc71; }"
        );
    connect(startBattleBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onStartBattleClicked);

    shipsLayout->addWidget(randomPlaceBtn);
    shipsLayout->addWidget(startBattleBtn);

    // Панель способностей
    battlePanel = new QWidget();
    QVBoxLayout *battlePanelLayout = new QVBoxLayout(battlePanel);
    battlePanelLayout->setAlignment(Qt::AlignCenter);
    battlePanelLayout->setSpacing(15);

    manaBar = new ManaBar(battlePanel);
    manaBar->setMana(0);

    QWidget *abilitiesContainer = new QWidget(battlePanel);
    QVBoxLayout *absLayout = new QVBoxLayout(abilitiesContainer);
    absLayout->setSpacing(10);
    absLayout->setAlignment(Qt::AlignCenter);

    ability1 = new AbilityWidget(1, 60, ":/images/fog.png");
    ability2 = new AbilityWidget(2, 80, ":/images/radar.png");
    ability3 = new AbilityWidget(3, 100, ":/images/airstrike.png");

    connect(ability1, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);
    connect(ability2, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);
    connect(ability3, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);

    ability1->setAvailable(true);
    ability2->setAvailable(true);
    ability3->setAvailable(true);

    absLayout->addWidget(ability1);
    absLayout->addWidget(ability2);
    absLayout->addWidget(ability3);

    battlePanelLayout->addWidget(new QLabel("СЕРВЕР: БАЗОВЫЙ"));
    battlePanelLayout->addWidget(manaBar);
    battlePanelLayout->addSpacing(10);
    battlePanelLayout->addWidget(new QLabel("СПОСОБНОСТИ (WIP)"));
    battlePanelLayout->addWidget(abilitiesContainer);
    battlePanel->hide();

    finishGameBtn = new QPushButton("РЕЗУЛЬТАТ");
    finishGameBtn->setMinimumHeight(50);
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onFinishGameClicked);

    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addWidget(battlePanel);
    centerLayout->addStretch();
    centerLayout->addWidget(finishGameBtn);

    // 3. ПРАВАЯ КОЛОНКА (ВРАГ)
    QVBoxLayout *rightLayout = new QVBoxLayout();

    enemyAvatar = new AvatarWidget(false, this);
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

    // ВАЖНО: Связь клика по вражескому полю со слотом стрельбы
    connect(enemyBoard, &BoardWidget::cellClicked, this, &MultiplayerGameWindow::onPlayerBoardClick);

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

// --- LOGIC ---

void MultiplayerGameWindow::onRandomPlaceClicked() {
    if (iAmReady) return;
    if (playerBoard->autoPlaceShips()) {
        playerBoard->update();
    }
}

void MultiplayerGameWindow::onStartBattleClicked() {
    iAmReady = true;
    startBattleBtn->setEnabled(false);
    startBattleBtn->setText("ОЖИДАНИЕ...");
    randomPlaceBtn->hide();
    playerBoard->setEditable(false);

    netClient->sendReady();

    if (opponentIsReady) {
        startRPS();
    } else {
        infoLabel->setText("ЖДЕМ\nПРОТИВНИКА...");
    }
}

void MultiplayerGameWindow::onOpponentReady() {
    opponentIsReady = true;
    enemyMessage->showMessage("Я ГОТОВ!");

    if (iAmReady) {
        startRPS();
    } else {
        infoLabel->setText("ПРОТИВНИК\nГОТОВ!");
    }
}

void MultiplayerGameWindow::startRPS() {
    shipsSetupPanel->hide();
    infoLabel->setText("К-Н-Б!");

    rpsOverlay = new RPSWidget(this);
    // Включаем режим мультиплеера!
    rpsOverlay->setMultiplayerMode(true);

    connect(rpsOverlay, &RPSWidget::choiceMade, this, &MultiplayerGameWindow::onMyRPSChoice);
    connect(rpsOverlay, &RPSWidget::gameFinished, this, [&](bool playerWon){
        isPlayerTurn = playerWon;
        rpsOverlay->deleteLater();
        rpsOverlay = nullptr;

        battlePanel->show();
        isBattleStarted = true;
        enemyBoard->setEnabled(true);

        if (isPlayerTurn) {
            infoLabel->setText("ВАШ ХОД!");
            playerMessage->showMessage("МОЙ ХОД!");
            enemyBoard->setActive(true);
        } else {
            infoLabel->setText("ХОД\nПРОТИВНИКА");
            enemyMessage->showMessage("МОЙ ХОД!");
            enemyBoard->setActive(false);
        }
        updateTurnVisuals();
    });

    rpsOverlay->show();
}

void MultiplayerGameWindow::onMyRPSChoice(RPSType choice) {
    myRPSShape = choice;
    int val = 0;
    if (choice == RPSType::Rock) val = 1;
    if (choice == RPSType::Paper) val = 2;
    if (choice == RPSType::Scissors) val = 3;
    netClient->sendRPS(val);

    checkRPSRound();
}

void MultiplayerGameWindow::onOpponentRPS(int shapeId) {
    if (shapeId == 1) opponentRPSShape = RPSType::Rock;
    else if (shapeId == 2) opponentRPSShape = RPSType::Paper;
    else if (shapeId == 3) opponentRPSShape = RPSType::Scissors;
    else opponentRPSShape = RPSType::None;

    checkRPSRound();
}

void MultiplayerGameWindow::checkRPSRound() {
    if (myRPSShape != RPSType::None && opponentRPSShape != RPSType::None && rpsOverlay) {
        rpsOverlay->resolveRound(myRPSShape, opponentRPSShape);
    }
}

void MultiplayerGameWindow::onTurnChanged(const QString &who) {
    bool isMyTurnNow = false;
    if (isHost && who == "Player1") isMyTurnNow = true;
    if (!isHost && who == "Player2") isMyTurnNow = true;

    isPlayerTurn = isMyTurnNow;

    if (isPlayerTurn) {
        infoLabel->setText("ВАШ ХОД!");
        enemyBoard->setActive(true);
    } else {
        infoLabel->setText("ХОД\nПРОТИВНИКА");
        enemyBoard->setActive(false);
    }
    updateTurnVisuals();
}

void MultiplayerGameWindow::updateTurnVisuals() {
    if (isPlayerTurn) {
        playerAvatar->setStyleSheet("border: 2px solid yellow;");
        enemyAvatar->setStyleSheet("border: none;");
    } else {
        enemyAvatar->setStyleSheet("border: 2px solid yellow;");
        playerAvatar->setStyleSheet("border: none;");
    }
}

// --- SHOOTING ---

void MultiplayerGameWindow::onPlayerBoardClick(int x, int y) {
    if (!isBattleStarted || !isPlayerTurn || isGameOver || isAnimating) return;
    if (!enemyBoard->canShootAt(x, y)) return;

    isAnimating = true;
    enemyBoard->setActive(false);
    enemyBoard->animateShot(x, y);
    netClient->sendFire(x, y);
}

void MultiplayerGameWindow::onOpponentFired(int x, int y) {
    int result = playerBoard->receiveShot(x, y);
    if (result > 0) shakeScreen();
    playerBoard->animateShot(x, y);

    netClient->sendFireResult(x, y, result);

    if (result == 0) {
        playerMessage->showMessage(getRandomPhrase(missPhrases));
    } else {
        if (result == 1) playerMessage->showMessage("ПОПАДАНИЕ!");
        else playerMessage->showMessage("КОРАБЛЬ УНИЧТОЖЕН!");
        checkGameStatus();
    }
}

void MultiplayerGameWindow::onFireResultReceived(int x, int y, int status) {
    isAnimating = false;
    CellState st = (status == 0) ? CellState::Miss : CellState::Hit;
    enemyBoard->setCellState(x, y, st);

    if (status == 0) {
        playerMessage->showMessage(getRandomPhrase(missPhrases));
    } else {
        enemyBoard->setActive(true);
        if (status == 2) playerMessage->showMessage(getRandomPhrase(killPhrases));
        else playerMessage->showMessage(getRandomPhrase(hitPhrases));
        checkGameStatus();
    }
}

void MultiplayerGameWindow::onMissileImpact(int, int, bool) {
    // Local animation logic
}

void MultiplayerGameWindow::checkGameStatus() {
    if (playerBoard->isAllDestroyed()) {
        endGame(false);
    } else {
        int hits = 0;
        for(int x=0; x<10; ++x)
            for(int y=0; y<10; ++y)
                if (enemyBoard->getCellState(x, y) == CellState::Hit) hits++;

        if (hits >= 20) endGame(true);
    }
}

void MultiplayerGameWindow::endGame(bool playerWon) {
    isGameOver = true;
    isBattleStarted = false;
    enemyBoard->setEnabled(false);
    battlePanel->hide();
    finishGameBtn->show();
    exitToMenuBtn->setText("В МЕНЮ");

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!");
        infoLabel->setStyleSheet("color: #27ae60; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
        playerMessage->showMessage("УРА!");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ");
        infoLabel->setStyleSheet("color: #c0392b; font-size: 22px; font-weight: bold; border-bottom: 2px solid #ccc; font-family: 'Courier New';");
        enemyMessage->showMessage("ЛЕГКО!");
    }
}

void MultiplayerGameWindow::onAbilityClicked(int type) {
    QMessageBox::information(this, "Способности",
                             "В данной версии сервера способности отключены.\nИграйте в классический режим!");
}

void MultiplayerGameWindow::onExitToMenuClicked() {
    emit backToMenu();
    this->close();
}

void MultiplayerGameWindow::onFinishGameClicked() {
    emit backToMenu();
    this->close();
}

void MultiplayerGameWindow::onChatMessageReceived(const QString &msg) {
    enemyMessage->showMessage(msg);
}

void MultiplayerGameWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(248, 240, 227));

    int w = width();
    int h = height();
    p.setBrush(QColor(160, 160, 160));
    p.setPen(Qt::NoPen);
    for (int y = 0; y < h; y += 50) {
        for (int x = -50; x < w + 50; x += 30) {
            p.drawRect(x, y, 4, 4);
        }
    }
}

void MultiplayerGameWindow::updateShake() {
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

void MultiplayerGameWindow::shakeScreen() {
    originalPos = this->pos();
    shakeFrames = 10;
    shakeTimer->start();
}

bool MultiplayerGameWindow::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseMove) {
        QPoint globalPos = QCursor::pos();
        mousePos = this->mapFromGlobal(globalPos);
        update();
    }
    return QWidget::eventFilter(watched, event);
}

void MultiplayerGameWindow::resizeEvent(QResizeEvent *event) {
    if (rpsOverlay) rpsOverlay->resize(size());
    QWidget::resizeEvent(event);
}

QString MultiplayerGameWindow::getRandomPhrase(const QStringList &list) {
    if (list.isEmpty()) return "";
    return list[QRandomGenerator::global()->bounded(list.size())];
}
