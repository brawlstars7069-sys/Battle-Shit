#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>
#include <QMouseEvent>
#include <QPoint>

class GameWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    // Отрисовка фона
    void paintEvent(QPaintEvent *event) override;
    // Отслеживание мыши для эффекта параллакса
    void mouseMoveEvent(QMouseEvent *event) override;

private slots:
    void onSinglePlayerClicked();
    void onMultiplayerClicked();
    void onSettingsClicked();
    void onExitClicked();

private:
    void setupUI();
    QPoint mousePos; // Текущая позиция мыши
};

#endif // MAINWINDOW_H
