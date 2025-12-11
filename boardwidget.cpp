#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QRandomGenerator>
#include <algorithm>

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent), isEditable(false), showShips(true)
{
    setAcceptDrops(true);
    setMouseTracking(true);
    clearBoard();
}

QSize BoardWidget::sizeHint() const {
    return QSize(330, 330);
}

int BoardWidget::heightForWidth(int w) const {
    return w;
}

void BoardWidget::setupSizePolicy() {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void BoardWidget::clearBoard() {
    for(int i=0; i<10; ++i)
        for(int j=0; j<10; ++j)
            grid[i][j] = Empty;
    update();
}

// Хелпер для получения координат клетки с учетом отступов
QPoint BoardWidget::getGridCoord(QPoint pos) {
    int w = width();
    int h = height();
    // Учитываем отступ слева (MARGIN) и снизу (тоже MARGIN для букв)
    int side = std::min(w - MARGIN, h - MARGIN);
    if (side <= 0) return QPoint(-1, -1);

    int cellSize = side / 10;
    if (cellSize == 0) return QPoint(-1, -1);

    // Сетка начинается с X=MARGIN, Y=0
    int x = (pos.x() - MARGIN) / cellSize;
    int y = pos.y() / cellSize;

    return QPoint(x, y);
}

void BoardWidget::leaveEvent(QEvent *) {
    hoverX = -1;
    hoverY = -1;
    update();
}

void BoardWidget::mouseMoveEvent(QMouseEvent *event) {
    QPoint p = getGridCoord(event->pos());
    int nx = p.x();
    int ny = p.y();

    if (nx != hoverX || ny != hoverY) {
        if (nx >= 0 && nx < 10 && ny >= 0 && ny < 10) {
            hoverX = nx;
            hoverY = ny;
        } else {
            hoverX = -1;
            hoverY = -1;
        }
        update();
    }
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

bool BoardWidget::canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip) {
    int dx = (orient == Orientation::Horizontal) ? 1 : 0;
    int dy = (orient == Orientation::Vertical) ? 1 : 0;

    int endX = x + (size - 1) * dx;
    int endY = y + (size - 1) * dy;
    if (x < 0 || y < 0 || endX >= 10 || endY >= 10) return false;

    for (Ship* other : myShips) {
        if (other == ignoreShip) continue;
        if (!other->isPlaced()) continue;

        int ox = other->topLeft.x();
        int oy = other->topLeft.y();
        int odx = (other->orientation == Orientation::Horizontal) ? 1 : 0;
        int ody = (other->orientation == Orientation::Vertical) ? 1 : 0;

        int dangerLeft = ox - 1;
        int dangerTop = oy - 1;
        int dangerRight = ox + (other->size - 1) * odx + 1;
        int dangerBottom = oy + (other->size - 1) * ody + 1;

        for (int k = 0; k < size; ++k) {
            int cx = x + k * dx;
            int cy = y + k * dy;
            if (cx >= dangerLeft && cx <= dangerRight && cy >= dangerTop && cy <= dangerBottom) {
                return false;
            }
        }
    }
    return true;
}

bool BoardWidget::placeShip(Ship* ship, int x, int y, Orientation orient) {
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
            if (y == s->topLeft.y() && x >= s->topLeft.x() && x < s->topLeft.x() + s->size) return s;
        } else {
            if (x == s->topLeft.x() && y >= s->topLeft.y() && y < s->topLeft.y() + s->size) return s;
        }
    }
    return nullptr;
}

