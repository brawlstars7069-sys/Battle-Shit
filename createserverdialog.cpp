#include "createserverdialog.h"
#include <QFormLayout>

CreateServerDialog::CreateServerDialog(QWidget *parent) : QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
{
    // Настраиваем флаги окна: диалог, без рамки, всегда поверх
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setFixedSize(400, 300);
    setupUI();
}

void CreateServerDialog::setupUI()
{
    // Стилизация в духе проекта
    this->setStyleSheet(
        "QWidget {"
        "   background-color: #dcdcdc;"
        "   font-family: 'Courier New';"
        "   color: #333;"
        "   border: 2px solid #555;"
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
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #dcd0c0;"
        "   border: 2px solid #333;"
        "}"
        );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    QLabel *title = new QLabel("СОЗДАНИЕ КОМНАТЫ", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 20px; font-weight: bold; border: none; border-bottom: 2px dashed #555; padding-bottom: 10px;");

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("Название комнаты");

    passwordEdit = new QLineEdit(this);
    passwordEdit->setPlaceholderText("Пароль (необязательно)");
    passwordEdit->setEchoMode(QLineEdit::Password);

    QLabel *lblName = new QLabel("НАЗВАНИЕ:", this);
    lblName->setStyleSheet("border: none; font-weight: bold;");
    QLabel *lblPass = new QLabel("ПАРОЛЬ:", this);
    lblPass->setStyleSheet("border: none; font-weight: bold;");

    formLayout->addRow(lblName, nameEdit);
    formLayout->addRow(lblPass, passwordEdit);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnCreate = new QPushButton("СОЗДАТЬ", this);
    QPushButton *btnCancel = new QPushButton("ОТМЕНА", this);

    btnCreate->setStyleSheet("background-color: #27ae60; color: white;");
    btnCancel->setStyleSheet("background-color: #c0392b; color: white;");

    connect(btnCreate, &QPushButton::clicked, this, &CreateServerDialog::onCreateClicked);
    connect(btnCancel, &QPushButton::clicked, this, &CreateServerDialog::onCancelClicked);

    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnCreate);

    mainLayout->addWidget(title);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(btnLayout);
}

void CreateServerDialog::onCreateClicked()
{
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите название комнаты!");
        return;
    }

    emit serverCreated(name, passwordEdit->text());
    this->close();
}

void CreateServerDialog::onCancelClicked()
{
    this->close();
}
