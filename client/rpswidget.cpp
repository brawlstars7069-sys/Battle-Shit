#include "RPSWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QPropertyAnimation>

// --- RPSItem (Элемент выбора) ---

RPSItem::RPSItem(RPSType type, QWidget *parent)
    : QWidget(parent), type(type), isHovered(false), isDisabled(false)
{
    setFixedSize(100, 100);
    setCursor(Qt::PointingHandCursor);

    shakeTimer = new QTimer(this);
    shakeTimer->setInterval(30);
    connect(shakeTimer, &QTimer::timeout, this, &RPSItem::updateShake);
}

void RPSItem::setDisabledState(bool disabled) {
    isDisabled = disabled;
    setCursor(disabled ? Qt::ArrowCursor : Qt::PointingHandCursor);
    if (disabled) shakeTimer->stop();
    update();
}

void RPSItem::enterEvent(QEnterEvent *) {
    if (isDisabled) return;
    isHovered = true;
    shakeTimer->start();
}

void RPSItem::leaveEvent(QEvent *) {
    isHovered = false;
    shakeTimer->stop();
    shakeOffset = QPoint(0, 0);
    update();
}

void RPSItem::mousePressEvent(QMouseEvent *event) {
    if (isDisabled) return;
    if (event->button() == Qt::LeftButton) {
        emit clicked(type);
    }
}

void RPSItem::updateShake() {
    // Генерируем случайное смещение от -2 до 2 пикселей
    int dx = QRandomGenerator::global()->bounded(5) - 2;
    int dy = QRandomGenerator::global()->bounded(5) - 2;
    shakeOffset = QPoint(dx, dy);
    update();
}

void RPSItem::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Смещаем painter для эффекта тряски
    p.translate(shakeOffset);

    int w = width();
    int h = height();

    // Фон кнопки
    if (isHovered && !isDisabled) {
        p.setBrush(QColor(255, 255, 255, 50));
        p.setPen(QPen(Qt::white, 2, Qt::DashLine));
        p.drawRect(2, 2, w-4, h-4);
    }

    // Отрисовка иконок в стиле 8-бит
    if (type == RPSType::Rock) drawRock(p, w, h);
    else if (type == RPSType::Paper) drawPaper(p, w, h);
    else if (type == RPSType::Scissors) drawScissors(p, w, h);
}

void RPSItem::drawRock(QPainter &p, int w, int h) {
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(120, 120, 120));
    // Простой "камень"
    QPolygon poly;
    poly << QPoint(w*0.3, h*0.2) << QPoint(w*0.7, h*0.2)
         << QPoint(w*0.9, h*0.5) << QPoint(w*0.8, h*0.8)
         << QPoint(w*0.2, h*0.8) << QPoint(w*0.1, h*0.5);
    p.drawPolygon(poly);

    // Блики/тени для объема
    p.setBrush(QColor(150, 150, 150));
    p.drawRect(w*0.3, h*0.3, w*0.2, h*0.2);
}

void RPSItem::drawPaper(QPainter &p, int w, int h) {
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(240, 240, 220)); // Бумага

    p.drawRect(w*0.25, h*0.2, w*0.5, h*0.6);

    // Линии текста
    p.setPen(QColor(100, 100, 100));
    for(int i=0; i<3; i++) {
        p.drawLine(w*0.35, h*0.35 + i*15, w*0.65, h*0.35 + i*15);
    }
}

void RPSItem::drawScissors(QPainter &p, int w, int h) {
    p.setPen(QPen(Qt::black, 2));
    p.setBrush(QColor(180, 50, 50)); // Красные ручки

    // Ручки
    p.drawEllipse(QPoint(w*0.3, h*0.75), 10, 10);
    p.drawEllipse(QPoint(w*0.7, h*0.75), 10, 10);

    // Лезвия
    p.setBrush(QColor(200, 200, 200));
    QPolygon blade1;
    blade1 << QPoint(w*0.3, h*0.75) << QPoint(w*0.7, h*0.2) << QPoint(w*0.75, h*0.25) << QPoint(w*0.35, h*0.8);
    p.drawPolygon(blade1);

    QPolygon blade2;
    blade2 << QPoint(w*0.7, h*0.75) << QPoint(w*0.3, h*0.2) << QPoint(w*0.25, h*0.25) << QPoint(w*0.65, h*0.8);
    p.drawPolygon(blade2);

    // Винтик
    p.setBrush(Qt::black);
    p.drawEllipse(QPoint(w*0.5, h*0.5), 3, 3);
}

// --- RPSWidget (Окно) ---

