#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include "ship.h"

enum CellState { Empty, ShipCell, Miss, Hit };

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    void setEditable(bool editable);
    void setShips(const QVector<Ship*>& ships);

    // Новое: скрывать или показывать корабли (для поля врага - false)
    void setShowShips(bool show);

    bool placeShip(Ship* ship, int x, int y, Orientation orient);
    bool autoPlaceShips();
    void clearBoard();

    int receiveShot(int x, int y);
    bool isAllDestroyed();

signals:
    void cellClicked(int x, int y);
    void shipPlaced();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool isEditable;
    bool showShips; // Флаг видимости
    CellState grid[10][10];
    QVector<Ship*> myShips;

    bool canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip = nullptr);
    Ship* getShipAt(int x, int y);
    void markAroundDestroyed(Ship* ship);
};

#endif // BOARDWIDGET_H
