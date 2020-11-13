#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_tabWidget_tabBarClicked(int index);
    void showBalances(QJsonObject data);
    void loadWallet(QJsonObject data);
    void createWallet(QJsonObject data);
    void callFunction(std::string funcName,QJsonObject data);
    void toggleTabs(bool visible);
    void setPixmap(QLabel* label, QString url);
    void GetResponse(std::string method, QJsonArray params);
    void ShowRecentTransaction(QJsonObject data);
    void on_signIn_clicked();
    void toggleIcons(bool send);
    void on_signUp_clicked();

private:
    QString username = QString();
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
