#include "boardwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QRandomGenerator>
#include <algorithm>
#include <QTimer>

BoardWidget::BoardWidget(QWidget *parent)
    : QWidget(parent), isEditable(false), showShips(true)
{
    setAcceptDrops(true);
    setMouseTracking(true);
    clearBoard();

    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &BoardWidget::updateAnimation);
}

QSize BoardWidget::sizeHint() const { return QSize(330, 330); }
int BoardWidget::heightForWidth(int w) const { return w; }
void BoardWidget::setupSizePolicy() { setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }

void BoardWidget::clearBoard() {
    for(int i=0; i<10; ++i)
        for(int j=0; j<10; ++j)
            grid[i][j] = Empty;
    update();
}

// --- ВАЖНЫЙ МЕТОД ДЛЯ МУЛЬТИПЛЕЕРА ---
void BoardWidget::setCellState(int x, int y, CellState state) {
    if (x >= 0 && x < 10 && y >= 0 && y < 10) {
        grid[x][y] = state;
        update();
    }
}
// -------------------------------------

void BoardWidget::setFog(bool active) {
    isFoggy = active;
    update();
}

void BoardWidget::setHighlight(QPoint pos) {
    highlightPos = pos;
    update();
}

QPoint BoardWidget::getGridCoord(QPoint pos) {
    int w = width();
    int h = height();
    int side = std::min(w - MARGIN, h - MARGIN);
    if (side <= 0) return QPoint(-1, -1);

    int cellSize = side / 10;
    if (cellSize == 0) return QPoint(-1, -1);

    if (pos.x() < MARGIN || pos.y() < 0) return QPoint(-1, -1);

    int x = (pos.x() - MARGIN) / cellSize;
    int y = pos.y() / cellSize;

    if (x < 0 || x >= 10 || y < 0 || y >= 10) return QPoint(-1, -1);

    return QPoint(x, y);
}

void BoardWidget::leaveEvent(QEvent *) { hoverX = -1; hoverY = -1; update(); }

