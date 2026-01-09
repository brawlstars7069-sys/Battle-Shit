#ifndef SHIP_H
#define SHIP_H

#include <vector>
#include <QPoint>

// Ориентация корабля
enum class Orientation { Horizontal, Vertical };

class Ship {
public:
    int id;             // Уникальный ID
    int size;           // Количество палуб (1-4)
    Orientation orientation;
    QPoint topLeft;     // Координата носа корабля на сетке (0-9, 0-9)
    int hits;           // Сколько раз попали

    Ship(int _id, int _size) : id(_id), size(_size), orientation(Orientation::Horizontal), hits(0) {
        topLeft = QPoint(-1, -1); // Не размещен
    }

    bool isPlaced() const { return topLeft.x() >= 0; }
    bool isDestroyed() const { return hits >= size; }
};

#endif // SHIP_H
