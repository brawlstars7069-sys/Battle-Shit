#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QMouseEvent>
#include "boardwidget.h"

class GameWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    // Добавляем событие движения мыши
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onStartBattleClicked();
    void onPlayerBoardClick(int x, int y);
    void enemyTurn();
    void onFinishGameClicked();
    void onExitToMenuClicked();

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;
    QLabel *infoLabel;

    QPushButton *startBattleBtn;
    QPushButton *finishGameBtn;
    QPushButton *exitToMenuBtn;
    QWidget *shipsSetupPanel;

    QVector<Ship*> playerShips;
    QVector<Ship*> enemyShips;
    bool isPlayerTurn;
    bool isBattleStarted;
    bool isGameOver;

    QList<QPoint> enemyTargetQueue;
    QList<QPoint> shipHitPoints;

    // Для эффекта фона
    QPoint mousePos;

    void addInitialTargets(int x, int y);
    void determineNextTargetLine();

    void setupUI();
    void initShips();
    void checkGameStatus();
    void endGame(bool playerWon);
    void updateTurnVisuals();
};

#endif // GAMEWINDOW_H
