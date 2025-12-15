#include "loginwindow.h"
#include <QFormLayout>
#include <QRegularExpression>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
{
    // Qt::Window - чтобы это было отдельное окно (поверх основного)
    // Qt::FramelessWindowHint - можно убрать рамку ОС для стиля, но это опционально.
    // Если нужна рамка с крестиком, уберите FramelessWindowHint.
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    setupUI();
    setWindowTitle("РЕГИСТРАЦИЯ");
    setFixedSize(350, 250);
}

void LoginWindow::setupUI()
{
    // --- СТИЛИЗАЦИЯ ПОД ИГРУ ---
    // Серый фон, но шрифт и кнопки как в игре
    this->setStyleSheet(
        "QWidget {"
        "   background-color: #dcdcdc;" /* Серый цвет */
        "   font-family: 'Courier New';"
        "   color: #333;"
        "}"
        "QLineEdit {"
        "   background-color: #fff;"
        "   border: 2px solid #555;"
        "   padding: 5px;"
        "   font-size: 14px;"
        "}"
        "QPushButton {"
        "   background-color: #e8e0d5;"
        "   border: 2px solid #8c8c8c;"
        "   font-weight: bold;"
        "   padding: 8px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #dcd0c0;"
        "   border: 2px solid #333;"
        "}"
        );

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);

    loginEdit = new QLineEdit(this);
    loginEdit->setPlaceholderText("Логин (3-15 симв.)");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Пароль");
    passwordEdit->setEchoMode(QLineEdit::Password);

    QLabel *lblLogin = new QLabel("ЛОГИН:", this);
    lblLogin->setStyleSheet("font-weight: bold;");
    QLabel *lblPass = new QLabel("ПАРОЛЬ:", this);
    lblPass->setStyleSheet("font-weight: bold;");

    formLayout->addRow(lblLogin, loginEdit);
    formLayout->addRow(lblPass, passwordEdit);

    registerBtn = new QPushButton("ЗАРЕГИСТРИРОВАТЬСЯ", this);
    registerBtn->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    QLabel *titleLabel = new QLabel("ДОБРО ПОЖАЛОВАТЬ!", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 20px; border-bottom: 2px dashed #555; padding-bottom: 10px;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(registerBtn);

    setLayout(mainLayout);

    connect(registerBtn, &QPushButton::clicked, this, &LoginWindow::onRegisterClicked);
}

bool LoginWindow::validateInput(const QString &text) {
    // Регулярное выражение:
    // ^ - начало строки
    // [a-zA-Z0-9.] - разрешенные символы (английские буквы, цифры, точка)
    // {3,15} - длина от 3 до 15
    // $ - конец строки
    static QRegularExpression regex("^[a-zA-Z0-9.]{3,15}$");
    QRegularExpressionMatch match = regex.match(text);
    return match.hasMatch();
}

void LoginWindow::onRegisterClicked()
{
    QString login = loginEdit->text();
    QString pass = passwordEdit->text();

    // ВАЛИДАЦИЯ
    if (!validateInput(login)) {
        QMessageBox::warning(this, "Ошибка валидации",
                             "Логин должен быть от 3 до 15 символов.\n"
                             "Разрешены: латиница, цифры и точки.");
        return;
    }

    if (!validateInput(pass)) {
        QMessageBox::warning(this, "Ошибка валидации",
                             "Пароль должен быть от 3 до 15 символов.\n"
                             "Разрешены: латиница, цифры и точки.");
        return;
    }

    // Если всё ок
    QMessageBox::information(this, "Успех", "Вы успешно зарегистрированы!");

    // Закрываем окно
    this->close();

    // Сигнализируем главному окну
    emit registrationSuccessful();
}
