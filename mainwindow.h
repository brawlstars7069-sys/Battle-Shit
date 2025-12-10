#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QApplication>

// Мы можем объявить класс GameWindow здесь, чтобы избежать циклической зависимости
// и сразу подготовить почву для перехода к игре.
class GameWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Слот, который будет обрабатывать запуск одиночной игры (следующий этап)
    void onSinglePlayerClicked();
    void onMultiplayerClicked();
    void onSettingsClicked();
    void onExitClicked();

private:
    void setupUI();
};

#endif // MAINWINDOW_H
