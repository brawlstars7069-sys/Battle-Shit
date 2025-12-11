#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include "boardwidget.h"

class GameWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

signals:
    void backToMenu();

private slots:
    void onStartBattleClicked();
    void onPlayerBoardClick(int x, int y);
    void enemyTurn();
    void onFinishGameClicked();

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;
    QLabel *infoLabel;

    QPushButton *startBattleBtn;
    QPushButton *finishGameBtn;
    QWidget *shipsSetupPanel;

    QVector<Ship*> playerShips;
    QVector<Ship*> enemyShips;
    bool isPlayerTurn;
    bool isBattleStarted;
    bool isGameOver;

    // --- НОВОЕ: Умная логика бота ---
    QList<QPoint> enemyTargetQueue;  // Очередь целей для добивания
    QList<QPoint> shipHitPoints;     // Успешно пораженные клетки текущего корабля

    // Вспомогательные методы
    void addInitialTargets(int x, int y); // Добавляет 4 соседа (первое попадание)
    void determineNextTargetLine();       // Уточняет линию огня после второго попадания
    // ---------------------------------

    void setupUI();
    void initShips();
    void checkGameStatus();
    void endGame(bool playerWon);
};

#endif // GAMEWINDOW_H
