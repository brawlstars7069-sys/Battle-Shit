#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);

    void connectToServer(const QString &ip, int port);
    bool isConnected() const;

    // Лобби
    void createLobby(const QString &playerName);
    void joinLobby(const QString &gameId);

    // Игровой процесс
    void sendReady();
    void sendRPS(int shapeId); // 1=Rock, 2=Paper, 3=Scissors
    void sendFire(int x, int y);
    void sendFireResult(int x, int y, int status); // 0=Miss, 1=Hit, 2=Kill
    void sendChatMessage(const QString &msg);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);

    // Сигналы лобби
    void lobbyCreated(const QString &gameId);
    void joinedLobby(const QString &gameId);
    void playerJoined(const QString &playerName);
    void gameError(const QString &message);

    // Сигналы игры
    void opponentReady();
    void opponentRPS(int shapeId);
    void opponentFired(int x, int y);
    void fireResultReceived(int x, int y, int status);
    void chatMessageReceived(const QString &msg);

    // Новый сигнал для смены хода
    void turnChanged(const QString &who); // "Player1" or "Player2"

private slots:
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket *socket;
    void sendJson(const QJsonObject &json);
};

#endif // NETWORKCLIENT_H
