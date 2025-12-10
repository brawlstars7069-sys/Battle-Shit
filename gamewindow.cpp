#include "gamewindow.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QTimer>
#include <QRandomGenerator>

// Виджет "Корабль в магазине", который можно перетащить на поле
class DraggableShipLabel : public QLabel {
public:
    int shipId;
    int size;
    DraggableShipLabel(int id, int s, QWidget* p=nullptr) : QLabel(p), shipId(id), size(s) {
        setText(QString("%1-палубный").arg(size));
        setFrameStyle(QFrame::Panel | QFrame::Raised);
        setAlignment(Qt::AlignCenter);
        setStyleSheet("background-color: lightgray; margin: 2px;");
        setFixedHeight(30);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData();
            // По умолчанию горизонтально (0)
            mimeData->setText(QString("%1:%2").arg(shipId).arg(0));
            drag->setMimeData(mimeData);
            drag->exec(Qt::CopyAction);
        }
    }
};

GameWindow::GameWindow(QWidget *parent) : QWidget(parent), isBattleStarted(false), isGameOver(false)
{
    setWindowTitle("Морской Бой");
    initShips();
    setupUI();
    resize(850, 550);
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

void GameWindow::setupUI() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // Лево: Игрок
    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(new QLabel("Ваш флот"));
    playerBoard = new BoardWidget(this);
    playerBoard->setShips(playerShips);
    playerBoard->setEditable(true);
    playerBoard->setShowShips(true); // Свои видим всегда
    leftLayout->addWidget(playerBoard);

    // Центр: Инфо и Кнопки
    QVBoxLayout *centerLayout = new QVBoxLayout();

    infoLabel = new QLabel("Расставьте корабли\n(Перетащите их)");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    shipsSetupPanel = new QWidget();
    QVBoxLayout *shipsLayout = new QVBoxLayout(shipsSetupPanel);
    shipsLayout->setAlignment(Qt::AlignCenter);

    // Группируем корабли по размеру, чтобы создать уникальные лейблы
    QMap<int, int> shipCounts;
    for (Ship* s : playerShips) {
        shipCounts[s->size]++;
    }

    // Добавляем лейблы для каждого корабля (DraggableShipLabel)
    for (Ship* s : playerShips) {
        // Мы хотим, чтобы все корабли 4-палубника выглядели одинаково
        // Но при этом каждый объект должен быть уникальным для перетаскивания
        DraggableShipLabel *shipLabel = new DraggableShipLabel(s->id, s->size);
        shipsLayout->addWidget(shipLabel);
    }
    shipsLayout->addStretch();

    startBattleBtn = new QPushButton("В БОЙ!");
    startBattleBtn->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold; padding: 10px;");
    connect(startBattleBtn, &QPushButton::clicked, this, &GameWindow::onStartBattleClicked);

    finishGameBtn = new QPushButton("Закончить игру");
    finishGameBtn->setStyleSheet("background-color: #f44336; color: white; padding: 10px;");
    finishGameBtn->hide(); // Скрыта в начале
    connect(finishGameBtn, &QPushButton::clicked, this, &GameWindow::onFinishGameClicked);

    centerLayout->addStretch();
    centerLayout->addWidget(infoLabel);
    centerLayout->addWidget(shipsSetupPanel);
    centerLayout->addWidget(startBattleBtn);
    centerLayout->addWidget(finishGameBtn);
    centerLayout->addStretch();

    // Право: Враг
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(new QLabel("Радар противника"));
    enemyBoard = new BoardWidget(this);
    enemyBoard->setShips(enemyShips);
    enemyBoard->setEditable(false);
    enemyBoard->setShowShips(false); // <--- СКРЫВАЕМ КОРАБЛИ ВРАГА
    enemyBoard->setEnabled(false);   // Пока не начнем, кликать нельзя
    connect(enemyBoard, &BoardWidget::cellClicked, this, &GameWindow::onPlayerBoardClick);
    rightLayout->addWidget(enemyBoard);

    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(centerLayout);
    mainLayout->addLayout(rightLayout);
}

void GameWindow::onStartBattleClicked() {
    // Проверка: все ли корабли на поле?
    for(auto s : playerShips) if(!s->isPlaced()) return;

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
    if (res == -1) return; // Повтор клика

    if (res == 0) {
        infoLabel->setText("Промах!");
        isPlayerTurn = false;
        QTimer::singleShot(800, this, &GameWindow::enemyTurn);
    } else {
        // Попал или Убил
        infoLabel->setText(res == 2 ? "КОРАБЛЬ УНИЧТОЖЕН!" : "ПОПАДАНИЕ!");
        checkGameStatus();
        // При попадании ход сохраняется, таймер не нужен
    }
}

void GameWindow::enemyTurn() {
    if(isPlayerTurn || !isBattleStarted || isGameOver) return;

    int x, y, res;
    do {
        x = QRandomGenerator::global()->bounded(10);
        y = QRandomGenerator::global()->bounded(10);
        res = playerBoard->receiveShot(x, y);
    } while(res == -1);

    if (res == 0) {
        infoLabel->setText("Враг промахнулся. Ваш ход!");
        isPlayerTurn = true;
    } else {
        infoLabel->setText(res == 2 ? "Враг уничтожил ваш корабль!" : "Враг попал!");
        checkGameStatus();
        if(!isGameOver) QTimer::singleShot(800, this, &GameWindow::enemyTurn);
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

    enemyBoard->setEnabled(false);

    if (playerWon) {
        infoLabel->setText("ПОБЕДА!\nВы уничтожили весь флот!");
        infoLabel->setStyleSheet("color: green; font-size: 20px; font-weight: bold;");
    } else {
        infoLabel->setText("ПОРАЖЕНИЕ.\nВаш флот разбит.");
        infoLabel->setStyleSheet("color: red; font-size: 20px; font-weight: bold;");
    }

    finishGameBtn->show(); // Показываем кнопку выхода
}

void GameWindow::onFinishGameClicked() {
    emit backToMenu(); // Сигнализируем главному окну
    this->close();
}