void BoardWidget::mouseMoveEvent(QMouseEvent *event) {
    if (isEditable || isActive) {
        QPoint p = getGridCoord(event->pos());
        if (p.x() != hoverX || p.y() != hoverY) {
            hoverX = p.x(); hoverY = p.y(); update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void BoardWidget::setEditable(bool editable) {
    isEditable = editable; setAcceptDrops(editable);
    setCursor(isEditable ? Qt::OpenHandCursor : Qt::ArrowCursor);
}
void BoardWidget::setShips(const QVector<Ship *> &ships) { myShips = ships; }
void BoardWidget::setShowShips(bool show) { showShips = show; update(); }

bool BoardWidget::canPlace(int x, int y, int size, Orientation orient, Ship* ignoreShip) {
    int dx = (orient == Orientation::Horizontal) ? 1 : 0;
    int dy = (orient == Orientation::Vertical) ? 1 : 0;
    if (x < 0 || y < 0 || x >= 10 || y >= 10) return false;
    if (orient == Orientation::Horizontal && (x + size) > 10) return false;
    if (orient == Orientation::Vertical && (y + size) > 10) return false;
    for (int i = 0; i < size; ++i) {
        int currentX = x + i * dx; int currentY = y + i * dy;
        for (int checkX = currentX - 1; checkX <= currentX + 1; ++checkX) {
            for (int checkY = currentY - 1; checkY <= currentY + 1; ++checkY) {
                if (checkX < 0 || checkX >= 10 || checkY < 0 || checkY >= 10) continue;
                Ship* neighbor = getShipAt(checkX, checkY);
                if (neighbor && neighbor != ignoreShip) return false;
            }
        }
    }
    return true;
}

bool BoardWidget::placeShip(Ship* ship, int x, int y, Orientation orient) {
    if (canPlace(x, y, ship->size, orient, ship)) {
        if (ship->isPlaced()) removeShipCells(ship);
        ship->topLeft = QPoint(x, y); ship->orientation = orient;
        placeShipCells(ship); update(); return true;
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

void BoardWidget::drawShipShape(QPainter &p, int size, Orientation orient, QRect rect, bool isEnemy, bool isDestroyed)
{
    p.save();
    if (orient == Orientation::Vertical) {
        p.translate(rect.center()); p.rotate(90); p.translate(-rect.center());
        int cx = rect.center().x(); int cy = rect.center().y();
        int w = rect.height(); int h = rect.width();
        rect = QRect(cx - w/2, cy - h/2, w, h);
    }

    int x = rect.x(); int y = rect.y(); int w = rect.width(); int h = rect.height();
    int u = h / 8; if (u < 1) u = 1;

    QColor cOutline = Qt::black;
    QColor cMain    = isEnemy ? QColor(80, 80, 80)    : QColor(200, 200, 200);
    QColor cShadow  = isEnemy ? QColor(50, 50, 50)    : QColor(140, 140, 140);
    QColor cDeck    = isEnemy ? QColor(100, 100, 100) : QColor(230, 230, 230);
    QColor cAccent  = isEnemy ? QColor(200, 60, 60)   : QColor(60, 150, 220);

    if (isDestroyed) {
        cMain   = QColor(60, 50, 50); cShadow = QColor(30, 20, 20);
        cDeck   = QColor(80, 60, 60); cAccent = QColor(40, 40, 40);
    }

    p.setPen(Qt::NoPen);
    int bodyTop = y + u; int bodyH = 6 * u;
    int sternX = x + u; int noseStart = x + w - 3*u;

    p.setBrush(cOutline);
    p.drawRect(sternX, bodyTop, u, bodyH);
    p.drawRect(sternX + u, bodyTop, (noseStart - sternX - u), bodyH);
    p.drawRect(noseStart, bodyTop + u, u, bodyH - 2*u);
    p.drawRect(noseStart + u, bodyTop + 2*u, u, bodyH - 4*u);

    p.setBrush(cMain);
    p.drawRect(sternX + u, bodyTop + u, (noseStart - sternX - u), bodyH - 2*u);
    p.drawRect(noseStart, bodyTop + 2*u, u, bodyH - 4*u);

    p.setBrush(cShadow);
    p.drawRect(sternX + u, bodyTop + bodyH - 2*u, (noseStart - sternX), u);

    p.setBrush(cDeck);
    p.drawRect(sternX + u, bodyTop + u, (noseStart - sternX - u), u);

    int cx = x + w / 2; int cy = y + h / 2;
    int bridgeX = sternX + 2*u; if (size == 1) bridgeX = cx - u;

    p.setBrush(cShadow); p.drawRect(bridgeX, cy - u, 3*u, 2*u);
    p.setBrush(cDeck); p.drawRect(bridgeX + u, cy - u, u, u);

    p.setBrush(cAccent);
    if (size >= 2) p.drawRect(noseStart - 2*u, cy - u, 2*u, 2*u);
    if (size >= 3) p.drawRect(sternX + u, cy - u, 2*u, 2*u);

    p.setBrush(Qt::black); p.drawRect(sternX - u/2, cy - u/2, u, u);
    p.restore();
}

bool BoardWidget::hasShipAt(int x, int y) { return getShipAt(x, y) != nullptr; }

bool BoardWidget::canShootAt(int x, int y) {
    if (x < 0 || x >= 10 || y < 0 || y >= 10) return false;
    return (grid[x][y] == Empty || grid[x][y] == ShipCell);
}

void BoardWidget::animateShot(int x, int y) {
    if (x < 0 || x > 9 || y < 0 || y > 9) return;
    if (currentAnim.state == AnimState::Falling) return;

    int side = std::min(width() - MARGIN, height() - MARGIN);
    int cellSize = side / 10;

    currentAnim.state = AnimState::Falling;
    currentAnim.gridPos = QPoint(x, y);

    float startX = x * cellSize + cellSize / 2.0;
    float startY = -40;

    currentAnim.targetY = y * cellSize + cellSize / 2.0;
    currentAnim.currentPos = QPointF(startX, startY);
    currentAnim.speedY = 15.0;
    currentAnim.frame = 0;

    // В мультиплеере мы не знаем, попали или нет, пока не получим ответ.
    // Пока считаем false, реальный эффект (взрыв) будет по приходу пакета fire_result
    // Или, если это локальный бот, проверяем hasShipAt
    if (!isEnemyBoard) {
        currentAnim.isHit = hasShipAt(x, y); // В нас стреляют - мы знаем
    } else {
        currentAnim.isHit = false; // В врага стреляем - узнаем позже
    }

    animTimer->start(16);
}

void BoardWidget::updateAnimation() {
    if (currentAnim.state == AnimState::Falling) {
        currentAnim.currentPos.ry() += currentAnim.speedY;
        currentAnim.speedY += 1.5;
        if (currentAnim.currentPos.y() >= currentAnim.targetY) {
            currentAnim.currentPos.ry() = currentAnim.targetY;
            currentAnim.state = AnimState::Exploding;
            currentAnim.frame = 0;
            emit missileImpact(currentAnim.gridPos.x(), currentAnim.gridPos.y(), currentAnim.isHit);
        }
    }
    else if (currentAnim.state == AnimState::Exploding) {
        currentAnim.frame++;
        if (currentAnim.frame > 20) {
            currentAnim.state = AnimState::Idle;
            animTimer->stop();
        }
    }
    update();
}

void BoardWidget::drawMissile(QPainter &p) {
    p.save();
    QPointF pos = currentAnim.currentPos;
    int w = 12; int h = 24; QRectF r(pos.x() - w/2, pos.y() - h/2, w, h);
    if (currentAnim.speedY > 20) {
        p.setPen(Qt::NoPen); QColor smoke(200, 200, 200, 150);
        for(int i=1; i<=3; ++i) { p.setBrush(smoke); p.drawRect(pos.x() - w/2 + 2, pos.y() - h - i*10, w-4, 8); }
    }
    p.setPen(QPen(Qt::black, 2)); p.setBrush(currentAnim.isHit ? QColor(255, 50, 50) : QColor(200, 200, 200));
    p.drawRect(r); p.setBrush(Qt::black); p.drawRect(pos.x() - w/2 + 2, pos.y() + h/2, w - 4, 4);
    p.restore();
}

void BoardWidget::drawExplosion(QPainter &p) {
    p.save();
    QPointF pos = currentAnim.currentPos; int f = currentAnim.frame;
    QColor color1 = currentAnim.isHit ? QColor(255, 200, 0) : QColor(100, 200, 255);
    QColor color2 = currentAnim.isHit ? QColor(255, 50, 0) : QColor(50, 100, 200);
    int radius = 5 + f * 2;
    p.setPen(Qt::NoPen);
    p.setBrush(color2);
    p.drawRect(pos.x() - radius, pos.y() - radius, radius*2, radius*2);
    int innerR = radius * 0.6;
    p.setBrush(color1);
    p.drawRect(pos.x() - innerR, pos.y() - innerR, innerR*2, innerR*2);
    if (f < 15) {
        p.setBrush(currentAnim.isHit ? Qt::black : Qt::white); int partDist = radius + 5;
        p.drawRect(pos.x() - partDist, pos.y() - partDist, 4, 4); p.drawRect(pos.x() + partDist, pos.y() - partDist, 4, 4);
        p.drawRect(pos.x() - partDist, pos.y() + partDist, 4, 4); p.drawRect(pos.x() + partDist, pos.y() + partDist, 4, 4);
    }
    p.restore();
}

void BoardWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), QColor(255, 255, 255, 200));
    int side = std::min(width() - MARGIN, height() - MARGIN);
    if (side <= 0) return;
    int cellSize = side / 10;
    int boardSize = cellSize * 10;
    p.translate(MARGIN, 0);

    p.setPen(QPen(Qt::black));
    QFont font = p.font(); font.setBold(true); font.setPixelSize(14); p.setFont(font);
    for(int i=0; i<10; ++i) {
        QRect textRect(-MARGIN, i * cellSize, MARGIN, cellSize);
        p.drawText(textRect, Qt::AlignCenter, QString::number(i + 1));
    }
    QString letters = "ABCDEFGHIJ";
    for(int i=0; i<10; ++i) {
        QRect textRect(i * cellSize, boardSize, cellSize, MARGIN);
        p.drawText(textRect, Qt::AlignCenter, QString(letters[i]));
    }

    if (isActive) p.fillRect(0, 0, boardSize, boardSize, QColor(0, 150, 255, 20));

    p.setPen(QPen(QColor(100, 100, 100), 1));
    for (int i = 0; i <= 10; ++i) {
        p.drawLine(i * cellSize, 0, i * cellSize, boardSize);
        p.drawLine(0, i * cellSize, boardSize, i * cellSize);
    }

    for (Ship* s : myShips) {
        if (!s->isPlaced()) continue;
        // Показываем корабли только если это наши, или они убиты, или включен режим отладки/конца игры
        if (showShips || s->isDestroyed()) {
            int w = (s->orientation == Orientation::Horizontal) ? s->size * cellSize : cellSize;
            int h = (s->orientation == Orientation::Vertical) ? s->size * cellSize : cellSize;
            QRect shipRect(s->topLeft.x() * cellSize, s->topLeft.y() * cellSize, w, h);
            drawShipShape(p, s->size, s->orientation, shipRect, isEnemyBoard, s->isDestroyed());
        }
    }

    for(int x=0; x<10; ++x) {
        for(int y=0; y<10; ++y) {
            int cx = x * cellSize; int cy = y * cellSize;
            if (grid[x][y] == Miss) {
                p.setBrush(Qt::black); p.setPen(Qt::NoPen);
                p.drawRect(cx + cellSize/2 - 2, cy + cellSize/2 - 2, 4, 4);
            } else if (grid[x][y] == Hit) {
                p.setPen(QPen(Qt::red, 3));
                p.drawLine(cx + 4, cy + 4, cx + cellSize - 4, cy + cellSize - 4);
                p.drawLine(cx + cellSize - 4, cy + 4, cx + 4, cy + cellSize - 4);
            }
        }
    }

    if (highlightPos.x() >= 0 && highlightPos.y() >= 0) {
        int cx = highlightPos.x() * cellSize;
        int cy = highlightPos.y() * cellSize;
        p.setPen(QPen(QColor(46, 204, 113), 3)); p.setBrush(Qt::NoBrush);
        int len = cellSize / 3;
        p.drawLine(cx, cy, cx + len, cy); p.drawLine(cx, cy, cx, cy + len);
        p.drawLine(cx + cellSize, cy, cx + cellSize - len, cy); p.drawLine(cx + cellSize, cy, cx + cellSize, cy + len);
        p.drawLine(cx, cy + cellSize, cx + len, cy + cellSize); p.drawLine(cx, cy + cellSize, cx, cy + cellSize - len);
        p.drawLine(cx + cellSize, cy + cellSize, cx + cellSize - len, cy + cellSize); p.drawLine(cx + cellSize, cy + cellSize, cx + cellSize, cy + cellSize - len);
        p.setPen(QPen(QColor(46, 204, 113, 100), 2));
        p.drawLine(cx + cellSize/2, cy + 5, cx + cellSize/2, cy + cellSize - 5);
        p.drawLine(cx + 5, cy + cellSize/2, cx + cellSize - 5, cy + cellSize/2);
    }

    if (isActive && hoverX != -1 && hoverY != -1) {
        p.setPen(QPen(QColor(255, 0, 0), 2)); p.setBrush(Qt::NoBrush);
        int hx = hoverX * cellSize; int hy = hoverY * cellSize; int len = cellSize / 3;
        p.drawLine(hx, hy, hx + len, hy); p.drawLine(hx, hy, hx, hy + len);
        p.drawLine(hx + cellSize, hy, hx + cellSize - len, hy); p.drawLine(hx + cellSize, hy, hx + cellSize, hy + len);
        p.drawLine(hx, hy + cellSize, hx + len, hy + cellSize); p.drawLine(hx, hy + cellSize, hx, hy + cellSize - len);
        p.drawLine(hx + cellSize, hy + cellSize, hx + cellSize - len, hy + cellSize); p.drawLine(hx + cellSize, hy + cellSize, hx + cellSize, hy + cellSize - len);
    }

    if (isFoggy) {
        p.fillRect(0, 0, boardSize, boardSize, QColor(200, 200, 200, 220));
        p.setPen(Qt::black);
        p.setFont(QFont("Courier New", 20, QFont::Bold));
        p.drawText(QRect(0, 0, boardSize, boardSize), Qt::AlignCenter, "ТУМАН");
    }

    p.setPen(QPen(Qt::black, 2)); p.setBrush(Qt::NoBrush);
    p.drawRect(0, 0, boardSize, boardSize);

    if (currentAnim.state != AnimState::Idle) {
        if (currentAnim.state == AnimState::Falling) drawMissile(p);
        else if (currentAnim.state == AnimState::Exploding) drawExplosion(p);
    }
}

int BoardWidget::receiveShot(int x, int y) {
    if (x < 0 || x >= 10 || y < 0 || y >= 10) return -1;
    // Если уже стреляли
    if (grid[x][y] != Empty && grid[x][y] != ShipCell) return -1;

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
    int x1 = s->topLeft.x(); int y1 = s->topLeft.y();
    int x2 = x1 + (s->size-1)*dx; int y2 = y1 + (s->size-1)*dy;
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
        event->setDropAction(Qt::MoveAction); event->accept();
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
            event->setDropAction(Qt::MoveAction); event->accept(); emit shipPlaced();
        } else {
            targetShip->topLeft = oldPos; event->ignore();
        }
    }
}

