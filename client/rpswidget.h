#ifndef RPSWIDGET_H
#define RPSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPoint>

// Типы выбора
enum class RPSType { Rock, Paper, Scissors, None };

// Класс отдельного элемента (Камень, Ножницы или Бумага)
class RPSItem : public QWidget {
    Q_OBJECT
public:
    explicit RPSItem(RPSType type, QWidget *parent = nullptr);
    void setDisabledState(bool disabled); // Визуальное отключение

signals:
    void clicked(RPSType type);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void updateShake();

private:
    RPSType type;
    QTimer *shakeTimer;
    QPoint shakeOffset;
    bool isHovered;
    bool isDisabled;

    void drawRock(QPainter &p, int w, int h);
    void drawPaper(QPainter &p, int w, int h);
    void drawScissors(QPainter &p, int w, int h);
};

// Класс самого виджета мини-игры (Overlay)
class RPSWidget : public QWidget {
    Q_OBJECT
public:
    explicit RPSWidget(QWidget *parent = nullptr);

signals:
    void gameFinished(bool playerWon); // Сигнал завершения: true если игрок выиграл и ходит первым

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override; // Чтобы всегда покрывать родителя

private slots:
    void onItemClicked(RPSType playerChoice);
    void resetRound();

private:
    QLabel *statusLabel;
    QLabel *vsLabel;
    RPSItem *itemRock;
    RPSItem *itemPaper;
    RPSItem *itemScissors;

    // Для отображения выбора бота
    RPSType botChoice;
    bool showResult;

    void processRound(RPSType playerChoice);
};

#endif // RPSWIDGET_H
