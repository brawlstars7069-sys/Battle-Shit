#ifndef CREATESERVERDIALOG_H
#define CREATESERVERDIALOG_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

class CreateServerDialog : public QWidget
{
    Q_OBJECT

public:
    explicit CreateServerDialog(QWidget *parent = nullptr);

signals:
    void serverCreated(const QString &name, const QString &password);

private slots:
    void onCreateClicked();
    void onCancelClicked();

private:
    QLineEdit *nameEdit;
    QLineEdit *passwordEdit;
    void setupUI();
};

#endif // CREATESERVERDIALOG_H
