#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QRandomGenerator>

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent), isEditable(false), showShips(true)
{
    setFixedSize(301, 301);
    setAcceptDrops(true);
    clearBoard();
}

void BoardWidget::clearBoard() {
    for(int i=0; i<10; ++i)
        for(int j=0; j<10; ++j)
            grid[i][j] = Empty;
    update();
}

void BoardWidget::setEditable(bool editable) {
    isEditable = editable;
    setAcceptDrops(editable);
}

void BoardWidget::setShips(const QVector<Ship *> &ships) {
    myShips = ships;
}

void BoardWidget::setShowShips(bool show) {
    showShips = show;
    update();
}

// ПРОВЕРКА ПРАВИЛ
bool BoardWidget::canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip) {
    int dx = (orient == Orientation::Horizontal) ? 1 : 0;
    int dy = (orient == Orientation::Vertical) ? 1 : 0;

    // 1. Проверка выхода за границы поля
    int endX = x + (size - 1) * dx;
    int endY = y + (size - 1) * dy;
    if (x < 0 || y < 0 || endX >= 10 || endY >= 10) return false;

    // 2. Проверка пересечений и дистанции (1 клетка)
    for (Ship* other : myShips) {
        if (other == ignoreShip) continue; // Не проверяем самого себя
        if (!other->isPlaced()) continue;  // Пропускаем те, что ещё в "магазине"

        // Координаты другого корабля
        int ox = other->topLeft.x();
        int oy = other->topLeft.y();
        int odx = (other->orientation == Orientation::Horizontal) ? 1 : 0;
        int ody = (other->orientation == Orientation::Vertical) ? 1 : 0;

        // "Опасная зона" вокруг другого корабля
        int dangerLeft = ox - 1;
        int dangerTop = oy - 1;
        int dangerRight = ox + (other->size - 1) * odx + 1;
        int dangerBottom = oy + (other->size - 1) * ody + 1;

        // Проверяем каждую клетку НАШЕГО нового корабля
        for (int k = 0; k < size; ++k) {
            int cx = x + k * dx;
            int cy = y + k * dy;

            if (cx >= dangerLeft && cx <= dangerRight &&
                cy >= dangerTop && cy <= dangerBottom) {
                return false; // Слишком близко!
            }
        }
    }
    return true;
}

bool BoardWidget::placeShip(Ship* ship, int x, int y, Orientation orient) {
    // Передаем ship в canPlace, чтобы игнорировать его старую позицию при проверке новой
    if (canPlace(x, y, ship->size, orient, ship)) {
        ship->topLeft = QPoint(x, y);
        ship->orientation = orient;
        update();
        return true;
    }
    return false;
}

Ship* BoardWidget::getShipAt(int x, int y) {
    for (Ship* s : myShips) {
        if (!s->isPlaced()) continue;

        if (s->orientation == Orientation::Horizontal) {
            if (y == s->topLeft.y() && x >= s->topLeft.x() && x < s->topLeft.x() + s->size)
                return s;
        } else {
            if (x == s->topLeft.x() && y >= s->topLeft.y() && y < s->topLeft.y() + s->size)
                return s;
        }
    }
    return nullptr;
}

void BoardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), Qt::white);
    int cellSize = 30;

    // Сетка
    p.setPen(QPen(QColor(200, 200, 200), 1));
    for (int i = 0; i <= 10; ++i) {
        p.drawLine(i * cellSize, 0, i * cellSize, 300);
        p.drawLine(0, i * cellSize, 300, i * cellSize);
    }

    // Корабли
    for (Ship* s : myShips) {
        if (!s->isPlaced()) continue;

        if (showShips || s->isDestroyed()) {
            int w = (s->orientation == Orientation::Horizontal) ? s->size * cellSize : cellSize;
            int h = (s->orientation == Orientation::Vertical) ? s->size * cellSize : cellSize;

            QColor shipColor = s->isDestroyed() ? QColor(80, 80, 80) : QColor(100, 150, 240);

            p.setBrush(shipColor);
            p.setPen(Qt::black);
            p.drawRect(s->topLeft.x() * cellSize + 2, s->topLeft.y() * cellSize + 2, w - 4, h - 4);
        }
    }

    // Попадания и промахи
    for(int x=0; x<10; ++x) {
        for(int y=0; y<10; ++y) {
            int cx = x * cellSize;
            int cy = y * cellSize;

            if (grid[x][y] == Miss) {
                p.setBrush(Qt::black);
                p.drawEllipse(QPoint(cx + 15, cy + 15), 3, 3);
            } else if (grid[x][y] == Hit) {
                p.setPen(QPen(Qt::red, 3));
                p.drawLine(cx + 5, cy + 5, cx + 25, cy + 25);
                p.drawLine(cx + 25, cy + 5, cx + 5, cy + 25);
                p.setBrush(Qt::NoBrush);
                p.drawRect(cx+1, cy+1, 28, 28);
            }
        }
    }
}

