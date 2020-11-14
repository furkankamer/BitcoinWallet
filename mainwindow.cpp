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
#include <QtConcurrent>
#include <QFuture>
#include <QCloseEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QTableView>
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
    setPixmap(ui->sendIcon, ":/upload.jpg");
    setPixmap(ui->receiveIcon, ":/download-flat.png");
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPixmap(QLabel* label, QString url){
    QPixmap pixmapTarget = QPixmap(url);
    pixmapTarget = pixmapTarget.scaled(100-5, 100-5, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    label->setPixmap(pixmapTarget);
    label->hide();
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



void MainWindow::closeEvent (QCloseEvent *event)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question( this, "btc",
                                                                tr("Are you sure?\n"),
                                                                QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                                                QMessageBox::Yes);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        URL = "http://localhost:8332/";
        if(current_user != "") GetResponse("unloadwallet", {current_user.toStdString().c_str()});
        QMessageBox msgBox;
        msgBox.setText("App will be closed soon");
        msgBox.exec();
    }
}

bool SignUp(QString querystr){
    try {
        return !(RunQuery(querystr,"write").lastError().isValid());
    }  catch (QException exp) {
        return false;
    }
}

void MainWindow::toggleTabs(bool visible){
    ui->transactionPanel->setVisible(visible);
    ui->Balances->setVisible(visible);
    for(int i=1;i<ui->tabWidget->count();i++){
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

void MainWindow::setNewBitcoinAddress(QJsonObject data){
    QString address = data["result"].toString();
    ui->receiveAddress->setText(address);
}

void MainWindow::toggleIcons(bool send){
    ui->sendIcon->setVisible(send);
    ui->receiveIcon->setVisible(!send);
}

void MainWindow::createWallet(QJsonObject data) {
    QJsonObject result = data["result"].toObject();
    if (result["name"].toString() != "") { //The wallet created and loaded successfully
        qDebug() << "Wallet created: " << current_user;
        URL = "http://localhost:8332/wallet/" + current_user;
    } else {
        qDebug() << "Error: createwallet";
        qDebug() << data;
    }
}
QLabel* createLabel(QString text){
    QFont f( "Arial", 8, QFont::Bold);
    QLabel* ll = new QLabel();
    ll->setText(text);
    ll->setFont(f);
    ll->adjustSize();
    return ll;
}
QLabel* labelWithImage(bool send){
    QLabel* label = new QLabel();
    QString url = send ? QString(":/upload.jpg") : QString(":/download-flat.png");
    QPixmap pixmapTarget = QPixmap(url);
    pixmapTarget = pixmapTarget.scaled(100-5, 30-5, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    label->setPixmap(pixmapTarget);
    return label;
}
void addRowToTable(QTableWidget* table,QString text, bool send){

    table->insertRow(table->rowCount());
    table->setItem(table->rowCount()-1,1,new QTableWidgetItem(text));
    table->setCellWidget(table->rowCount()-1,0,labelWithImage(send));
}


void MainWindow::ShowRecentTransaction(QJsonObject data){
    QJsonArray alltransactions = data["result"].toArray();
    QJsonObject recentTransaction = alltransactions.last().toObject();
    if(recentTransaction.empty()) ui->transactionText->setText("No transaction available");
    else{
        ui->tableWidget->setRowCount(0);
        for(int i=0;i<alltransactions.count();i++){
            QJsonObject transaction = alltransactions[i].toObject();
            addRowToTable(ui->tableWidget,
            QString("Time: ") + QString::number(transaction["time"].toVariant().toInt())
                    + QString(", Address: ") + transaction["address"].toString()
                    + QString(", Amount: ") + QString::number(transaction["amount"].toVariant().toDouble() +
                              recentTransaction["fee"].toVariant().toDouble(), 'd', 8)
                    + QString(", Confirmations: ") + QString::number(transaction["confirmations"].toInt())
                    + QString(", Trusted: ") + transaction["trusted"].toString(),transaction["category"].toString() == "send");
        }
        ui->time->setText(QString::number(recentTransaction["time"].toVariant().toInt()));
        ui->address->setText(recentTransaction["address"].toString());
        ui->amount->setText(QString::number(recentTransaction["amount"].toVariant().toDouble() +
                            recentTransaction["fee"].toVariant().toDouble(), 'd', 8)); //amount+fee

        qDebug() << "Time: " << recentTransaction["time"].toVariant().toInt();
        qDebug() << "Address: " << recentTransaction["address"].toString();
        qDebug() << "Amount: " << recentTransaction["amount"].toVariant().toDouble();
        qDebug() << "Fee: " << recentTransaction["fee"].toVariant().toDouble();

        QString transactionCategory = recentTransaction["category"].toString();
        toggleIcons(transactionCategory == "send");
    }
}

void MainWindow::estimateFee(QJsonObject data) {
    QJsonObject result = data["result"].toObject();
    if (!result["feerate"].isNull()) {
        ui->label_feeRate->setText(QString::number(result["feerate"].toVariant().toDouble(), 'd', 8));
    } else {
        qDebug() << "Error when estimating fee.";
        ui->label_feeRate->setText("0.00001000");
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
    else if(funcName == "listtransactions")
        ShowRecentTransaction(data);
    else if(funcName == "getnewaddress")
        setNewBitcoinAddress(data);
    else if(funcName == "estimatesmartfee")
        estimateFee(data);
    else if(funcName == "sendtoaddress")
        GetResponse("getbalances",{});
}


void MainWindow::GetResponse(std::string method,QJsonArray params = {}){
        QNetworkAccessManager *mgr = new QNetworkAccessManager(this);
        const QUrl url(URL);
        QNetworkRequest request(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization",QString("Basic " + QString("user:pw").toLocal8Bit().toBase64()).toLocal8Bit());
        QJsonObject obj;
        obj["method"] = method.c_str();

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
        ui->userText->setText("");
        ui->passText->setText("");
        ui->tabWidget->setCurrentIndex(0);
    }

    if(username == NULL)
        return;
    if(index == 0){ //Main Page
        GetResponse("getbalances");
        GetResponse("listtransactions");
    } else if (index == 1) { //Send
        GetResponse("estimatesmartfee", {1008}); //Calculate conservative fee
        if(ui->label_feeRate->text().toDouble() > ui->BalanceText1->text().toDouble())
            ui->sendBitcoinAmount->setMaximum(ui->BalanceText1->text().toDouble()); //Send
        else ui->sendBitcoinAmount->setMaximum(ui->BalanceText1->text().toDouble() - ui->label_feeRate->text().toDouble());
    } else if (index == 3){//listtransactions
        GetResponse("listtransactions");
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
        ui->tabWidget->setCurrentIndex(5);
        GetResponse("estimatesmartfee", {1008});
        if(ui->label_feeRate->text() == "")
            ui->label_feeRate->setText("0.000010000");
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

void MainWindow::on_doubleSpinBox_amount_valueChanged(double arg1)
{

}

void MainWindow::on_kayit_2_clicked()
{
    QMessageBox msgBox;
    if(ui->sendBitcoinAddress->text() == ""){
        msgBox.setText("You must enter an address");
        msgBox.exec();
        return;
    }
    else if(ui->sendBitcoinAmount->text().toInt() == 0){
        msgBox.setText("The amount cannot be zero");
        msgBox.exec();
        return;
    }
    GetResponse("sendtoaddress",{ui->sendBitcoinAddress->text(),ui->sendBitcoinAmount->value()});
}

void MainWindow::on_generateAddress_clicked()
{
    GetResponse("getnewaddress");
}
