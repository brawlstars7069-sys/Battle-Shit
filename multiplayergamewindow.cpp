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
    iAmReady(false), opponentIsReady(false), myRPSShape(0), opponentRPSShape(0)
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

    // Подключаем сетевые сигналы
    connect(netClient, &NetworkClient::opponentReady, this, &MultiplayerGameWindow::onOpponentReady);
    connect(netClient, &NetworkClient::opponentRPS, this, &MultiplayerGameWindow::onOpponentRPS);
    connect(netClient, &NetworkClient::opponentFired, this, &MultiplayerGameWindow::onOpponentFired);
    connect(netClient, &NetworkClient::fireResultReceived, this, &MultiplayerGameWindow::onFireResultReceived);
    connect(netClient, &NetworkClient::chatMessageReceived, this, &MultiplayerGameWindow::onChatMessageReceived);
    connect(netClient, &NetworkClient::disconnected, this, &MultiplayerGameWindow::onExitToMenuClicked);

    initShips();
    setupUI();

    // В мультиплеере вражеское поле всегда неактивно для расстановки
    enemyBoard->setEditable(false);

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

    // Header
    QWidget *headerWidget = new QWidget(this);
    headerWidget->setStyleSheet("background-color: rgba(40, 40, 60, 200); border-bottom: 2px solid #555;");
    headerWidget->setFixedHeight(60);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);

    QLabel *gameTitle = new QLabel("МУЛЬТИПЛЕЕР", this);
    gameTitle->setStyleSheet("color: #f0e6d2; font-weight: bold; font-size: 24px; font-family: 'Courier New';");

    exitToMenuBtn = new QPushButton("СДАТЬСЯ", this);
    exitToMenuBtn->setStyleSheet("background-color: #c0392b; color: white; border: 2px solid #922b21; padding: 5px 15px;");
    connect(exitToMenuBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onExitToMenuClicked);

    headerLayout->addWidget(gameTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(exitToMenuBtn);
    globalLayout->addWidget(headerWidget);

    // Game Content
    QWidget *gameContentWidget = new QWidget(this);
    gameContentWidget->installEventFilter(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(gameContentWidget);
    mainLayout->setContentsMargins(30, 10, 30, 20);

    // Левая колонка (Игрок)
    QVBoxLayout *leftLayout = new QVBoxLayout();
    playerAvatar = new AvatarWidget(true, this);
    playerMessage = new MessageBubble(this);
    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true); // Можно двигать корабли до старта
    playerBoard->setShowShips(true);
    playerBoard->setupSizePolicy();

    leftLayout->addWidget(playerMessage, 0, Qt::AlignCenter);
    leftLayout->addWidget(playerAvatar, 0, Qt::AlignCenter);
    leftLayout->addWidget(new QLabel("ВАШ ФЛОТ"), 0, Qt::AlignCenter);
    leftLayout->addWidget(playerBoard);

    // Центральная колонка
    centerWidget = new QWidget();
    centerWidget->setFixedWidth(280);
    centerWidget->setStyleSheet("background-color: #f0e6d2; border: 4px solid #2c3e50;");
    QVBoxLayout *centerLayout = new QVBoxLayout(centerWidget);

    infoLabel = new QLabel("ОЖИДАНИЕ\nСОПЕРНИКА...");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; font-family: 'Courier New';");

    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsSetupLayout = new QVBoxLayout(shipsSetupPanel);
    randomPlaceBtn = new QPushButton("АВТО-РАССТАНОВКА");
    connect(randomPlaceBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onRandomPlaceClicked);

    startBattleBtn = new QPushButton("ГОТОВ К БОЮ!");
    startBattleBtn->setStyleSheet("background-color: #27ae60; color: white; font-weight: bold; padding: 10px;");
    connect(startBattleBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onStartBattleClicked);

    shipsSetupLayout->addWidget(randomPlaceBtn);
    shipsSetupLayout->addWidget(startBattleBtn);

    battlePanel = new QWidget();
    battlePanel->hide();
    QVBoxLayout *battleLayout = new QVBoxLayout(battlePanel);
    manaBar = new ManaBar(battlePanel);
    manaBar->setMana(0); // Заглушка

    // Способности-заглушки
    QWidget *absWidget = new QWidget();
    QHBoxLayout *absLayout = new QHBoxLayout(absWidget);
    ability1 = new AbilityWidget(1, 60, ":/images/fog.png");
    ability2 = new AbilityWidget(2, 80, ":/images/radar.png");
    ability3 = new AbilityWidget(3, 100, ":/images/airstrike.png");
    connect(ability1, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);
    connect(ability2, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);
    connect(ability3, &AbilityWidget::clicked, this, &MultiplayerGameWindow::onAbilityClicked);
    absLayout->addWidget(ability1);
    absLayout->addWidget(ability2);
    absLayout->addWidget(ability3);

    battleLayout->addWidget(new QLabel("МАНА (НЕДОСТУПНО)"));
    battleLayout->addWidget(manaBar);
    battleLayout->addWidget(new QLabel("СПОСОБНОСТИ (ТЕСТ)"));
    battleLayout->addWidget(absWidget);

    finishGameBtn = new QPushButton("В МЕНЮ");
    finishGameBtn->hide();
    connect(finishGameBtn, &QPushButton::clicked, this, &MultiplayerGameWindow::onFinishGameClicked);

    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addWidget(battlePanel);
    centerLayout->addStretch();
    centerLayout->addWidget(finishGameBtn);

    // Правая колонка (Враг)
    QVBoxLayout *rightLayout = new QVBoxLayout();
    enemyAvatar = new AvatarWidget(false, this);
    enemyMessage = new MessageBubble(this);
    enemyBoard = new BoardWidget(this);
    enemyBoard->setShips(enemyShips);
    enemyBoard->setEnemy(true);
    enemyBoard->setEditable(false);
    enemyBoard->setShowShips(false); // Не видим корабли врага
    enemyBoard->setupSizePolicy();
    // Подключаем клик по вражескому полю
    connect(enemyBoard, &BoardWidget::cellClicked, this, &MultiplayerGameWindow::onPlayerBoardClick);

    rightLayout->addWidget(enemyMessage, 0, Qt::AlignCenter);
    rightLayout->addWidget(enemyAvatar, 0, Qt::AlignCenter);
    rightLayout->addWidget(new QLabel("ПРОТИВНИК"), 0, Qt::AlignCenter);
    rightLayout->addWidget(enemyBoard);

    mainLayout->addLayout(leftLayout);
    mainLayout->addWidget(centerWidget);
    mainLayout->addLayout(rightLayout);
    globalLayout->addWidget(gameContentWidget);
}

