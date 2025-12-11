#ifndef BOARDWIDGET_H
#define BOARDWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPoint>
#include "ship.h"
#include <algorithm> // Для std::min

enum CellState { Empty, ShipCell, Miss, Hit };

class BoardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BoardWidget(QWidget *parent = nullptr);

    // --- Методы для адаптивного дизайна ---
    QSize sizeHint() const override;
    int heightForWidth(int w) const override;
    void setupSizePolicy(); // Настройка QSizePolicy
    // -------------------------------------

    void setEditable(bool editable);
    void setShips(const QVector<Ship*>& ships);
    void setShowShips(bool show);
    void setActive(bool active) { isActive = active; update(); }

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
    void leaveEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    bool isEditable;
    bool showShips; // Флаг видимости
    CellState grid[10][10];
    QVector<Ship*> myShips;

    bool canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip);
    Ship* getShipAt(int x, int y);
    void markAroundDestroyed(Ship *s);

    bool isActive = false; // Подсветка активного поля
    int hoverX = -1;       // Координата X под курсором
    int hoverY = -1;       // Координата Y под курсором
};

#endif // BOARDWIDGET_H
