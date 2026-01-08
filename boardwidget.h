#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QPainter>
#include <QTimer>
#include "ship.h"
#include <algorithm>

enum CellState { Empty, ShipCell, Miss, Hit };

enum class AnimState { Idle, Falling, Exploding };

struct MissileAnim {
    AnimState state = AnimState::Idle;
    QPoint gridPos;
    QPointF currentPos;
    float targetY;
    float speedY;
    int frame;
    bool isHit;
};

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    static const int MARGIN = 30;

    static void drawShipShape(QPainter &p, int size, Orientation orient, QRect rect, bool isEnemy, bool isDestroyed);

    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    void setupSizePolicy();

    void setEditable(bool editable);
    void setShips(const QVector<Ship*>& ships);
    void setShowShips(bool show);
    void setActive(bool active) { isActive = active; update(); }
    void setEnemy(bool enemy) { isEnemyBoard = enemy; }

    // --- НОВЫЕ МЕТОДЫ ДЛЯ МУЛЬТИПЛЕЕРА ---
    // Позволяет принудительно установить статус клетки (ответ от сервера)
    void setCellState(int x, int y, CellState state);

    // Получить состояние клетки (нужно для радара и проверок)
    CellState getCellState(int x, int y) { return grid[x][y]; }

    // Методы способностей
    void setFog(bool active);
    void setHighlight(QPoint pos);

    // Основная логика
    bool placeShip(Ship* ship, int x, int y, Orientation orient);
    bool autoPlaceShips();
    void clearBoard();

    void animateShot(int x, int y);
    int receiveShot(int x, int y); // Локальный расчет (когда стреляют в нас)

    bool canShootAt(int x, int y);
    bool isAllDestroyed();
    QPoint getGridCoord(QPoint pos);
    bool hasShipAt(int x, int y);

signals:
    void cellClicked(int x, int y);
    void shipPlaced();
    void missileImpact(int x, int y, bool isHit);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void updateAnimation();

private:
    bool isEditable;
    bool showShips;
    bool isEnemyBoard = false;
    CellState grid[10][10];
    QVector<Ship*> myShips;

    bool isFoggy = false;
    QPoint highlightPos = QPoint(-1, -1);

    bool canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip);
    Ship* getShipAt(int x, int y);
    void markAroundDestroyed(Ship *s);
    void placeShipCells(Ship* ship);
    void removeShipCells(Ship* ship);

    bool isActive = false;
    int hoverX = -1;
    int hoverY = -1;

    QTimer *animTimer;
    MissileAnim currentAnim;

    void drawMissile(QPainter &p);
    void drawExplosion(QPainter &p);
};

#endif // BOARDWIDGET_H
