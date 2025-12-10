#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow() = default;

signals:
    // Сигнал, который мы отправим, когда регистрация пройдет успешно
    void registrationSuccessful();

private slots:
    void onRegisterClicked();

private:
    QLineEdit *loginEdit;
    QLineEdit *passwordEdit;
    QPushButton *registerBtn;

    void setupUI();
};

#endif // LOGINWINDOW_H