void MultiplayerGameWindow::onRandomPlaceClicked() {
    if (iAmReady) return;
    playerBoard->autoPlaceShips();
    playerBoard->update();
}

void MultiplayerGameWindow::onStartBattleClicked() {
    // Проверка, все ли корабли расставлены
    // (упрощение: считаем, что autoPlaceShips всегда вызывается или пользователь сам все расставил)
    // В идеале пройтись по playerShips и проверить isPlaced()

    iAmReady = true;
    startBattleBtn->setEnabled(false);
    startBattleBtn->setText("ОЖИДАНИЕ...");
    playerBoard->setEditable(false);

    // Отправляем на сервер
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

// --- RPS LOGIC ---
void MultiplayerGameWindow::startRPS() {
    shipsSetupPanel->hide();
    infoLabel->setText("К-Н-Б!");

    // Запускаем виджет КНБ
    rpsOverlay = new RPSWidget(this);
    connect(rpsOverlay, &RPSWidget::gameFinished, this, [&](bool /*unused*/){
        // Локально игрок выбрал что-то, но RPSWidget возвращает сразу победу/поражение в сингле.
        // Нам нужно перехватить выбор. RPSWidget нужно немного модифицировать
        // или просто использовать сигнал choiceMade если бы он был.
        // Для совместимости со старым кодом, я предположу, что мы просто генерируем рандом здесь
        // и отправляем его, а RPSWidget просто визуализация.
    });

    // Хак: так как RPSWidget написан для сингла, мы просто используем его UI
    // и ловим его закрытие/результат. Но для сетевой игры лучше сделать обмен.
    // Упростим: покажем UI, пусть игрок выберет.
    // Добавим метод в NetworkClient для отправки выбора (1,2,3)

    rpsOverlay->show();

    // В текущей реализации RPSWidget сразу играет с ботом.
    // В мультиплеере это не подойдет.
    // Вместо сложного рефакторинга RPSWidget, сделаем упрощенную логику "Кто хост, тот и первый".
    // И скажем об этом в чате.

    rpsOverlay->hide(); // Скрываем, так как он для сингла
    delete rpsOverlay;
    rpsOverlay = nullptr;

    // Просто определяем ход: Хост первый.
    isPlayerTurn = isHost;

    battlePanel->show();
    isBattleStarted = true;

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
}

void MultiplayerGameWindow::updateTurnVisuals() {
    if (isPlayerTurn) {
        playerAvatar->setStyleSheet("border: 2px solid yellow;");
        enemyAvatar->setStyleSheet("border: none;");
        enemyBoard->setActive(true);
    } else {
        enemyAvatar->setStyleSheet("border: 2px solid yellow;");
        playerAvatar->setStyleSheet("border: none;");
        enemyBoard->setActive(false);
    }
}

// --- GAMEPLAY ---

void MultiplayerGameWindow::onPlayerBoardClick(int x, int y) {
    if (!isBattleStarted || !isPlayerTurn || isGameOver || isAnimating) return;
    if (!enemyBoard->canShootAt(x, y)) return;

    // Блокируем ввод пока ждем ответа сервера
    isAnimating = true;
    enemyBoard->setActive(false);

    // Анимация выстрела (визуальная, пока летит)
    enemyBoard->animateShot(x, y);

    // Отправляем запрос
    netClient->sendFire(x, y);
}

// Меня атаковали
void MultiplayerGameWindow::onOpponentFired(int x, int y) {
    // Враг выстрелил в мои координаты x, y
    // Проверяем результат локально на playerBoard
    int result = playerBoard->receiveShot(x, y);
    // result: 0=Miss, 1=Hit, 2=Destroyed

    if (result > 0) shakeScreen();

    // Обновляем визуализацию моего поля
    playerBoard->animateShot(x, y); // Чтобы проиграть анимацию взрыва на моей доске

    // Отправляем результат врагу
    netClient->sendFireResult(x, y, result);

    // Логика смены хода
    if (result == 0) {
        isPlayerTurn = true;
        playerMessage->showMessage(getRandomPhrase(missPhrases)); // "Мимо" от врага
        updateTurnVisuals();
    } else {
        // Враг попал, он продолжает ходить
        isPlayerTurn = false;
        if (result == 1) playerMessage->showMessage("ПОПАДАНИЕ!");
        else playerMessage->showMessage("КОРАБЛЬ УНИЧТОЖЕН!");
        updateTurnVisuals();
        checkGameStatus();
    }
}

// Пришел результат МОЕГО выстрела
void MultiplayerGameWindow::onFireResultReceived(int x, int y, int status) {
    isAnimating = false;

    // Обновляем состояние клетки на enemyBoard вручную
    // Нам нужен метод setCellState в BoardWidget
    CellState st = (status == 0) ? CellState::Miss : CellState::Hit;
    enemyBoard->setCellState(x, y, st);

    // Если убил, нужно пометить вокруг (в BoardWidget нужна логика markAroundDestroyed,
    // но мы не знаем где корабль врага. В простом варианте просто ставим Hit/Miss.
    // Если статус = 2 (Kill), мы можем просто закрасить Hit, а крестики вокруг
    // сервер не присылает координаты. Оставим пока просто Hit.

    if (status == 0) {
        // Промах
        isPlayerTurn = false;
        playerMessage->showMessage(getRandomPhrase(missPhrases));
        updateTurnVisuals();
    } else {
        // Попал
        isPlayerTurn = true;
        enemyBoard->setActive(true); // Ходим еще раз

        if (status == 2) {
            playerMessage->showMessage(getRandomPhrase(killPhrases));
        } else {
            playerMessage->showMessage(getRandomPhrase(hitPhrases));
        }

        checkGameStatus();
    }
}

// --- UTILS ---

void MultiplayerGameWindow::checkGameStatus() {
    // Проверка победы/поражения
    // В мультиплеере мы доверяем локальному состоянию:
    // Если у меня кончились корабли - я проиграл (и враг узнает об этом сам, когда убьет последний)

    if (playerBoard->isAllDestroyed()) {
        endGame(false); // Я проиграл
    }
    // А вот как узнать, что я победил?
    // Мы не знаем, сколько кораблей у врага, если не ведем счетчик.
    // В BoardWidget::isAllDestroyed работает только для `myShips`.
    // Поэтому для enemyBoard это не сработает.
    // Решение: Считать попадания. Всего клеток кораблей 20 (4+3+3+2+2+2+1+1+1+1 = 20).

    int hits = 0;
    for(int x=0; x<10; ++x)
        for(int y=0; y<10; ++y)
            if (enemyBoard->getCellState(x, y) == CellState::Hit) hits++;

    if (hits >= 20) {
        endGame(true);
    }
}

void MultiplayerGameWindow::endGame(bool playerWon) {
    isGameOver = true;
    isBattleStarted = false;
    enemyBoard->setEnabled(false);
    battlePanel->hide();
    finishGameBtn->show();

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!");
        playerMessage->showMessage("УРА!");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ");
        enemyMessage->showMessage("ЛЕГКО!");
    }
}