void BoardWidget::mousePressEvent(QMouseEvent *event) {
    QPoint gridPos = getGridCoord(event->pos());
    int x = gridPos.x(); int y = gridPos.y();
    if (x == -1) return;

    if (!isEditable) {
        if (event->button() == Qt::LeftButton) emit cellClicked(x, y);
        return;
    }
    Ship* s = getShipAt(x, y);
    if(s && event->button() == Qt::LeftButton) {
        QDrag *drag = new QDrag(this); QMimeData *mime = new QMimeData();
        mime->setText(QString("%1:%2").arg(s->id).arg((int)s->orientation)); drag->setMimeData(mime);
        int cellSize = 30; int w = (s->orientation == Orientation::Horizontal) ? s->size * cellSize : cellSize;
        int h = (s->orientation == Orientation::Vertical) ? s->size * cellSize : cellSize;
        QPixmap pixmap(w, h); pixmap.fill(Qt::transparent); QPainter pixPainter(&pixmap);
        BoardWidget::drawShipShape(pixPainter, s->size, s->orientation, QRect(0,0,w,h), false, false);
        pixPainter.end(); drag->setPixmap(pixmap); drag->setHotSpot(QPoint(cellSize / 2, cellSize / 2));
        QPoint oldPos = s->topLeft; Orientation oldOrient = s->orientation;
        s->topLeft = QPoint(-1, -1); update();
        if (drag->exec(Qt::MoveAction) != Qt::MoveAction) {
            s->topLeft = oldPos; s->orientation = oldOrient; update();
        }
    } else if (s && event->button() == Qt::RightButton) {
        Orientation newO = (s->orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
        QPoint oldPos = s->topLeft; s->topLeft = QPoint(-1, -1);
        if (!placeShip(s, oldPos.x(), oldPos.y(), newO)) { s->topLeft = oldPos; } update();
    }
}

bool BoardWidget::isAllDestroyed() {
    for (Ship* s : myShips) { if (!s->isDestroyed()) return false; } return true;
}

bool BoardWidget::autoPlaceShips() {
    clearBoard();
    for(auto s : myShips) s->topLeft = QPoint(-1,-1);
    int loopSafety = 0;
    for(auto s : myShips) {
        bool placed = false;
        while(!placed && loopSafety < 5000) {
            loopSafety++;
            int x = QRandomGenerator::global()->bounded(10); int y = QRandomGenerator::global()->bounded(10);
            Orientation o = (QRandomGenerator::global()->bounded(2) == 0) ? Orientation::Horizontal : Orientation::Vertical;
            if(placeShip(s, x, y, o)) placed = true;
        }
    }
    return loopSafety < 5000;
}

void BoardWidget::placeShipCells(Ship* ship) {
    if (!ship->isPlaced()) return;
    int dx = (ship->orientation == Orientation::Horizontal) ? 1 : 0;
    int dy = (ship->orientation == Orientation::Vertical) ? 1 : 0;
    for(int i=0; i<ship->size; ++i) {
        int cellX = ship->topLeft.x() + i * dx; int cellY = ship->topLeft.y() + i * dy;
        if (cellX >= 0 && cellX < 10 && cellY >= 0 && cellY < 10) grid[cellX][cellY] = ShipCell;
    }
}

void BoardWidget::removeShipCells(Ship* ship) {
    if (!ship->isPlaced()) return;
    int dx = (ship->orientation == Orientation::Horizontal) ? 1 : 0;
    int dy = (ship->orientation == Orientation::Vertical) ? 1 : 0;
    for(int i=0; i<ship->size; ++i) {
        int cellX = ship->topLeft.x() + i * dx; int cellY = ship->topLeft.y() + i * dy;
        if (cellX >= 0 && cellX < 10 && cellY >= 0 && cellY < 10) {
            if (grid[cellX][cellY] == ShipCell) grid[cellX][cellY] = Empty;
        }
    }
}
