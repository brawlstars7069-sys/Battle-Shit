#include "networkclient.h"
#include <QDebug>

NetworkClient::NetworkClient(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, this, &NetworkClient::connected);
    connect(socket, &QTcpSocket::disconnected, this, &NetworkClient::disconnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(socket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::errorOccurred),
            this, &NetworkClient::onSocketError);
}

void NetworkClient::connectToServer(const QString &ip, int port)
{
    if (socket->state() != QAbstractSocket::ConnectedState) {
        socket->abort();
        socket->connectToHost(ip, port);
    }
}

bool NetworkClient::isConnected() const {
    return socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::sendJson(const QJsonObject &json) {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        data.append('\n');
        socket->write(data);
        socket->flush();
    }
}

// --- Lobby Methods ---

void NetworkClient::createLobby(const QString &playerName) {
    QJsonObject json;
    json["action"] = "create_game";
    json["data"] = playerName;
    sendJson(json);
}

void NetworkClient::joinLobby(const QString &gameId) {
    QJsonObject json;
    json["action"] = "join_game";
    json["gameId"] = gameId;
    sendJson(json);
}

// --- Game Methods ---

void NetworkClient::sendReady() {
    QJsonObject json;
    json["action"] = "game_event";
    json["type"] = "ready";
    sendJson(json);
}

void NetworkClient::sendRPS(int shapeId) {
    QJsonObject json;
    json["action"] = "game_event";
    json["type"] = "rps";
    json["value"] = shapeId;
    sendJson(json);
}

void NetworkClient::sendFire(int x, int y) {
    QJsonObject json;
    json["action"] = "game_event";
    json["type"] = "fire";
    json["x"] = x;
    json["y"] = y;
    sendJson(json);
}

void NetworkClient::sendFireResult(int x, int y, int status) {
    QJsonObject json;
    json["action"] = "game_event";
    json["type"] = "fire_result";
    json["x"] = x;
    json["y"] = y;
    json["value"] = status;
    sendJson(json);
}

void NetworkClient::sendChatMessage(const QString &msg) {
    QJsonObject json;
    json["action"] = "game_event";
    json["type"] = "chat";
    json["data"] = msg;
    sendJson(json);
}

// --- Handling ---

void NetworkClient::onReadyRead()
{
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON Parse Error:" << parseError.errorString() << "Data:" << line;
            continue;
        }

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QString action = obj["action"].toString();
            QString data = obj["data"].toString();

            qDebug() << "Server Action:" << action;

            if (action == "game_created") {
                // Сервер иногда присылает gameId внутри data или отдельно
                QString gid = obj.contains("gameId") ? obj["gameId"].toString() : data;
                emit lobbyCreated(gid);
            }
            else if (action == "game_joined") { // Изменено с joined_game под стандарт
                QString gid = obj.contains("gameId") ? obj["gameId"].toString() : data;
                emit joinedLobby(gid);
            }
            else if (action == "player_joined") {
                QString name = obj.contains("opponentId") ? obj["opponentId"].toString() : data;
                emit playerJoined(name);
            }
            else if (action == "error") {
                emit gameError(obj["message"].toString());
            }
            // Обработка игровых событий (Relay от сервера)
            else if (action == "game_event") {
                QString type = obj["type"].toString();
                if (type == "ready") {
                    emit opponentReady();
                } else if (type == "rps") {
                    emit opponentRPS(obj["value"].toInt());
                } else if (type == "fire") {
                    emit opponentFired(obj["x"].toInt(), obj["y"].toInt());
                } else if (type == "fire_result") {
                    emit fireResultReceived(obj["x"].toInt(), obj["y"].toInt(), obj["value"].toInt());
                } else if (type == "chat") {
                    emit chatMessageReceived(obj["data"].toString());
                }
            }
        }
    }
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    emit errorOccurred(socket->errorString());
}
