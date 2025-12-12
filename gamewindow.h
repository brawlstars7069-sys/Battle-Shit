#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QTimer>
#include "boardwidget.h"

// --- КЛАСС АВАТАРА ---
class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(bool isPlayer, QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool isPlayer;
};

// --- КЛАСС СООБЩЕНИЯ (ЧАТА) ---
class MessageBubble : public QLabel {
    Q_OBJECT
public:
    explicit MessageBubble(QWidget *parent = nullptr);
    void showMessage(const QString &text);
private slots:
    void hideMessage();
protected:
    // Переопределяем, чтобы не схлопывался
    QSize sizeHint() const override { return QSize(160, 60); }
private:
    QTimer *hideTimer;
};

// --- ОСНОВНОЕ ОКНО ---
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
    // Переопределяем eventFilter для отслеживания мыши над дочерними виджетами
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onStartBattleClicked();
    void onRandomPlaceClicked(); // НОВЫЙ СЛОТ
    void onPlayerBoardClick(int x, int y);
    void enemyTurn();
    void onFinishGameClicked();
    void onExitToMenuClicked();

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;

    QWidget *centerWidget;
    QLabel *infoLabel;
    QPushButton *startBattleBtn;
    QPushButton *randomPlaceBtn; // НОВАЯ КНОПКА
    QPushButton *finishGameBtn;
    QWidget *shipsSetupPanel;

    AvatarWidget *playerAvatar;
    AvatarWidget *enemyAvatar;
    MessageBubble *playerMessage;
    MessageBubble *enemyMessage;

    QPushButton *exitToMenuBtn;

    QVector<Ship*> playerShips;
    QVector<Ship*> enemyShips;
    bool isPlayerTurn;
    bool isBattleStarted;
    bool isGameOver;

    QList<QPoint> enemyTargetQueue;
    QList<QPoint> shipHitPoints;

    QPoint mousePos;

    QStringList hitPhrases;
    QStringList killPhrases;
    QStringList missPhrases;

    void addInitialTargets(int x, int y);
    void determineNextTargetLine();

    void setupUI();
    void initShips();
    void checkGameStatus();
    void endGame(bool playerWon);
    void updateTurnVisuals();

    QString getRandomPhrase(const QStringList &list);
};

#endif // GAMEWINDOW_H