RPSWidget::RPSWidget(QWidget *parent)
    : QWidget(parent), botChoice(RPSType::None), showResult(false)
{
    // Важно: наложение должно быть поверх всего, но внутри родителя
    if (parent) resize(parent->size());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(30);

    // Заголовок
    statusLabel = new QLabel("КТО ХОДИТ ПЕРВЫМ?\nВЫБЕРИТЕ ОРУЖИЕ!", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: white; "
        "background-color: rgba(0,0,0,150); padding: 10px; border-radius: 5px;"
        "font-family: 'Courier New';"
        );

    // Контейнер для кнопок
    QWidget *itemsContainer = new QWidget(this);
    itemsContainer->setStyleSheet("background: transparent;");
    QHBoxLayout *itemsLayout = new QHBoxLayout(itemsContainer);
    itemsLayout->setSpacing(40);

    itemRock = new RPSItem(RPSType::Rock, this);
    itemPaper = new RPSItem(RPSType::Paper, this);
    itemScissors = new RPSItem(RPSType::Scissors, this);

    connect(itemRock, &RPSItem::clicked, this, &RPSWidget::onItemClicked);
    connect(itemPaper, &RPSItem::clicked, this, &RPSWidget::onItemClicked);
    connect(itemScissors, &RPSItem::clicked, this, &RPSWidget::onItemClicked);

    itemsLayout->addWidget(itemRock);
    itemsLayout->addWidget(itemPaper);
    itemsLayout->addWidget(itemScissors);

    // Лейбл для показа выбора бота (пока скрыт)
    vsLabel = new QLabel("", this);
    vsLabel->setAlignment(Qt::AlignCenter);
    vsLabel->setStyleSheet("font-size: 40px; color: yellow; font-weight: bold; font-family: 'Courier New';");
    vsLabel->hide();

    mainLayout->addStretch(1);
    mainLayout->addWidget(statusLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(itemsContainer);
    mainLayout->addWidget(vsLabel);
    mainLayout->addStretch(1);
}

void RPSWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    // Затемнение экрана
    p.fillRect(rect(), QColor(0, 0, 0, 180));
}

void RPSWidget::resizeEvent(QResizeEvent *) {
    if (parentWidget()) resize(parentWidget()->size());
}

void RPSWidget::onItemClicked(RPSType playerChoice) {
    // Блокируем выбор
    itemRock->setDisabledState(true);
    itemPaper->setDisabledState(true);
    itemScissors->setDisabledState(true);

    processRound(playerChoice);
}

void RPSWidget::processRound(RPSType playerChoice) {
    // Выбор бота
    int rnd = QRandomGenerator::global()->bounded(3);
    RPSType bot = static_cast<RPSType>(rnd);

    QString botStr;
    if (bot == RPSType::Rock) botStr = "БОТ: КАМЕНЬ";
    else if (bot == RPSType::Paper) botStr = "БОТ: БУМАГА";
    else botStr = "БОТ: НОЖНИЦЫ";

    vsLabel->setText(botStr);
    vsLabel->show();

    // Определение победителя
    // 0=Draw, 1=Player Win, -1=Bot Win
    int result = 0;

    if (playerChoice == bot) {
        result = 0;
    } else if (
        (playerChoice == RPSType::Rock && bot == RPSType::Scissors) ||
        (playerChoice == RPSType::Scissors && bot == RPSType::Paper) ||
        (playerChoice == RPSType::Paper && bot == RPSType::Rock)
        ) {
        result = 1;
    } else {
        result = -1;
    }

    if (result == 0) {
        statusLabel->setText("НИЧЬЯ!\nЕЩЕ РАЗ...");
        statusLabel->setStyleSheet("color: #f1c40f; font-size: 24px; background-color: rgba(0,0,0,150); padding: 10px; font-family: 'Courier New'; font-weight: bold;");
        QTimer::singleShot(1500, this, &RPSWidget::resetRound);
    } else {
        bool playerWon = (result == 1);
        if (playerWon) {
            statusLabel->setText("ВЫ ПОБЕДИЛИ!\nВЫ ХОДИТЕ ПЕРВЫМ!");
            statusLabel->setStyleSheet("color: #2ecc71; font-size: 24px; background-color: rgba(0,0,0,150); padding: 10px; font-family: 'Courier New'; font-weight: bold;");
        } else {
            statusLabel->setText("БОТ ПОБЕДИЛ!\nОН ХОДИТ ПЕРВЫМ.");
            statusLabel->setStyleSheet("color: #e74c3c; font-size: 24px; background-color: rgba(0,0,0,150); padding: 10px; font-family: 'Courier New'; font-weight: bold;");
        }

        // Задержка перед началом игры
        QTimer::singleShot(2000, [this, playerWon](){
            emit gameFinished(playerWon);
            this->hide();
            this->deleteLater();
        });
    }
}

void RPSWidget::resetRound() {
    vsLabel->hide();
    statusLabel->setText("ВЫБЕРИТЕ ОРУЖИЕ!");
    statusLabel->setStyleSheet("color: white; font-size: 24px; background-color: rgba(0,0,0,150); padding: 10px; font-family: 'Courier New'; font-weight: bold;");

    itemRock->setDisabledState(false);
    itemPaper->setDisabledState(false);
    itemScissors->setDisabledState(false);
}
