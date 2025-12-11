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

protected:
    // Добавляем paintEvent для отрисовки фона окна
    void paintEvent(QPaintEvent *event) override;

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

    // ИИ
    QList<QPoint> enemyTargetQueue;
    QList<QPoint> shipHitPoints;

    void addInitialTargets(int x, int y);
    void determineNextTargetLine();

    void setupUI();
    void initShips();
    void checkGameStatus();
    void endGame(bool playerWon);
    void updateTurnVisuals();
};

#endif // GAMEWINDOW_H
