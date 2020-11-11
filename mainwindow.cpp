#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <functional>
#include<string>
#include <QSqlDatabase>
#include<QNetworkReply>
#include<QNetworkAccessManager>
#include <QtSql>
#include<QDebug>
#include <QJsonObject>
#include <QThread>
#include <map>

#define STR_SALT_KEY "12344321"
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
std::map<std::string, std::function<void(QJsonObject)>> myMap;
QSqlQuery RunQuery(QString querystr,QString mode = "read"){
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("hattie.db.elephantsql.com");
    db.setDatabaseName("lvzhcnac");
    db.setUserName("lvzhcnac");
    db.setPassword("FjnjB28yNrnKOwp_coyq7LABdtIL2iIK");
    bool ok = db.open();
    QSqlQuery query;
    if(ok){
        qDebug( querystr.toStdString().c_str());
        query.exec(querystr);
    }
    return query;
}

bool SignUp(QString querystr){
    try {
        RunQuery(querystr,"write");
        return true;
    }  catch (QException exp) {
        return false;
    }
}


bool SignIn(QString querystr){
    QSqlQuery resultquery = RunQuery(querystr);
    if(!resultquery.next())
        return false;
    return true;
}

std::string getHashedPass(QString password){
    QByteArray pswNsalt (password.toStdString().c_str());
    pswNsalt.append(STR_SALT_KEY) ;
    return QCryptographicHash::hash(pswNsalt, QCryptographicHash::Sha256).toHex().toStdString();
}

void MainWindow::on_pushButton_clicked()
{
    QString username = ui->lineEdit->text();
    QString password = QString(getHashedPass(ui->lineEdit_2->text()).c_str());
    QString signinquery = "select * from users where username = '"
            + username + "' and password = '" + password + "'";
    bool signin = SignIn(signinquery);
    ui->text->show();
    if(signin){
        ui->lineEdit->hide();
        ui->lineEdit_2->hide();
        ui->text->setText("success in signin");
    }
    else{
        ui->text->show();
        ui->text->setText("an error occured");
    }
}

void MainWindow::on_kayit_clicked()
{
    ui->text->show();
    QString username = ui->lineEdit->text();
    QString password = QString(getHashedPass(ui->lineEdit_2->text()).c_str());
    QString signupquery = "insert into users (username, password) values('"
            + username + "','" + password + "')";
    bool signup = SignUp(signupquery);
    if(signup){
        ui->text->setText("success in signup");
    }
    else{
        ui->text->show();
        ui->text->setText("an error occured");
    }
}

void MainWindow::showBalances(QJsonObject data){
    bool ok = false;
    QJsonObject result = data["result"].toObject()["mine"].toObject();
    ui->BalanceText1->setText(QString::number(result["trusted"].toVariant().toDouble(&ok), 'd', 8));
    ui->BalanceText2->setText(QString::number(result["untrusted_pending"].toVariant().toDouble(&ok), 'd', 8));
    ui->BalanceText3->setText(QString::number(result["immature"].toVariant().toDouble(&ok), 'd', 8));
    ui->BalanceText1_2->setText(QString::number(result["trusted"].toVariant().toDouble(&ok), 'd', 8)); //Send page balance
    double total = result["trusted"].toDouble()
           + result["untrusted_pending"].toDouble()
           + result["immature"].toDouble();
    ui->BalanceText4->setText(QString::number(total, 'd', 8));
    qDebug() << data;
    qDebug() << ok;
}
void MainWindow::callFunction(std::string funcName,QJsonObject data){
    if(funcName == "getbalances")
        showBalances(data);
}

void MainWindow::GetResponse(std::string method,std::string params = ""){

    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    const QUrl url(QStringLiteral("http://localhost:8332/"));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",QString("Basic " + QString("user:pw").toLocal8Bit().toBase64()).toLocal8Bit());
    QJsonObject obj;
    obj["method"] = method.c_str();
    if(params != "")
        obj["params"] = params.c_str();
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();
    QJsonObject response;
    QNetworkReply *reply = mgr->post(request, data);
        QObject::connect(reply, &QNetworkReply::finished, [=](){
            if(reply->error() == QNetworkReply::NoError){
                QString contents = QString::fromUtf8(reply->readAll());
                qDebug() << contents;
                callFunction(method,QJsonDocument::fromJson(contents.toUtf8()).object());
            }
            else{
                QString err = reply->errorString();
                qDebug() << err;
            }
            reply->deleteLater();
        });

}

void MainWindow::on_tabWidget_tabBarClicked(int index)
{
    if(index == 0){ //Main Page
        GetResponse("getbalances");

    } else if (index == 1) { //Send
        /*QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        const QUrl url(QStringLiteral("http://localhost:8332/"));
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization",QString("Basic " + QString("user:pw").toLocal8Bit().toBase64()).toLocal8Bit());
        QJsonObject obj;
        obj["method"] = "getbalances";
        QJsonDocument doc(obj);
        QByteArray data = doc.toJson();
        QNetworkReply *reply = mgr->post(request, data);
        QObject::connect(reply, &QNetworkReply::finished, [=](){
            if(reply->error() == QNetworkReply::NoError){
                bool ok = false;
                QString contents = QString::fromUtf8(reply->readAll());
                QJsonDocument jsonResponse = QJsonDocument::fromJson(contents.toUtf8());
                QJsonObject jsonObject = jsonResponse.object();
                QJsonObject result = jsonObject["result"].toObject()["mine"].toObject();
                ui->BalanceText1->setText(QString::number(result["trusted"].toVariant().toDouble(&ok), 'd', 8));
                ui->BalanceText2->setText(QString::number(result["untrusted_pending"].toVariant().toDouble(&ok), 'd', 8));
                ui->BalanceText3->setText(QString::number(result["immature"].toVariant().toDouble(&ok), 'd', 8));

                double total = result["trusted"].toDouble()
                        + result["untrusted_pending"].toDouble()
                        + result["immature"].toDouble();
                ui->BalanceText4->setText(QString::number(total, 'd', 8));
                qDebug() << result;
                qDebug() << ok;
            }
            else{
                QString err = reply->errorString();
                qDebug() << err;
            }
            reply->deleteLater();
        });*/
    }
}
