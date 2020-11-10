#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include<string>
#include <QSqlDatabase>
#include <QtSql>

int onclick = 0;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString GetDataFromDataBase(QString query){
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("hattie.db.elephantsql.com");
    db.setDatabaseName("lvzhcnac");
    db.setUserName("lvzhcnac");
    db.setPassword("FjnjB28yNrnKOwp_coyq7LABdtIL2iIK");
    bool ok = db.open();
    QString result = "success";
    if(!db.open())
         result = "Failed";
    return result;
}

void MainWindow::on_pushButton_clicked()
{
    ui->pushButton->setText(GetDataFromDataBase("asd"));
}
