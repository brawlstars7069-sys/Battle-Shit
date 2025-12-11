#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QPainter> // Важно добавить
#include "ship.h"
#include <algorithm>

enum CellState { Empty, ShipCell, Miss, Hit };

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    // Константа для отступа (для цифр и букв)
    static const int MARGIN = 30;

    // Статический метод для рисования корабля (используется и на доске, и в магазине)
    static void drawShipShape(QPainter &p, int size, Orientation orient, QRect rect, bool isEnemy, bool isDestroyed);

    // --- Адаптивность ---
    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    void setupSizePolicy();

    void setEditable(bool editable);
    void setShips(const QVector<Ship*>& ships);
    void setShowShips(bool show);
    void setActive(bool active) { isActive = active; update(); }
    void setEnemy(bool enemy) { isEnemyBoard = enemy; } // Помечаем, что это доска врага

    bool placeShip(Ship* ship, int x, int y, Orientation orient);
    bool autoPlaceShips();
    void clearBoard();

    int receiveShot(int x, int y);
    bool isAllDestroyed();

    // Преобразование координат экрана в координаты сетки
    QPoint getGridCoord(QPoint pos);

signals:
    void cellClicked(int x, int y);
    void shipPlaced();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool isEditable;
    bool showShips;
    bool isEnemyBoard = false; // Флаг для цвета кораблей
    CellState grid[10][10];
    QVector<Ship*> myShips;

    bool canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip);
    Ship* getShipAt(int x, int y);
    void markAroundDestroyed(Ship *s);

    bool isActive = false;
    int hoverX = -1;
    int hoverY = -1;
};

#endif // BOARDWIDGET_H
