#ifndef STATISTICSDIALOG_H
#define STATISTICSDIALOG_H

#include "ui_StatisticsDialog.h"
#include <QMap>

class CharacterInfo {
public:
    CharacterInfo();
    ~CharacterInfo();
    static void reset();
    void add();
    static void addClass();
    float getPrecentage() const;
    float getRate() const;
    float getStd() const;
    u_int32_t getCount() const;
    static u_int32_t getTotalCount();
private:
    u_int32_t count;
    static u_int32_t classCount;
    static u_int32_t totalCount;
};

class StatisticsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StatisticsDialog(QWidget *parent = 0);

protected:
    void changeEvent(QEvent *e);

public slots:
    void setBoxFile(QString);
    void updateStats();

private:
    Ui::ShowStatistics ui;
    QString boxFile;
    QMap<QString,CharacterInfo> statMap;
};

#endif // STATISTICSDIALOG_H
