#include "Statistics.h"
#include <QFile>
#include <QByteArray>

#include <QDebug>

u_int32_t CharacterInfo::totalCount = 0;
u_int32_t CharacterInfo::classCount = 0;

StatisticsDialog::StatisticsDialog(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);
}

void StatisticsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void StatisticsDialog::setBoxFile(QString fileName)
{
    this->boxFile = fileName;
}

void StatisticsDialog::updateStats()
{
    CharacterInfo::reset();
    this->statMap.clear();
    ui.tableWidget->clear();
    //ui.tableWidget->horizontalHeader()
    ui.tableWidget->setRowCount(0);
    QFile file(this->boxFile);
    if(file.open(QFile::ReadOnly)){
        while(!file.atEnd()){
            QByteArray ba = file.readLine();
            QString line = QString::fromUtf8(ba.data(),ba.size());
            QStringList splitted = line.split(QChar(' '));
            if(splitted.size() != 6){
                // TODO: handle error
                continue;
            }
            QString c = splitted.at(0);
            CharacterInfo i;
            if(this->statMap.contains(c)){
                i = statMap.value(c);
            } else {
                CharacterInfo::addClass();
            }
            i.add();
            statMap.insert(c,i);
       }
    }
    file.close();
    QMapIterator<QString, CharacterInfo> i(statMap);
    while (i.hasNext()) {
        i.next();
        int lastRow = ui.tableWidget->rowCount();
        ui.tableWidget->insertRow(lastRow);
        QTableWidgetItem* p = new QTableWidgetItem();
        p->setData(Qt::DisplayRole,i.value().getPrecentage());
        QTableWidgetItem* c = new QTableWidgetItem();
        c->setData(Qt::DisplayRole,i.value().getCount());
        QTableWidgetItem* s = new QTableWidgetItem();
        s->setData(Qt::DisplayRole,i.value().getStd());
        ui.tableWidget->setItem(lastRow,0,new QTableWidgetItem(i.key()));
        ui.tableWidget->setItem(lastRow,1,c);
        ui.tableWidget->setItem(lastRow,2,p);
        ui.tableWidget->setItem(lastRow,3,s);
    }
}

CharacterInfo::CharacterInfo():
    count(0) {

}

CharacterInfo::~CharacterInfo()
{

}

void CharacterInfo::add()
{
    CharacterInfo::totalCount++;
    this->count++;
}

void CharacterInfo::addClass()
{
    classCount++;
}

float CharacterInfo::getPrecentage() const
{
    return 100.0 * static_cast<float>(count)/static_cast<float>(totalCount);
}

float CharacterInfo::getRate() const
{
    return static_cast<float>(count)/static_cast<float>(totalCount);
}

float CharacterInfo::getStd() const
{
    float mean = 1.0 / static_cast<float>(classCount);
    return getRate() - mean;
}

u_int32_t CharacterInfo::getCount() const
{
    return this->count;
}

void CharacterInfo::reset()
{
    CharacterInfo::totalCount = 0;
    CharacterInfo::classCount = 0;
}