// --- СТАТИЧЕСКИЙ МЕТОД ОТРИСОВКИ КОРАБЛЯ (ИСПРАВЛЕННЫЙ) ---
void BoardWidget::drawShipShape(QPainter &p, int size, Orientation orient, QRect rect, bool isEnemy, bool isDestroyed)
{
    // Цвета
    QColor mainColor = isEnemy ? QColor(80, 80, 80) : QColor(140, 140, 140);
    QColor deckColor = isEnemy ? QColor(60, 60, 60) : QColor(100, 100, 100);
    QColor borderColor = Qt::black;

    if (isDestroyed) {
        mainColor = QColor(40, 40, 40);
        deckColor = QColor(20, 20, 20);
        borderColor = QColor(200, 50, 50); // Красная обводка для уничтоженного
    }

    p.save();

    if (orient == Orientation::Vertical) {
        // Поворачиваем систему координат вокруг центра прямоугольника корабля
        p.translate(rect.center());
        p.rotate(90);
        // После поворота рисуем прямоугольник, центрированный в (0,0)
        // Но ширина и высота меняются местами логически для отрисовки
        // rect.width() - это узкая сторона (клетка), rect.height() - длинная (size * клетка)
        // При повороте на 90, мы рисуем горизонтально
        int w = rect.height(); // Длина
        int h = rect.width();  // Ширина
        QRect localRect(-w/2, -h/2, w, h);

        // Рекурсивный вызов или отрисовка здесь же? Проще отрисовать здесь же.
        // Копируем логику рисования горизонтального корабля для localRect

        int gap = 4;
        QRect body = localRect.adjusted(gap, gap, -gap, -gap);

        p.setPen(QPen(borderColor, 2));
        p.setBrush(mainColor);
        p.drawRoundedRect(body, 5, 5);

        p.setPen(Qt::NoPen);
        p.setBrush(deckColor);

        for(int i=0; i<size; ++i) {
            int cx = body.left() + (body.width() / size) * i + (body.width()/size)/2;
            int cy = body.center().y();
            if (i == 1 && size > 2) p.drawRect(cx - 4, cy - 6, 8, 12);
            else p.drawEllipse(QPoint(cx, cy), 3, 3);
        }

    } else {
        // Горизонтальный - рисуем как есть
        int gap = 4;
        QRect body = rect.adjusted(gap, gap, -gap, -gap);

        p.setPen(QPen(borderColor, 2));
        p.setBrush(mainColor);
        p.drawRoundedRect(body, 5, 5);

        p.setPen(Qt::NoPen);
        p.setBrush(deckColor);

        for(int i=0; i<size; ++i) {
            int cx = body.left() + (body.width() / size) * i + (body.width()/size)/2;
            int cy = body.center().y();
            if (i == 1 && size > 2) p.drawRect(cx - 4, cy - 6, 8, 12);
            else p.drawEllipse(QPoint(cx, cy), 3, 3);
        }
    }

    p.restore();
}

void BoardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(255, 255, 255, 200));

    // Вычисляем размер стороны так же, как в getGridCoord
    int side = std::min(width() - MARGIN, height() - MARGIN);
    if (side <= 0) return;
    int cellSize = side / 10;
    int boardSize = cellSize * 10;

    // Смещаем начало координат вправо на MARGIN. Сверху отступ 0 (сетка прижата к верху).
    p.translate(MARGIN, 0);

    // Рисуем цифры (1-10) слева (в зоне отрицательных X после translate)
    p.setPen(QPen(Qt::black));
    QFont font = p.font();
    font.setBold(true);
    font.setPixelSize(14);
    p.setFont(font);

    for(int i=0; i<10; ++i) {
        // Клетка цифры находится слева от сетки
        QRect textRect(-MARGIN, i * cellSize, MARGIN, cellSize);
        p.drawText(textRect, Qt::AlignCenter, QString::number(i + 1));
    }

    // Рисуем буквы (A-J) снизу (под сеткой)
    QString letters = "ABCDEFGHIJ";
    for(int i=0; i<10; ++i) {
        QRect textRect(i * cellSize, boardSize, cellSize, MARGIN);
        p.drawText(textRect, Qt::AlignCenter, QString(letters[i]));
    }

    // Подсветка активного поля
    if (isActive) {
        p.fillRect(0, 0, boardSize, boardSize, QColor(0, 150, 255, 20));
    }

    // Сетка
    p.setPen(QPen(QColor(100, 100, 100), 1));
    for (int i = 0; i <= 10; ++i) {
        p.drawLine(i * cellSize, 0, i * cellSize, boardSize);
        p.drawLine(0, i * cellSize, boardSize, i * cellSize);
    }

    // Корабли
    for (Ship* s : myShips) {
        if (!s->isPlaced()) continue;
        if (showShips || s->isDestroyed()) {
            int w = (s->orientation == Orientation::Horizontal) ? s->size * cellSize : cellSize;
            int h = (s->orientation == Orientation::Vertical) ? s->size * cellSize : cellSize;

            // Координаты на холсте
            QRect shipRect(s->topLeft.x() * cellSize, s->topLeft.y() * cellSize, w, h);
            drawShipShape(p, s->size, s->orientation, shipRect, isEnemyBoard, s->isDestroyed());
        }
    }

    // Попадания и промахи
    for(int x=0; x<10; ++x) {
        for(int y=0; y<10; ++y) {
            int cx = x * cellSize;
            int cy = y * cellSize;

            if (grid[x][y] == Miss) {
                p.setBrush(Qt::black);
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPoint(cx + cellSize/2, cy + cellSize/2), 3, 3);
            } else if (grid[x][y] == Hit) {
                p.setPen(QPen(Qt::red, 3));
                p.drawLine(cx + 4, cy + 4, cx + cellSize - 4, cy + cellSize - 4);
                p.drawLine(cx + cellSize - 4, cy + 4, cx + 4, cy + cellSize - 4);
            }
        }
    }

    // Курсор
    if (isActive && hoverX != -1 && hoverY != -1) {
        p.setPen(QPen(QColor(255, 0, 0), 2));
        p.setBrush(Qt::NoBrush);

        int hx = hoverX * cellSize;
        int hy = hoverY * cellSize;
        int len = cellSize / 3;

        p.drawLine(hx, hy, hx + len, hy);
        p.drawLine(hx, hy, hx, hy + len);
        p.drawLine(hx + cellSize, hy, hx + cellSize - len, hy);
        p.drawLine(hx + cellSize, hy, hx + cellSize, hy + len);
        p.drawLine(hx, hy + cellSize, hx + len, hy + cellSize);
        p.drawLine(hx, hy + cellSize, hx, hy + cellSize - len);
        p.drawLine(hx + cellSize, hy + cellSize, hx + cellSize - len, hy + cellSize);
        p.drawLine(hx + cellSize, hy + cellSize, hx + cellSize, hy + cellSize - len);
    }

    // Рамка вокруг
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, boardSize, boardSize);
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
            return 2;
        }
        update();
        return 1;
    } else {
        grid[x][y] = Miss;
        update();
        return 0;
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

void BoardWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (isEditable && event->mimeData()->hasText()) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
}