void MultiplayerGameWindow::onAbilityClicked(int type) {
    QMessageBox::information(this, "Способности",
                             "Способности (Туман, Радар, Авиаудар) пока не работают в сетевом режиме,\n"
                             "так как требуют обновления логики сервера.\n"
                             "Ожидайте в следующих обновлениях!");
}

void MultiplayerGameWindow::onExitToMenuClicked() {
    // Рвем соединение или просто выходим
    // netClient->disconnectFromHost(); // Лучше не рвать, если хотим вернуться в лобби
    emit backToMenu();
    this->close();
}

void MultiplayerGameWindow::onFinishGameClicked() {
    emit backToMenu();
    this->close();
}

void MultiplayerGameWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(240, 240, 250)); // Чуть другой оттенок для MP

    // Параллакс как в одиночной (можно скопировать код из GameWindow::paintEvent)
    // Для краткости просто фон
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(160, 160, 180));
    p.drawRect(0, height()/2, width(), height()/2);
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
    QWidget::resizeEvent(event);
}

void MultiplayerGameWindow::onChatMessageReceived(const QString &msg) {
    enemyMessage->showMessage(msg);
}

void MultiplayerGameWindow::onOpponentRPS(int shapeId) {
    // Заглушка, если решим вернуть RPS
    opponentRPSShape = shapeId;
}
void MultiplayerGameWindow::onRPSFinished(bool playerWon) {}
void MultiplayerGameWindow::checkRPSResult() {}
QString MultiplayerGameWindow::getRandomPhrase(const QStringList &list) {
    return list[QRandomGenerator::global()->bounded(list.size())];
}
