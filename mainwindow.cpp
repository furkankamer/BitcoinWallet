#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include<string>
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



void MainWindow::on_pushButton_clicked()
{
    ui->pushButton->setText(QString::number(onclick++));
}
