#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPoint>
#include <QTimer>
#include <QPixmap>
#include "boardwidget.h"
#include "RPSWidget.h"

// Классы-помощники
class AvatarWidget : public QWidget {
    Q_OBJECT
public:
    explicit AvatarWidget(bool isPlayer, QWidget *parent = nullptr);
    void setAvatarImage(const QString &path);

protected:
    void paintEvent(QPaintEvent *event) override;
private:
    bool isPlayer;
    QPixmap avatarImage;
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

// Шкала маны
class ManaBar : public QWidget {
    Q_OBJECT
public:
    explicit ManaBar(QWidget *parent = nullptr);
    void setMana(int mana); // 0 - 100
    int getMana() const { return currentMana; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override { return QSize(200, 30); }

private slots:
    void updateShake();

private:
    int currentMana;
    QTimer *shakeTimer;
    QPoint shakeOffset;
};

// Виджет способности
class AbilityWidget : public QWidget {
    Q_OBJECT
public:
    // Добавлен аргумент iconPath
    explicit AbilityWidget(int type, int cost, const QString &iconPath, QWidget *parent = nullptr);
    void setAvailable(bool available);
    int getCost() const { return cost; }
    int getType() const { return type; }

signals:
    void clicked(int type);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    // События для отслеживания мыши
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    QSize sizeHint() const override { return QSize(60, 60); }

private slots:
    void updateShake();

private:
    int type; // 1, 2, 3
    int cost;
    bool isAvailable;

    // Новые поля для иконки и анимации
    QPixmap iconPixmap;
    QTimer *shakeTimer;
    QPoint shakeOffset;
};


class GameWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GameWindow(const QString &playerAvatarPath = "", QWidget *parent = nullptr);
    ~GameWindow();

signals:
    void backToMenu();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onStartBattleClicked();
    void startGameAfterRPS(bool playerFirst);

    void onRandomPlaceClicked();
    void onPlayerBoardClick(int x, int y);
    void onMissileImpact(int x, int y, bool isHit);

    // Слоты способностей
    void onAbilityClicked(int type);
    void activateFog();
    void activateRadar();
    void activateCluster();

    // Обработка очереди выстрелов для кластера
    void processClusterShot();

    void enemyTurn();
    void onFinishGameClicked();
    void onExitToMenuClicked();
    void updateShake();

private:
    BoardWidget *playerBoard;
    BoardWidget *enemyBoard;

    QWidget *centerWidget;
    QLabel *infoLabel;

    // Панель расстановки
    QWidget *shipsSetupPanel;
    QPushButton *startBattleBtn;
    QPushButton *randomPlaceBtn;

    // Панель боя
    QWidget *battlePanel;
    ManaBar *manaBar;
    AbilityWidget *ability1;
    AbilityWidget *ability2;
    AbilityWidget *ability3;

    QPushButton *finishGameBtn;

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
    bool isAnimating = false;

    // Мана
    int playerMana;
    void addMana(int amount);
    void resetMana();
    void updateAbilitiesState();

    // --- Состояния способностей ---
    bool isFogActive = false;

    bool isRadarActive = false;
    QPoint radarCell = QPoint(-1, -1);

    bool isClusterMode = false;
    bool isClusterExecuting = false;
    QList<QPoint> clusterQueue;
    int clusterHitsCount = 0; // Для подсчета попаданий в серии
    // ----------------------------

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

    RPSWidget *rpsOverlay = nullptr;
    QString currentPlayerAvatarPath;
};

#endif // GAMEWINDOW_H
