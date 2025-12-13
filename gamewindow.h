#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QTimer>
#include "boardwidget.h"

// Классы-помощники
class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(bool isPlayer, QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool isPlayer;
};

class MessageBubble : public QLabel {
    Q_OBJECT
public:
    explicit MessageBubble(QWidget *parent = nullptr);
    void showMessage(const QString &text);
private slots:
    void hideMessage();
protected:
    QSize sizeHint() const override { return QSize(160, 60); }
private:
    QTimer *hideTimer;
};

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
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onStartBattleClicked();
    void onRandomPlaceClicked();
    void onPlayerBoardClick(int x, int y);
    // НОВЫЙ СЛОТ: Обработка удара ракеты
    void onMissileImpact(int x, int y, bool isHit);

    void enemyTurn();
    void onFinishGameClicked();
    void onExitToMenuClicked();
    void updateShake();

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;

    QWidget *centerWidget;
    QLabel *infoLabel;
    QPushButton *startBattleBtn;
    QPushButton *randomPlaceBtn;
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
    bool isAnimating = false; // Блокировка ввода

    QList<QPoint> enemyTargetQueue;
    QList<QPoint> shipHitPoints;

    QPoint mousePos;

    QStringList hitPhrases;
    QStringList killPhrases;
    QStringList missPhrases;

    // Тряска экрана
    QTimer *shakeTimer;
    QPoint originalPos;
    int shakeFrames = 0;
    void shakeScreen();

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
