#ifndef RPSWIDGET_H
#define RPSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPoint>

enum class RPSType { Rock, Paper, Scissors, None };

class RPSItem : public QWidget {
    Q_OBJECT
public:
    explicit RPSItem(RPSType type, QWidget *parent = nullptr);
    void setDisabledState(bool disabled);

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

class RPSWidget : public QWidget {
    Q_OBJECT
public:
    explicit RPSWidget(QWidget *parent = nullptr);

    // Переключатель режима
    void setMultiplayerMode(bool active);

    // Методы для сетевого режима
    void resolveRound(RPSType myChoice, RPSType oppChoice);

signals:
    void gameFinished(bool playerWon);
    void choiceMade(RPSType choice);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onItemClicked(RPSType playerChoice);
    void resetRound();

private:
    QLabel *statusLabel;
    QLabel *vsLabel;
    RPSItem *itemRock;
    RPSItem *itemPaper;
    RPSItem *itemScissors;

    bool isMultiplayerMode; // Флаг режима

    void processBotRound(RPSType playerChoice); // Для одиночной игры
    void showOutcome(int result);
};

#endif // RPSWIDGET_H
