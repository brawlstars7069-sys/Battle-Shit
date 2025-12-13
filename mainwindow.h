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
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QGridLayout>

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
    // Обновляем геометрию виджетов при ресайзе
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onSinglePlayerClicked();
    void onMultiplayerClicked();
    void onSettingsClicked();
    void onExitClicked();

    // Новые слоты
    void onBackFromSettingsClicked();
    void onAvatarSelected(const QString &path);
    void onChangeAvatarClicked();

private:
    void setupUI();
    void setupMenuContainer();
    void setupSettingsContainer();

    // Методы анимации
    void slideToSettings();
    void slideToMenu();

    QPoint mousePos; // Текущая позиция мыши

    // Контейнеры интерфейса
    QWidget *menuContainer;
    QWidget *settingsContainer;

    // Элементы настроек
    QWidget *avatarSelectionWidget; // Виджет с сеткой аватаров
    QPushButton *btnChangeAvatar;
    QLabel *currentAvatarPreview;

    // Состояние
    QString selectedAvatarPath;

    // Анимации
    QPropertyAnimation *animMenu;
    QPropertyAnimation *animSettings;
};

#endif // MAINWINDOW_H