int BoardWidget::receiveShot(int x, int y) {
    if (x<0 || x>9 || y<0 || y>9) return -1;
    if (grid[x][y] != Empty) return -1;

    Ship* s = getShipAt(x, y);
    if (s) {
        grid[x][y] = Hit;
        s->hits++;

        if (s->isDestroyed()) {
            markAroundDestroyed(s);
            update();
            return 2; // Убил
        }
        update();
        return 1; // Попал
    } else {
        grid[x][y] = Miss;
        update();
        return 0; // Мимо
    }
}

void BoardWidget::markAroundDestroyed(Ship *s) {
    int dx = (s->orientation == Orientation::Horizontal) ? 1 : 0;
    int dy = (s->orientation == Orientation::Vertical) ? 1 : 0;

    int x1 = s->topLeft.x();
    int y1 = s->topLeft.y();
    int x2 = x1 + (s->size-1)*dx;
    int y2 = y1 + (s->size-1)*dy;

    for (int i = x1 - 1; i <= x2 + 1; ++i) {
        for (int j = y1 - 1; j <= y2 + 1; ++j) {
            if (i >= 0 && i < 10 && j >= 0 && j < 10) {
                if (grid[i][j] == Empty) grid[i][j] = Miss;
            }
        }
    }
}

// --- DRAG & DROP СОБЫТИЯ ---

void BoardWidget::dragEnterEvent(QDragEnterEvent *event) {
    // ИЗМЕНЕНИЕ: Разрешаем MoveAction. Это важно для "рукопожатия" с магазином.
    if (isEditable && event->mimeData()->hasText()) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
}

void BoardWidget::dropEvent(QDropEvent *event) {
    if (!isEditable) return;
    QStringList parts = event->mimeData()->text().split(":");
    if (parts.size() != 2) return;

    int shipId = parts[0].toInt();
    Orientation orient = static_cast<Orientation>(parts[1].toInt());
    int x = event->position().toPoint().x() / 30;
    int y = event->position().toPoint().y() / 30;

    Ship* targetShip = nullptr;
    for(Ship* s : myShips) { if(s->id == shipId) { targetShip = s; break; } }

    if (targetShip) {
        QPoint oldPos = targetShip->topLeft;
        // Временно "поднимаем" корабль, чтобы он не мешал сам себе при проверке места
        targetShip->topLeft = QPoint(-1, -1);

        if (placeShip(targetShip, x, y, orient)) {
            // ИЗМЕНЕНИЕ: Успех! Ставим действие MoveAction и принимаем событие.
            // Именно это заставит DraggableShipLabel выполнить код this->hide()
            event->setDropAction(Qt::MoveAction);
            event->accept();
            emit shipPlaced();
        } else {
            // Неудачная попытка - возвращаем как было
            targetShip->topLeft = oldPos;
            event->ignore();
        }
    }
}

void BoardWidget::mousePressEvent(QMouseEvent *event) {
    int x = event->pos().x() / 30;
    int y = event->pos().y() / 30;

    // Режим игры (стрельба)
    if (!isEditable) {
        if (event->button() == Qt::LeftButton) {
            emit cellClicked(x, y);
        }
        return;
    }

    // Режим редактирования
    Ship* s = getShipAt(x, y);
    if(s) {
        if (event->button() == Qt::LeftButton) {
            // Drag корабля с поля
            QDrag *drag = new QDrag(this);
            QMimeData *mime = new QMimeData();
            mime->setText(QString("%1:%2").arg(s->id).arg((int)s->orientation));
            drag->setMimeData(mime);

            // Здесь тоже используем MoveAction, чтобы dropEvent обработал это одинаково
            drag->exec(Qt::MoveAction);
        }
        else if (event->button() == Qt::RightButton) {
            // Поворот (твоя рабочая логика + небольшая подстраховка с oldPos)
            Orientation newO = (s->orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
            QPoint oldPos = s->topLeft;
            Orientation oldOrient = s->orientation;

            s->topLeft = QPoint(-1, -1); // Поднимаем
            if (!placeShip(s, oldPos.x(), oldPos.y(), newO)) {
                // Не вышло повернуть - возвращаем
                s->topLeft = oldPos;
                s->orientation = oldOrient;
            }
        }
    }
}

bool BoardWidget::isAllDestroyed() {
    for (Ship* s : myShips) {
        if (!s->isDestroyed()) return false;
    }
    return true;
}

bool BoardWidget::autoPlaceShips() {
    clearBoard();
    for(auto s : myShips) s->topLeft = QPoint(-1,-1);

    int loopSafety = 0;
    for(auto s : myShips) {
        bool placed = false;
        while(!placed && loopSafety < 5000) {
            loopSafety++;
            int x = QRandomGenerator::global()->bounded(10);
            int y = QRandomGenerator::global()->bounded(10);
            Orientation o = (QRandomGenerator::global()->bounded(2) == 0) ? Orientation::Horizontal : Orientation::Vertical;
            if(placeShip(s, x, y, o)) placed = true;
        }
    }
    return loopSafety < 5000;
}
