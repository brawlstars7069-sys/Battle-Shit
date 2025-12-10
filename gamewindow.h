#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "boardwidget.h"

class GameWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GameWindow(QWidget *parent = nullptr);
    ~GameWindow();

signals:
    void backToMenu(); // Сигнал для MainWindow

private slots:
    void onStartBattleClicked();
    void onPlayerBoardClick(int x, int y);
    void enemyTurn();
    void onFinishGameClicked(); // Слот для кнопки выхода

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;
    QLabel *infoLabel;

    // Элементы управления
    QPushButton *startBattleBtn;
    QPushButton *finishGameBtn; // Новая кнопка
    QWidget *shipsSetupPanel;

    QVector<Ship*> playerShips;
    QVector<Ship*> enemyShips;
    bool isPlayerTurn;
    bool isBattleStarted;
    bool isGameOver;

    void setupUI();
    void initShips();
    void checkGameStatus();
    void endGame(bool playerWon);
};

#endif // GAMEWINDOW_H
