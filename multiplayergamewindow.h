#ifndef MULTIPLAYERGAMEWINDOW_H
#define MULTIPLAYERGAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QTimer>
#include <QPixmap>
#include "boardwidget.h"
#include "networkclient.h"
#include "gamewindow.h" // Используем вспомогательные классы (AvatarWidget и т.д.)
#include "RPSWidget.h"

class MultiplayerGameWindow : public QWidget
{
    Q_OBJECT
public:
    // isHost определяет, кто "главный" (обычно хост ходит первым по умолчанию, если RPS не решит иначе)
    explicit MultiplayerGameWindow(NetworkClient *client, bool isHost, const QString &playerAvatarPath = "", QWidget *parent = nullptr);
    ~MultiplayerGameWindow();

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // UI Slots
    void onRandomPlaceClicked();
    void onStartBattleClicked(); // Кнопка "Готов"
    void onAbilityClicked(int type);
    void onExitToMenuClicked();
    void onFinishGameClicked();

    // Game Logic Slots
    void onPlayerBoardClick(int x, int y); // Клик по вражескому полю (выстрел)
    void updateShake();

    // Network Slots
    void onOpponentReady();
    void onOpponentRPS(int shapeId);
    void onOpponentFired(int x, int y);
    void onFireResultReceived(int x, int y, int status);
    void onChatMessageReceived(const QString &msg);

    // RPS Logic
    void startRPS();
    void onRPSFinished(bool playerWon); // Локальный результат RPS
    void checkRPSResult(); // Сверка выбора

private:
    NetworkClient *netClient;
    bool isHost;

    // UI Elements (аналогично GameWindow)
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;

    QWidget *centerWidget;
    QLabel *infoLabel;

    QWidget *shipsSetupPanel;
    QPushButton *startBattleBtn;
    QPushButton *randomPlaceBtn;

    QWidget *battlePanel;
    ManaBar *manaBar; // Заглушка
    AbilityWidget *ability1;
    AbilityWidget *ability2;
    AbilityWidget *ability3;

    QPushButton *finishGameBtn;
    QPushButton *exitToMenuBtn;

    AvatarWidget *playerAvatar;
    AvatarWidget *enemyAvatar;
    MessageBubble *playerMessage;
    MessageBubble *enemyMessage;

    // Game State
    QVector<Ship*> playerShips;
    QVector<Ship*> enemyShips;

    bool isPlayerTurn;
    bool isBattleStarted;
    bool isGameOver;
    bool isAnimating;

    bool iAmReady;
    bool opponentIsReady;

    // RPS State
    RPSWidget *rpsOverlay;
    int myRPSShape; // 0=None, 1=Rock, 2=Paper, 3=Scissors
    int opponentRPSShape;

    // Helpers
    QPoint mousePos;
    QStringList hitPhrases;
    QStringList killPhrases;
    QStringList missPhrases;

    QTimer *shakeTimer;
    QPoint originalPos;
    int shakeFrames;
    void shakeScreen();

    void setupUI();
    void initShips();
    void updateTurnVisuals();
    void checkGameStatus();
    void endGame(bool playerWon);
    QString getRandomPhrase(const QStringList &list);

    QString currentPlayerAvatarPath;
};

#endif // MULTIPLAYERGAMEWINDOW_H
