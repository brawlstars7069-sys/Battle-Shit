#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QMouseEvent>
#include <QPoint>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QGridLayout>
#include <QListWidget>
#include <QInputDialog>
#include "loginwindow.h"
#include "createserverdialog.h"
#include "networkclient.h"

class GameWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    Q_PROPERTY(float backgroundOffset READ getBackgroundOffset WRITE setBackgroundOffset)
    Q_PROPERTY(float backgroundOffsetY READ getBackgroundOffsetY WRITE setBackgroundOffsetY)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    float getBackgroundOffset() const { return backgroundOffset; }
    void setBackgroundOffset(float offset);

    float getBackgroundOffsetY() const { return backgroundOffsetY; }
    void setBackgroundOffsetY(float offset);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSinglePlayerClicked();
    void onMultiplayerClicked();
    void onSettingsClicked();
    void onExitClicked();

    void onBackFromSettingsClicked();
    void onAvatarSelected(const QString &path);
    void onChangeAvatarClicked();

    void onBackFromMultiplayerClicked();
    void onCreateServerClicked();
    void onConnectClicked();
    void onRegistrationFinished();

    // UI слоты
    void onServerCreatedUI(const QString &name, const QString &password);
    void onCancelWaiting();

    // Сетевые слоты (Ответы от сервера)
    void onNetworkConnected();
    void onNetworkError(const QString &msg);
    void onLobbyCreated(const QString &gameId);
    void onJoinedLobby(const QString &gameId);
    void onPlayerJoinedMyLobby(const QString &playerName);
    void onGameError(const QString &msg);

private:
    void setupUI();
    void setupMenuContainer();
    void setupSettingsContainer();
    void setupMultiplayerContainer();
    void setupWaitingLobby();

    void startMultiplayerAnimation();

    QPoint mousePos;

    QWidget *menuContainer;
    QWidget *settingsContainer;
    QWidget *multiplayerContainer;

    QWidget *waitingLobbyWidget;
    QLabel *waitingStatusLabel;

    // Элементы для копирования ID
    QLineEdit *gameIdDisplay;

    QWidget *avatarSelectionWidget;
    QPushButton *btnChangeAvatar;
    QLabel *currentAvatarPreview;
    QListWidget *serverListWidget;

    QString selectedAvatarPath;
    // Сохраняем логин игрока, чтобы отправить его на сервер
    QString currentPlayerName;

    float backgroundOffset;
    float backgroundOffsetY;

    QPropertyAnimation *animBackground;
    QPropertyAnimation *animBackgroundY;
    QPropertyAnimation *animMenu;
    QPropertyAnimation *animSettings;
    QPropertyAnimation *animMultiplayer;

    LoginWindow *loginWindow;
    CreateServerDialog *createServerDialog;

    NetworkClient *netClient;
    bool isUserRegistered;
};

#endif // MAINWINDOW_H