void BoardWidget::dropEvent(QDropEvent *event) {
    if (!isEditable) return;
    QStringList parts = event->mimeData()->text().split(":");
    if (parts.size() != 2) return;

    QPoint gridPos = getGridCoord(event->position().toPoint());
    if (gridPos.x() == -1) return;

    int shipId = parts[0].toInt();
    Orientation orient = Orientation::Horizontal;

    Ship* targetShip = nullptr;
    for(Ship* s : myShips) { if(s->id == shipId) { targetShip = s; break; } }

    if (targetShip) {
        QPoint oldPos = targetShip->topLeft;
        targetShip->topLeft = QPoint(-1, -1);

        if (placeShip(targetShip, gridPos.x(), gridPos.y(), orient)) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
            emit shipPlaced();
        } else {
            targetShip->topLeft = oldPos;
            event->ignore();
        }
    }
}

void BoardWidget::mousePressEvent(QMouseEvent *event) {
    QPoint gridPos = getGridCoord(event->pos());
    int x = gridPos.x();
    int y = gridPos.y();

    if (x == -1) return;

    if (!isEditable) {
        if (event->button() == Qt::LeftButton) emit cellClicked(x, y);
        return;
    }

    Ship* s = getShipAt(x, y);
    if(s && event->button() == Qt::LeftButton) {
        QDrag *drag = new QDrag(this);
        QMimeData *mime = new QMimeData();
        mime->setText(QString("%1:%2").arg(s->id).arg((int)s->orientation));
        drag->setMimeData(mime);

        int cellSize = 30;
        int w = (s->orientation == Orientation::Horizontal) ? s->size * cellSize : cellSize;
        int h = (s->orientation == Orientation::Vertical) ? s->size * cellSize : cellSize;

        QPixmap pixmap(w, h);
        pixmap.fill(Qt::transparent);
        QPainter pixPainter(&pixmap);

        BoardWidget::drawShipShape(pixPainter, s->size, s->orientation, QRect(0,0,w,h), false, false);
        pixPainter.end();

        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(cellSize / 2, cellSize / 2));

        QPoint oldPos = s->topLeft;
        Orientation oldOrient = s->orientation;
        s->topLeft = QPoint(-1, -1);
        update();

        if (drag->exec(Qt::MoveAction) != Qt::MoveAction) {
            s->topLeft = oldPos;
            s->orientation = oldOrient;
            update();
        }
    } else if (s && event->button() == Qt::RightButton) {
        Orientation newO = (s->orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
        QPoint oldPos = s->topLeft;
        s->topLeft = QPoint(-1, -1);
        if (!placeShip(s, oldPos.x(), oldPos.y(), newO)) {
            s->topLeft = oldPos;
        }
        update();
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
