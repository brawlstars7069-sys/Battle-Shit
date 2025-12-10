#include "LoginWindow.h"
#include <QFormLayout>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent)
{
    setupUI();
    setWindowTitle("Морской Бой - Регистрация");
    // Фиксированный размер для лучшей центровки
    setFixedSize(300, 200);
}

void LoginWindow::setupUI()
{
    // Используем QFormLayout для более аккуратного расположения полей
    QFormLayout *formLayout = new QFormLayout();

    loginEdit = new QLineEdit(this);
    loginEdit->setPlaceholderText("Введите логин...");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Введите пароль...");
    passwordEdit->setEchoMode(QLineEdit::Password);

    formLayout->addRow("Логин:", loginEdit);
    formLayout->addRow("Пароль:", passwordEdit);

    registerBtn = new QPushButton("Зарегистрироваться", this);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("Добро пожаловать!", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 18px; margin-bottom: 10px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(registerBtn);
    mainLayout->addStretch();

    setLayout(mainLayout);

    connect(registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);
}

void LoginWindow::onRegisterClicked()
{
    // Временно проверяем, чтобы поля не были пустыми
    if(loginEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Пожалуйста, заполните все поля.");
        return;
    }

    // Имитация успешной регистрации
    // QMessageBox::information(this, "Успех", "Регистрация прошла успешно!");

    // Испускаем сигнал, чтобы главное окно могло открыться
    emit registrationSuccessful();
}
