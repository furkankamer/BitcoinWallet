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

using namespace std;

int onclick = 0;

QString URL = "http://localhost:8332/"; //Changeable URL per wallet(user)
QString current_user;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    toggleTabs(false);
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
    if(ok) query.exec(querystr);
    return query;
}

bool SignUp(QString querystr){
    try {
        if(RunQuery(querystr,"write").lastError().isValid())
            return false;
        return true;
    }  catch (QException exp) {
        return false;
    }
}

void MainWindow::toggleTabs(bool visible){

    for(int i=1;i<4;i++){
        ui->tabWidget->setTabEnabled(i,visible);
        ui->tabWidget->setTabVisible(i,visible);
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

void MainWindow::loadWallet(QJsonObject data) {
    QJsonObject result = data["result"].toObject();
    if (result["warning"].toString() != "") { //We must create a new wallet for the user
        qDebug() << "Creating wallet: " << current_user;
        GetResponse("createwallet",{ current_user.toStdString().c_str()});
    } else { //The existing wallet loaded successfully
        qDebug() << "Wallet loaded: " << current_user;
        URL = "http://localhost:8332/wallet/" + current_user;
    }
}

void MainWindow::createWallet(QJsonObject data) {
    QJsonObject result = data["result"].toObject();
    if (result["name"].toString() != "") { //The wallet created and loaded successfully
        qDebug() << "Wallet created: " << current_user;
        URL = "http://localhost:8332/wallet/" + current_user + ".dat";
    } else {
        qDebug() << "Error: createwallet";
        qDebug() << data;
    }
}

void MainWindow::callFunction(std::string funcName,QJsonObject data){
    if(funcName == "getbalances")
        showBalances(data);
    else if(funcName == "loadwallet")
        loadWallet(data);
    else if(funcName == "createwallet")
        createWallet(data);
    else if(funcName == "unloadwallet")
        qDebug() << "Wallet unloaded: " << current_user;
}

void MainWindow::GetResponse(std::string method,QJsonArray params = {}){ //Parameters forms an array of strings
    QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
    const QUrl url(URL);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization",QString("Basic " + QString("user:pw").toLocal8Bit().toBase64()).toLocal8Bit());
    QJsonObject obj;
    obj["method"] = method.c_str();

    /*if(params[0] != "") {
        int paramCount = sizeof(params) / sizeof(params[0]);
        for(int i = 0; i < paramCount; ++i) {
            obj["params"] = QJsonArray();
            obj["params"][i] = params[i];
        }
    }*/

    if(!params.empty()){
        obj["params"] = params;
    }

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
    if(ui->tabWidget->tabText(index) == "Logout"){
        this->username = QString();
        ui->LoginForm->show();
        toggleTabs(false);
        ui->BalanceText1->setText(" ");
        ui->BalanceText2->setText(" ");
        ui->BalanceText3->setText(" ");
        ui->BalanceText4->setText(" ");
        ui->label_wallet->setText(" ");
        URL = "http://localhost:8332/";
        GetResponse("unloadwallet", {current_user.toStdString().c_str()});
        current_user = "";
    }

    if(username == NULL)
        return;
    if(index == 0){ //Main Page
        GetResponse("getbalances");
    } else if (index == 1) { //Send
        /*QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        const QUrl url(URL);
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

void MainWindow::on_signIn_clicked()
{
    QString username = ui->userText->text();
    QString password = QString(getHashedPass(ui->passText->text()).c_str());
    QString signinquery = "select * from users where username = '"
            + username + "' and password = '" + password + "'";
    bool signin = SignIn(signinquery);
    ui->Information->show();
    if(signin){
        this->username = username;
        ui->LoginForm->hide();
        ui->Information->hide();
        toggleTabs(true);

        //Welcoming user
        current_user = username;
        QString welcome_user = "Welcome, " + username + ".";
        ui->label_wallet->setText(welcome_user);

        //Loading existing wallet
        GetResponse("loadwallet", {QString(username).toStdString().c_str()});

        GetResponse("getbalances");
    }
    else{
        ui->Information->show();
        ui->Information->setText("an error occured");
    }

}

void MainWindow::on_signUp_clicked()
{
    ui->Information->show();
    QString username = ui->userText->text();
    QString password = QString(getHashedPass(ui->passText->text()).c_str());
    QString signupquery = "insert into users (username, password) values('"
            + username + "','" + password + "')";
    bool signup = SignUp(signupquery);
    if(signup){
        ui->Information->setText("success in signup");
    }
    else{
        ui->Information->show();
        ui->Information->setText("this username already in use.");
    }

}
