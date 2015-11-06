/**********************************************************************
* File:        ChildWidget.cpp
* Description: ChildWidget functions
* Author:      Marcel Kolodziejczyk
* Created:     2010-01-06
*
* (C) Copyright 2010, Marcel Kolodziejczyk
* (C) Copyright 2011-2014, Zdenko Podobny
* (C) Copyright 2012, Zohar Gofer
* (C) Copyright 2012, Dmitri Silaev
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**    http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*
**********************************************************************/

#include <string>
#include <algorithm>
#include <leptonica/allheaders.h>

#include "ChildWidget.h"
#include "Settings.h"
#include "DelegateEditors.h"
#include "TessTools.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/GetRowIDDialog.h"
#include "dialogs/FindDialog.h"
#include "dialogs/DrawRectangle.h"
#include "dialogs/Statistics.h"

// This allows storing QGraphicsRectItem's in table model data
Q_DECLARE_METATYPE(QGraphicsRectItem*)

// Print debug message
int DMESS = 0;

// Min/max macros
int my_min(int arg1, int arg2) {
    return((arg1 < arg2) ? arg1 : arg2);
}

int my_max(int arg1, int arg2) {
    return((arg1 > arg2) ? arg1 : arg2);
}

// STATICS INITIALIZATION
const Qt::CursorShape DragResizer::gripCursor[dirCount] = {
    Qt::SizeHorCursor, Qt::SizeBDiagCursor, Qt::SizeVerCursor,
    Qt::SizeFDiagCursor,
    Qt::SizeHorCursor, Qt::SizeBDiagCursor, Qt::SizeVerCursor,
    Qt::SizeFDiagCursor};

////////////////////////////////////////////////////////////////////////////////

void DragResizer::init(QGraphicsScene* scene) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // NOTE: QGraphicsScene takes ownership for all these objects

    // This is required to be able to process all drag rectangles' messages
    scene->addItem(this);

    for (int i = 0; i < dirCount; ++i) {
        gripRect[i] = scene->addRect(0, 0, 0, 0,
                                     QPen(/*QColor(Qt::black)*/Qt::NoPen),
                                     QBrush(Qt::NoBrush));
        gripRect[i]->setCursor(gripCursor[i]);
        gripRect[i]->setZValue(5);
        // Store direction in data key #0
        gripRect[i]->setData(0, i);
        gripRect[i]->installSceneEventFilter(this);
    }

    disable();
}

void DragResizer::setFromRect(const QRect& arect) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    rect = arect;

    gripRect[dirE]->setRect(rect.right() - gripMargin,
                            rect.top() + gripMargin + 1,
                            2*gripMargin + 1,
                            rect.height() - 2*gripMargin - 2);
    gripRect[dirNE]->setRect(rect.right() - gripMargin,
                             rect.top() - gripMargin,
                             2*gripMargin + 1,
                             2*gripMargin + 1);
    gripRect[dirN]->setRect(rect.left() + gripMargin + 1,
                            rect.top() - gripMargin,
                            rect.width() - 2*gripMargin - 2,
                            2*gripMargin + 1);
    gripRect[dirNW]->setRect(rect.left() - gripMargin,
                             rect.top() - gripMargin,
                             2*gripMargin + 1,
                             2*gripMargin + 1);
    gripRect[dirW]->setRect(rect.left() - gripMargin,
                            rect.top() + gripMargin + 1,
                            2*gripMargin + 1,
                            rect.height() - 2*gripMargin - 2);
    gripRect[dirSW]->setRect(rect.left() - gripMargin,
                             rect.bottom() - gripMargin,
                             2*gripMargin + 1,
                             2*gripMargin + 1);
    gripRect[dirS]->setRect(rect.left() + gripMargin + 1,
                            rect.bottom() - gripMargin,
                            rect.width() - 2*gripMargin - 2,
                            2*gripMargin + 1);
    gripRect[dirSE]->setRect(rect.right() - gripMargin,
                             rect.bottom() - gripMargin,
                             2*gripMargin + 1,
                             2*gripMargin + 1);

    for (int i = 0; i < dirCount; ++i)
        gripRect[i]->setVisible(true);
}

void DragResizer::disable() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    for (int i = 0; i < dirCount; ++i)
        gripRect[i]->setVisible(false);
}

bool DragResizer::enabled() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return gripRect[0]->isVisible();
}

bool DragResizer::sceneEventFilter(QGraphicsItem* watched, QEvent* event) {
    if (DMESS > 11) qDebug() << Q_FUNC_INFO;
    QGraphicsRectItem* rectItem = static_cast<QGraphicsRectItem*>(watched);
    QGraphicsSceneMouseEvent* mouseEvent =
            static_cast<QGraphicsSceneMouseEvent*>(event);

    QRectF tmpRect;
    QPoint gripPt;

    switch (event->type()) {
    case QEvent::GraphicsSceneMouseMove:
        // Shift rect item by pixel delta since prev mouse move event
        tmpRect = rectItem->rect();
        tmpRect.translate(mouseEvent->scenePos() - mouseEvent->lastScenePos());
        rectItem->setRect(tmpRect);

        gripPt = rectItem->rect().center().toPoint();

        switch (watched->data(0).toInt()) {
        case dirE:
            rect.setRight(gripPt.x());
            break;
        case dirNE:
            rect.setTopRight(gripPt);
            break;
        case dirN:
            rect.setTop(gripPt.y());
            break;
        case dirNW:
            rect.setTopLeft(gripPt);
            break;
        case dirW:
            rect.setLeft(gripPt.x());
            break;
        case dirSW:
            rect.setBottomLeft(gripPt);
            break;
        case dirS:
            rect.setBottom(gripPt.y());
            break;
        case dirSE:
            rect.setBottomRight(gripPt);
            break;
        default:
            break;
        }

        emit changed();

        return true;
    case QEvent::GraphicsSceneMousePress:
        return true;
    case QEvent::GraphicsSceneMouseRelease:
        setFromRect(rect);
        return true;
    default:
        break;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////

ChildWidget::ChildWidget(QWidget* parent)
    : QSplitter(Qt::Horizontal, parent) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    table = new QTableView;
    statisticsTable = new QTableView;
    table->resize(1, 1);
    table->setAlternatingRowColors(true);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    table->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#else
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#endif
    table->installEventFilter(this);  // installs event filter
    statisticsTable->resize(1, 1);
    statisticsTable->setAlternatingRowColors(true);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    statisticsTable->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#else
    statisticsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
#endif
    statisticsTable->installEventFilter(this);  // installs event filter
    initTable();

    // Make graphics Scene and View
    imageScene = new QGraphicsScene;
    imageScene->installEventFilter(this);
    imageView = new QGraphicsView(imageScene);
    imageView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    imageView->setRenderHints(QPainter::Antialiasing |
                              QPainter::SmoothPixmapTransform);
    imageView->setAttribute(Qt::WA_TranslucentBackground, true);
    imageView->setAutoFillBackground(true);

    resizer = new DragResizer;
    resizer->init(imageScene);
    connect(resizer, SIGNAL(changed()), this, SLOT(boxDragChanged()));

    readSettings();

    // Table toolbar
    QPushButton* upButton = new QPushButton();
    upButton->setIcon(QIcon::fromTheme("up"));
    upButton->setToolTip(tr("Move row up"));
    upButton->setMinimumSize(QSize(24, 24));
    upButton->setMaximumSize(QSize(24, 24));
    connect(upButton, SIGNAL(clicked()), this, SLOT(moveUp()));

    QPushButton* downButton = new QPushButton();
    downButton->setIcon(QIcon::fromTheme("down"));
    downButton->setToolTip(tr("Move row down"));
    downButton->setMinimumSize(QSize(24, 24));
    downButton->setMaximumSize(QSize(24, 24));
    connect(downButton, SIGNAL(clicked()), this, SLOT(moveDown()));

    QPushButton* moveToButton = new QPushButton();
    moveToButton->setIcon(QIcon::fromTheme("moveTo"));
    moveToButton->setToolTip(tr("Move row to position…"));
    moveToButton->setMinimumSize(QSize(24, 24));
    moveToButton->setMaximumSize(QSize(24, 24));
    connect(moveToButton, SIGNAL(clicked()), this, SLOT(moveTo()));

    QPushButton* goToButton = new QPushButton();
    goToButton->setIcon(QIcon::fromTheme("jump"));
    goToButton->setToolTip(tr("Go to row…"));
    goToButton->setMinimumSize(QSize(24, 24));
    goToButton->setMaximumSize(QSize(24, 24));
    connect(goToButton, SIGNAL(clicked()), this, SLOT(goToRow()));

    QPushButton* insertButton = new QPushButton();
    insertButton->setIcon(QIcon::fromTheme("insertRow"));
    insertButton->setToolTip(tr("Insert symbol"));
    insertButton->setMaximumSize(QSize(24, 24));
    connect(insertButton, SIGNAL(clicked()), this, SLOT(insertSymbol()));

    QPushButton* joinButton = new QPushButton();
    joinButton->setIcon(QIcon::fromTheme("joinRow"));
    joinButton->setToolTip(tr("Join symbols"));
    joinButton->setMaximumSize(QSize(24, 24));
    connect(joinButton, SIGNAL(clicked()), this, SLOT(joinSymbol()));

    QPushButton* splitButton = new QPushButton();
    splitButton->setIcon(QIcon::fromTheme("splitRow"));
    splitButton->setToolTip(tr("Split symbol"));
    splitButton->setMaximumSize(QSize(24, 24));
    connect(splitButton, SIGNAL(clicked()), this, SLOT(splitSymbol()));

    QPushButton* removeButton = new QPushButton();
    removeButton->setIcon(QIcon::fromTheme("deleteRow"));
    removeButton->setToolTip(tr("Remove symbol"));
    removeButton->setMaximumSize(QSize(24, 24));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(deleteSymbol()));

    QHBoxLayout* movementLayout = new QHBoxLayout();
    movementLayout->setSpacing(2);
    movementLayout->addWidget(upButton);
    movementLayout->addWidget(downButton);
    movementLayout->addWidget(moveToButton);
    movementLayout->addWidget(goToButton);

    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(2);
    actionLayout->addWidget(insertButton);
    actionLayout->addWidget(joinButton);
    actionLayout->addWidget(splitButton);
    actionLayout->addWidget(removeButton);

    QSpacerItem* horizontalSpacer = new QSpacerItem(73, 20,
                                                    QSizePolicy::Expanding,
                                                    QSizePolicy::Minimum);

    QGridLayout* gridLayout = new QGridLayout();
    gridLayout->setContentsMargins(3, 3, 3, 3);
    gridLayout->addLayout(movementLayout, 0, 0, 1, 1);
    gridLayout->addItem(horizontalSpacer, 0, 1, 1, 1);
    gridLayout->addLayout(actionLayout, 0, 2, 1, 1);

    QWidget* tableWidget = new QWidget(this);
    QVBoxLayout* verticalLayout = new QVBoxLayout(tableWidget);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    verticalLayout->addWidget(table);
    verticalLayout->addWidget(statisticsTable);
    verticalLayout->addLayout(gridLayout);

    // Image part of display
    QWidget* imageWidget = new QWidget(this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imageWidget);
    imageLayout->setContentsMargins(0, 0, 0, 2);
    imageLayout->addWidget(imageView);

    pageWidget = new QWidget(this);
    QHBoxLayout* pageControlLayout = new QHBoxLayout();
    pageControlLayout->setContentsMargins(0, 0, 0, 4);
    QSpacerItem* leftHorSpacer = new QSpacerItem(248, 20, QSizePolicy::Expanding,
                                                 QSizePolicy::Minimum);
    pageControlLayout->addItem(leftHorSpacer);
    currentPage = new QSpinBox(imageWidget);
    pageControlLayout->addWidget(currentPage);
    numberOfPages = new QLabel(imageWidget);
    pageControlLayout->addWidget(numberOfPages);
    QSpacerItem* rightHorSpacer = new QSpacerItem(308, 20, QSizePolicy::Expanding,
                                                  QSizePolicy::Minimum);
    pageControlLayout->addItem(rightHorSpacer);
    pageWidget->setLayout(pageControlLayout);
    imageLayout->addWidget(pageWidget);

    // splitter
    addWidget(tableWidget);
    addWidget(imageWidget);
    setStretchFactor(indexOf(table), 0);
    setStretchFactor(indexOf(imageView), 1);
    connect(this, SIGNAL(splitterMoved(int, int)), this,
            SLOT(updateColWidthsOnSplitter(int, int)));

    setSelectionRect();
    widgetWidth = parent->size().width();
    imageItem = NULL;
    modified = false;
    boxesVisible = false;
    drawnRectangle = false;
    symbolShown = true;
    directTypingMode = false;
    f_dialog = 0;
    statisticsDialog = 0;
    m_DrawRectangle = 0;
    rectangle = 0;
    vertLineLeft = 0;
    vertLineRight = 0;
    horLineTop = 0;
    horLineBottom = 0;
    currPage = 0;

    rubberBand = new QRubberBand(QRubberBand::Rectangle, imageView);

    m_undostack.SetRedoStack(&m_redostack);
    bIsSpinBoxChanged = false;
    bIsLineEditChanged = false;
    fileWatcher = 0;
}

void ChildWidget::initTable() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    model = new QStandardItemModel(0, 10, this);
    model->setHeaderData(0, Qt::Horizontal, tr("Letter"));
    model->setHeaderData(1, Qt::Horizontal, tr("Left"));
    model->setHeaderData(2, Qt::Horizontal, tr("Bottom"));
    model->setHeaderData(3, Qt::Horizontal, tr("Right"));
    model->setHeaderData(4, Qt::Horizontal, tr("Top"));
    model->setHeaderData(5, Qt::Horizontal, tr("Page"));
    model->setHeaderData(6, Qt::Horizontal, tr("Italic"));
    model->setHeaderData(7, Qt::Horizontal, tr("Bold"));
    model->setHeaderData(8, Qt::Horizontal, tr("Underline"));
    model->setHeaderData(9, Qt::Horizontal, tr("<Hidden> BB"));
    table->setModel(model);
    selectionModel = new QItemSelectionModel(model);
    connect(
                selectionModel,
                SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
                this, SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));
    table->setSelectionModel(selectionModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);

    table->hideColumn(5);
    table->hideColumn(6);
    table->hideColumn(7);
    table->hideColumn(8);
    table->hideColumn(9);

    //TODO(zdenop): does it make sense to initialize this when changing/reloading page?
    LineEditDelegate* leDelegate = new LineEditDelegate;
    table->setItemDelegateForColumn(0, leDelegate);

    connect(leDelegate, SIGNAL(led_editstarted()), this,
            SLOT(letterStartEdit()));
    connect(leDelegate, SIGNAL(led_editfinished()), this,
            SLOT(letterEditFinished()));

    SpinBoxDelegate* sbDelegate = new SpinBoxDelegate;
    // TODO(zdenop): setMaximum for delegates after changing box
    table->setItemDelegateForColumn(1, sbDelegate);
    table->setItemDelegateForColumn(2, sbDelegate);
    table->setItemDelegateForColumn(3, sbDelegate);
    table->setItemDelegateForColumn(4, sbDelegate);
    connect(sbDelegate, SIGNAL(sbd_valueChanged(int)), this,
            SLOT(sbValueChanged(int)));
    connect(sbDelegate, SIGNAL(sbd_editingFinished()), this,
            SLOT(sbFinished()));

    CheckboxDelegate* cbDelegate = new CheckboxDelegate;
    table->setItemDelegateForColumn(6, cbDelegate);
    table->setItemDelegateForColumn(7, cbDelegate);
    table->setItemDelegateForColumn(8, cbDelegate);
    connect(cbDelegate, SIGNAL(toggled(bool, int)), this,
            SLOT(cbFontToggleProxy(bool, int)));

    statisticsModel = new QStandardItemModel(0, 4, this);
    statisticsModel->setHeaderData(0, Qt::Horizontal, tr("Letter"));
    statisticsModel->setHeaderData(1, Qt::Horizontal, tr("Count"));
    statisticsModel->setHeaderData(2, Qt::Horizontal, tr("%"));
    statisticsModel->setHeaderData(3, Qt::Horizontal, tr("STD"));

    statisticsModelProxy = new QSortFilterProxyModel(statisticsModel);
    statisticsModelProxy->setSourceModel(statisticsModel);
    statisticsModelProxy->setSortRole(Qt::EditRole);

    statisticsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statisticsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    statisticsTable->setModel(statisticsModelProxy);

    connect(model,SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this,SLOT(updateStats(QModelIndex,QModelIndex)));
}

void ChildWidget::readSettings() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // Font for table
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       SETTING_ORGANIZATION, SETTING_APPLICATION);
    QFont tableFont = settings.value("GUI/Font").value<QFont>();

    if (tableFont.family().isEmpty()) {
        tableFont.setFamily(TABLE_FONT);
        tableFont.setPointSize(TABLE_FONT_SIZE);
    }
    table->setFont(tableFont);

    // Font for Image/balloons
    if (settings.contains("GUI/UseTheSameFont") &&
            settings.value("GUI/UseTheSameFont").toBool()) {
        m_imageFont = tableFont;
    } else {
        m_imageFont = settings.value("GUI/ImageFont").value<QFont>();
        if (m_imageFont.family().isEmpty()) {
            m_imageFont.setFamily(TABLE_FONT);
            m_imageFont.setPointSize(TABLE_FONT_SIZE);
        }
    }
    m_imageFont.setPointSize(2 * m_imageFont.pointSize());

    if (settings.contains("GUI/ImageFontOffset")) {
        fontOffset = settings.value("GUI/ImageFontOffset").toInt();
    } else {
        fontOffset = 16 * 2 - 15;
    }

    if (settings.contains("GUI/BalloonCount")) {
        balloonCount = settings.value("GUI/BalloonCount").toInt();
    } else {
        balloonCount = 13;
    }

    if (settings.contains("GUI/ImageFontColor")) {
        imageFontColor = settings.value("GUI/ImageFontColor").value<QColor>();
    } else {
        imageFontColor = Qt::red;
    }

    if (settings.contains("GUI/Rectagle")) {
        rectColor = settings.value("GUI/Rectagle").value<QColor>();
    } else {
        rectColor = Qt::red;
    }

    if (settings.contains("GUI/Rectagle_fill")) {
        rectFillColor = settings.value("GUI/Rectagle_fill").value<QColor>();
    } else {
        rectFillColor = Qt::red;
        rectFillColor.setAlpha(127);
    }

    if (settings.contains("GUI/Box")) {
        boxColor = settings.value("GUI/Box").value<QColor>();
    } else {
        boxColor = Qt::green;
    }

    if (settings.contains("GUI/BackgroundColor")) {
        backgroundColor = settings.value("GUI/BackgroundColor").value<QColor>();
    } else {
        backgroundColor = (Qt::gray);
    }
    imageView->setBackgroundBrush(backgroundColor);

    if (model->rowCount() > 0) {
        table->resizeRowsToContents();
        statisticsTable->resizeRowsToContents();
        calculateLettersTableWidth();
    }
}

void ChildWidget::calculateLettersTableWidth() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // set optimum size of table
    table->resizeColumnsToContents();
    int tableVisibleWidth = 0;
    tableVisibleWidth += table->verticalHeader()->sizeHint().width();

    for (int col = 0; col < table->horizontalHeader()->count(); col++) {
        if (table->columnWidth(col) > 0)
            tableVisibleWidth += table->columnWidth(col) + 1;  // 1 px for table grid
    }
    // scrollbar
    tableVisibleWidth += table->verticalScrollBar()->sizeHint().width();
    tableVisibleWidth += table->frameWidth()*2;

    QList<int> splitterSizes;
    splitterSizes << tableVisibleWidth;
    splitterSizes << widgetWidth - tableVisibleWidth - this->handleWidth();
    setSizes(splitterSizes);
}

void ChildWidget::updateColWidthsOnSplitter(int /*pos*/, int /*index*/) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    table->horizontalHeader()->resizeSections(QHeaderView::Stretch);
    statisticsTable->horizontalHeader()->resizeSections(QHeaderView::Stretch);
}

bool ChildWidget::loadImage(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;

    QImage image;
    FILE   *fp;
    PIX    *pix;
    char   *filein;
    int    nPages = 0;

    filein = fileName.toLocal8Bit().data();
    fp = lept_fopen(filein, "rb");
    if (fileFormatIsTiff(fp)) {
        tiffGetCount(fp, &nPages);
        pix = pixReadStreamTiff(fp, currPage);  // open first page
        image = TessTools::PIX2qImage(pix);
        pixDestroy(&pix);
        lept_fclose(fp);
    } else {
        //  pixReadStream/PIX2qImage was not able to display png image
        //  So lets use QImage for other format than tiff...
        image.load(fileName);
    }

    if (image.isNull()) {
        QMessageBox::information(this, tr("Wrong file"),
                                 tr("Cannot load %1.").arg(fileName));
        return false;
    }
    if (nPages > 1) {
        currentPage->setMaximum(nPages);
        currentPage->setMinimum(1);
        numberOfPages->setText(tr("of %1").arg(nPages));
        connect(currentPage, SIGNAL(valueChanged(int)), this,
                SLOT(slotChangePage(int)));
    } else {
        pageWidget->hide();
    }
    imageHeight = image.height();
    imageWidth = image.width();
    setCurrentImageFile(fileName);
    QString boxFileName = QFileInfo(fileName).path() + "/"  // QDir::separator()
            + QFileInfo(fileName).completeBaseName() + ".box";

    if (!QFile::exists(boxFileName)) {
        qCreateBoxes(boxFileName);
    } else {
        if (!loadBoxes(boxFileName)) return false;
    }

    setCurrentBoxFile(boxFileName);
    setFileWatcher(boxFileName);
    imageItem = imageScene->addPixmap(QPixmap::fromImage(image));
    modified = false;
    emit modifiedChanged();
    connect(model, SIGNAL(itemChanged(QStandardItem*)), this,
            SLOT(emitBoxChanged()));
    connect(model, SIGNAL(itemChanged(QStandardItem*)), this,
            SLOT(documentWasModified()));
    return true;
}

bool ChildWidget::qCreateBoxes(const QString &boxFileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    loadTable();
    save(boxFileName);
    modified = false;
    emit modifiedChanged();
    return true;
}

bool ChildWidget::makeBoxPage() {
    if (imageFile.isEmpty())
        return false;
    QImage image;
    if (pageWidget->isHidden()) {  // one page - QImage is ok
        image.load(imageFile);
    } else {  // multipage - use leptonica
        PIX    *pix;
        pix = pixReadTiff(imageFile.toLocal8Bit().data(), currPage);
        image = TessTools::PIX2qImage(pix);
        pixDestroy(&pix);
    }

    TessTools tt;
    QString str = tt.makeBoxes(image, currPage);
    if (str == "")
        return false;

    QTextStream boxdata(&str);
    readToVector(boxdata);
    return true;
}

void ChildWidget::loadTable() {
    bool showFontColumns = isFontColumnsShown();
    cleanTable();
    initTable();
    setShowFontColumns(showFontColumns);

    if (fillTableData(currPage)) {
        table->setCurrentIndex(model->index(0, 0));
        updateSelectionRects();
    }
}

bool ChildWidget::readToVector(QTextStream &boxdata) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    boxdata.setCodec("UTF-8");
    QString data = boxdata.readAll();
    QStringList lineBoxes = data.split(QRegExp("\n"),
                                       QString::SkipEmptyParts);
    QString pagePrev = "0";
    QVector<QStringList> page;

    for (int i = 0; i < lineBoxes.size(); ++i) {
        QString line = lineBoxes.at(i);
        QStringList box = line.split(" ");
        if (box.size() == 7) {
            if (line.startsWith(" "))
                box.removeFirst ();  // tess2image generate also box for spaces
        } else if (box.size() != 6) {
            qDebug() << "box:" << box;
            QMessageBox::warning(this, SETTING_APPLICATION,
                                 tr("File can not be loaded because of wrong "
                                    "(non tesseract-ocr 3.02) box "
                                    "file format at line '%1'! (box.size: %2)")
                                 .arg(i + 1).arg (box.size()));
            QApplication::restoreOverrideCursor();
            return false;
        }

        if (box[5] == pagePrev) {
            page.append(box);
        } else {
            pagePrev = box[5];
            pages.append(page);
            page.clear();
            page.append(box);
        }
    }
    pages.append(page);
    return true;
}

bool ChildWidget::fillTableData(int pageNum) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;

    if (pageNum > (pages.size() - 1)) {
        switch (QMessageBox::question(
                    this,
                    tr("Warning: Missing data!"),
                    tr("There are no data for page %1.\n" \
                       "Should I create them?")
                    .arg(pageNum + 1),
                    QMessageBox::Yes |
                    QMessageBox::No)) {
        case QMessageBox::Yes: {
            makeBoxPage();
            documentWasModified();
            break;
        }
        case QMessageBox::No:
        case QMessageBox::Cancel:
        default:
            return false;
            break;
        }
    }
    int row = 0;
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // Stop some table features to improve update performance
    QFlags<QAbstractItemView::EditTrigger> oldEditTriggers = table->editTriggers();
    table->setUpdatesEnabled(false);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // There could be no data for requested page e.g. makeboxpage failed.
    if (pageNum >= pages.size()) {
        QApplication::restoreOverrideCursor();
        return false;
    }

    QVector<QStringList> pageData = pages[pageNum];
    for (int i = 0; i < pageData.size(); ++i) {
        QFont letterFont;
        QStringList pieces = pageData[i];
        QString letter = pieces.value(0);
        bool bold = false, italic = false, underline = false;
        // formating is present only in case there are more than 2 letters
        if (letter.at(0) == '@' && letter.size() > 1) {
            bold = true;
            letterFont.setBold(true);
            letter.remove(0, 1);
        }
        if (letter.at(0) == '$' && letter.size() > 1) {
            italic = true;
            letterFont.setItalic(true);
            letter.remove(0, 1);
        }
        if (letter.at(0) == '\'' && letter.size() > 1) {
            underline = true;
            letterFont.setUnderline(true);
            letter.remove(0, 1);
        }
        int left = pieces.value(1).toInt();
        int bottom = imageHeight - pieces.value(2).toInt();
        int right = pieces.value(3).toInt();
        int top = imageHeight - pieces.value(4).toInt();
        int page = pieces.value(5).toInt();

        model->insertRow(row);
        model->setData(model->index(row, 0, QModelIndex()), letterFont,
                       Qt::FontRole);
        model->setData(model->index(row, 0, QModelIndex()), letter);
        model->setData(model->index(row, 1, QModelIndex()), left);
        model->setData(model->index(row, 2, QModelIndex()), bottom);
        model->setData(model->index(row, 3, QModelIndex()), right);
        model->setData(model->index(row, 4, QModelIndex()), top);
        model->setData(model->index(row, 5, QModelIndex()), page);
        model->setData(model->index(row, 6, QModelIndex()), italic);
        model->setData(model->index(row, 7, QModelIndex()), bold);
        model->setData(model->index(row, 8, QModelIndex()), underline);
        createModelItemBox(row);
        row++;
    }

    // Set table features
    table->resizeRowsToContents();
    table->setCornerButtonEnabled(true);
    table->setWordWrap(true);
    calculateLettersTableWidth();

    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(oldEditTriggers);
    table->setUpdatesEnabled(true);

    QApplication::restoreOverrideCursor();
    return true;
}

bool ChildWidget::loadBoxes(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QFile file(fileName);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, SETTING_APPLICATION,
                             tr("Cannot read file %1:\n%2.").arg(fileName).arg(
                                 file.errorString()));
        return false;
    }
    QTextStream boxdata(&file);
    if (!readToVector(boxdata)) {
        return false;
    }
    if (!fillTableData(0)) {
        return false;
    }
    file.close();
    return true;
}

void ChildWidget::setFileWatcher(const QString & fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (fileWatcher) {
        fileWatcher->removePaths(fileWatcher->files());
    } else {
        fileWatcher = new QFileSystemWatcher(this);
        connect(fileWatcher, SIGNAL(fileChanged(const QString &)),
                this, SLOT(slotfileChanged(const QString &)));
    }
    fileWatcher->addPath(fileName);
}

void ChildWidget::slotfileChanged(const QString &fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (!QFile::exists(fileName)) {
        switch (QMessageBox::question(
                    this,
                    tr("Warning: File was removed!"),
                    tr("File '%1' was removed from disk." \
                       " Do you want to store your current work to disk?")
                    .arg(fileName),
                    QMessageBox::Yes |
                    QMessageBox::No,
                    QMessageBox::No)) {
        case QMessageBox::Yes: {
            save(fileName);
            break;
        }
        case QMessageBox::No:
        case QMessageBox::Cancel:
        default:
            // inform user that saved copy does not match to QBE copy
            documentWasModified();
            break;
        }
    } else {
        switch (QMessageBox::question(
                    this,
                    tr("Warning: File was modified..."),
                    tr("File '%1' was modified outside of %2.\nReload it?\n\n" \
                       "Warning: This operation can not be undone!")
                    .arg(fileName).arg(SETTING_APPLICATION),
                    QMessageBox::Yes |
                    QMessageBox::No,
                    QMessageBox::No)) {
        case QMessageBox::Yes: {
            reload(fileName);
            break;
        }
        case QMessageBox::No:
        case QMessageBox::Cancel:
            // inform user that saved copy does not match to QBE copy
            documentWasModified();
        default:
            break;
        }
    }
}

bool ChildWidget::reload(const QString& fileName) {
    if (boxesVisible) {
        drawBoxes();
    }
    deleteModelItemBox(table->currentIndex().row());
    bool showFontColumns = isFontColumnsShown();
    model->clear();
    delete selectionModel;
    delete model;
    pages.clear();


    initTable();
    setShowFontColumns(showFontColumns);
    loadBoxes(fileName);
    updateSelectionRects();

    modified = false;
    emit modifiedChanged();
    return true;
}

/**
 * @brief reload current image/page from image file
  */
bool ChildWidget::reloadImg() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    imageScene->removeItem(static_cast<QGraphicsItem*>(imageItem));
    delete imageItem;
    QImage image;
    if (pageWidget->isHidden()) {  // one page - QImage is ok
        image.load(imageFile);
    } else {  // multipage - use leptonica
        PIX    *pix;
        pix = pixReadTiff(imageFile.toLocal8Bit().data(), currPage);
        image = TessTools::PIX2qImage(pix);
        pixDestroy(&pix);
    }
    imageItem = imageScene->addPixmap(QPixmap::fromImage(image));
    return true;
}

bool ChildWidget::save(const QString& fileName) {
    // TODO(zdenop): support multipage!
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;

    storePage();
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox::warning(
                    this,
                    SETTING_APPLICATION,
                    tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false;
    }

    if (fileWatcher) {
        delete fileWatcher;
        fileWatcher = 0;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    QApplication::setOverrideCursor(Qt::WaitCursor);

    for (int i = 0; i < pages.size(); ++i) {
        QVector<QStringList> page = pages[i];
        for (int j = 0; j < page.size(); ++j) {
            QStringList box = page[j];
            for ( QStringList::Iterator it = box.begin(); it != box.end(); ++it ) {
                out << *it;
                if (it != (box.end() - 1))
                    out << " ";
            }
            out << "\n";
        }
    }

    file.close();
    QApplication::restoreOverrideCursor();

    modified = false;
    emit modifiedChanged();
    setFileWatcher(fileName);
    return true;
}

bool ChildWidget::splitToFeatureBF(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // Todo(zdenop): there must be a smarter way how to do this

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString normBoxes = "", boldBoxes = "", italicBoxes = "", boldItaBoxes = "";
    QString underBoxes = "";
    for (int row = 0; row < model->rowCount(); ++row) {
        QString letter = model->index(row, 0).data().toString();
        int left = model->index(row, 1).data().toInt();
        int bottom = model->index(row, 2).data().toInt();
        int right = model->index(row, 3).data().toInt();
        int top = model->index(row, 4).data().toInt();
        int page = model->index(row, 5).data().toInt();
        bool italic = model->index(row, 6).data().toBool();
        bool bold = model->index(row, 7).data().toBool();
        bool underline = model->index(row, 8).data().toBool();

        if (bold && !italic) {
            boldBoxes += QString("%1 %2 %3 %4 %5 %6\n").arg(letter).arg(left)
                    .arg(imageHeight - bottom).arg(right).arg(imageHeight - top)
                    .arg(page);
        } else if (italic && !bold) {
            italicBoxes += QString("%1 %2 %3 %4 %5 %6\n").arg(letter).arg(left)
                    .arg(imageHeight - bottom).arg(right)
                    .arg(imageHeight - top).arg(page);
        } else if (italic && bold) {
            boldItaBoxes += QString("%1 %2 %3 %4 %5 %6\n").arg(letter).arg(left)
                    .arg(imageHeight - bottom).arg(right)
                    .arg(imageHeight - top).arg(page);
        } else if (underline) {
            underBoxes += QString("%1 %2 %3 %4 %5 %6\n").arg(letter).arg(left)
                    .arg(imageHeight - bottom).arg(right).arg(imageHeight - top)
                    .arg(page);
        } else {
            normBoxes += QString("%1 %2 %3 %4 %5 %6\n").arg(letter).arg(left)
                    .arg(imageHeight - bottom).arg(right).arg(imageHeight - top)
                    .arg(page);
        }
    }

    // find path + name + ext:
    int dotCount = QFileInfo(fileName).fileName().count(".");
    QStringList results = QFileInfo(fileName).fileName().split(".");
    QString path, base, ext, imgExt;
    path = QFileInfo(fileName).path() + QDir::separator();

    if (dotCount < 3) {
        base = QFileInfo(fileName).baseName();
        ext = QFileInfo(fileName).completeSuffix();
    } else  {
        for (int dot = 0; dot < (dotCount - 1); ++dot) {
            base += results[dot];
            if (dot < (dotCount - 2))
                base += ".";
        }
        ext = results[(dotCount - 1)] + "." + results[dotCount];
    }
    imgExt = ext;
    imgExt.replace(imgExt.size() - 3 , 3, "png");

    if (normBoxes.size()) {
        saveString(path + base + "normal." + ext, normBoxes);
        createStringImage(path + base + "normal." + imgExt, normBoxes);
    }

    if (boldBoxes.size()) {
        saveString(path + base + "bold." + ext, boldBoxes);
        createStringImage(path + base + "bold." + imgExt, boldBoxes);
    }

    if (italicBoxes.size()) {
        saveString(path + base + "italic." + ext, italicBoxes);
        createStringImage(path + base + "italic." + imgExt, italicBoxes);
    }

    if (boldItaBoxes.size()) {
        saveString(path + base + "bolditalic." + ext, boldItaBoxes);
        createStringImage(path + base + "bolditalic." + imgExt, boldItaBoxes);
    }

    if (underBoxes.size()) {
        saveString(path + base + "underline." + ext, underBoxes);
        createStringImage(path + base + "underline." + imgExt, underBoxes);
    }

    QApplication::restoreOverrideCursor();
    return true;
}

bool ChildWidget::saveString(const QString& fileName, const QString& qData) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(
                    this,
                    SETTING_APPLICATION,
                    tr("Cannot write file %1:\n%2.").arg(fileName).arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << qData;
    file.close();
    return true;
}

/*
 * Copy defined boxes to new image file
 */

bool ChildWidget::createStringImage(const QString& fileName,
                                    const QString& qData) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QImage image = gItem2qImage();
    QImage result = image;
    result.fill(Qt::white);

    QPainter painter(&result);
    QStringList rowOfData = qData.split("\n");
    for (int x = 0; x < rowOfData.size(); x++) {
        QStringList rowData = rowOfData.at(x).split(" ");
        if (rowData.size() < 4)
            continue;  // skip rows without enough items
        int x0 = rowData[1].toInt();
        int y0 = imageHeight - rowData[2].toInt();
        int w = rowData[3].toInt() - x0;
        int h = rowData[4].toInt() - rowData[2].toInt() ;
        QImage srcImage = image.copy(x0, y0 - h, w, h);
        painter.drawImage(QPoint(x0, y0 - h), srcImage);
    }
    painter.end();
    result.save(fileName, 0);
    return true;
}

bool ChildWidget::importSPLToChild(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QFile file(fileName);

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, SETTING_APPLICATION,
                             tr("Cannot read file %1:\n%2.").arg(fileName).arg(
                                 file.errorString()));
        return false;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    // TODO(zdenop): ask for format of input file
    // format_1: 1 line = 1 symbol
    // format_2: 1 letter = 1 symbol

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString line;
    int row = 0;
    do {
        line = in.readLine();
        if (!line.isEmpty()) {
            if (row > model->rowCount()) {
                QMessageBox::warning(this, SETTING_APPLICATION,
                                     tr("There are more symbols in import file than " \
                                        "boxes!\nRest of symbols are ignored."));
                file.close();
                QApplication::restoreOverrideCursor();
                return true;
            }
            model->setData(model->index(row, 0, QModelIndex()), line);
            row++;
        }
    } while (!line.isEmpty());

    if (row < model->rowCount()) {
        QMessageBox::warning(this, SETTING_APPLICATION,
                             tr("There are less symbols in import file than boxes!")
                             .arg(fileName).arg(file.errorString()));
    }

    file.close();
    QApplication::restoreOverrideCursor();

    modified = true;
    emit modifiedChanged();

    return true;
}


bool ChildWidget::importTextToChild(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // TODO(zdenop): code clean up, and join with importSPLToChild

    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       SETTING_ORGANIZATION, SETTING_APPLICATION);

    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, SETTING_APPLICATION,
                             tr("Cannot read file %1:\n%2.").arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    if (settings.contains("Text/OpenDialog") != true ||
            (settings.value("Text/OpenDialog").toBool())) {
        SettingsDialog *runSettingsDialog = new SettingsDialog(this, 1);
        runSettingsDialog->exec();
    }

    QList<QString> ligatures = settings.value("Text/Ligatures").toString()
            .split("\n");

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QApplication::setOverrideCursor(Qt::WaitCursor);
    QString line;
    QString symbolText;

    do {
        line = in.readLine();
        if (!line.isEmpty()) {
            symbolText += line;
        }
    } while (!line.isEmpty());
    file.close();

    // check if there are ligatures in line
    for (int i = 0; i < ligatures.size(); ++i) {
        symbolText.replace(ligatures[i], QString("\n%1\n").arg(ligatures[i]));
    }

    QStringList lines;
    QString oneLetter, letters;
    QList<QString> symbols;

    lines = symbolText.split("\n");  // split string to lines
    for (int i = 0; i < lines.size(); ++i) {
        letters = lines.at(i);
        bool ligature = false;
        for (int m = 0; m < ligatures.size(); ++m) {
            if (ligatures.at(m) == letters) {  // this is ligature
                symbols.append(letters);
                ligature = true;
            }
        }
        if (ligature == false) {
            letters = letters.remove(QRegExp("\\s+"));  // remove whitespace
            for (int j = 0; j < letters.size(); ++j) {  // split lines to letters
                oneLetter = letters[j];
                symbols.append(oneLetter);
            }
        }
    }
    if (symbols.size() != model->rowCount()) {
        QMessageBox::warning(this, SETTING_APPLICATION,
                             tr("Number of symbols in import file differ with " \
                                "number of boxes!"));
    }

    for (int i = 0; i < symbols.size(); ++i) {
        model->setData(model->index(i, 0, QModelIndex()), symbols.at(i));
    }

    QApplication::restoreOverrideCursor();

    modified = true;
    emit modifiedChanged();

    return true;
}

/**
   * Export symbols from table to text file. eType identify export format:
   * 1 - one symbol per line
   * 2 - one row per line
   * 3 - one paragraph per line
   * Returns false if export was not possible for some reason.
   *
   * Export will work only on one column text correctly.
   * Words are identified if space between boxes is bigger than 6.
   * Lines are identified if space between boxes is bigger than -6.
   * Paragraph is identified based on left indentation (15) of from last left
   * margin.
*/
bool ChildWidget::exportTxt(const int& eType, const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QFile file(fileName);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(
                    this,
                    SETTING_APPLICATION,
                    tr("Cannot write file %1:\n%2.").arg(boxFile).arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       SETTING_ORGANIZATION, SETTING_APPLICATION);
    int line_start_prev = 0;
    int line_end_prev = 0;
    int right_prev = -1;
    int last_bottom = -1;
    int wordSpace = settings.value("Text/WordSpace").toInt();
    int paraIndent = settings.value("Text/ParagraphIndent").toInt();

    for (int row = 0; row < model->rowCount(); ++row) {
        QString letter = model->index(row, 0).data().toString();
        int left = model->index(row, 1).data().toInt();
        int bottom = model->index(row, 2).data().toInt();
        int right = model->index(row, 3).data().toInt();
        int top = model->index(row, 4).data().toInt();

        if (last_bottom == -1)
            last_bottom = top;

        if (eType == 1 && (right_prev != -1)) {
            out << "\n";
        }

        // line by line
        if (eType == 2) {
            if (((left - right_prev) >= wordSpace) && (right_prev != -1)) {
                // new word
                out << " ";
            } else if (((left - right_prev) <= (2 * (left - right))) &&
                       (right_prev != -1)) {
                // new line -> if negative difference is bigger than double of letter
                // width
                out << "\n";
            }
        }

        if (eType == 3) {
            if (((left - right_prev) >= wordSpace) && (right_prev != -1))
                // new word
                out << " ";

            if (((left - right_prev) <= (2 * (left - right))) && (right_prev != -1)) {
                // new line -> if negative difference is bigger than double of letter
                // width
                if (line_end_prev == 0)  // first line
                    line_end_prev = right;
                if ((left - line_start_prev >= paraIndent) ||  // distance from left
                        (top - last_bottom >= paraIndent) ||   // distance between lines
                        (abs(right - line_end_prev) >= (paraIndent / 2)))
                    // distance from right
                    out << "\n";
                else
                    out << " ";
                line_start_prev = left;
                line_end_prev = right_prev;
            }
        }

        out << letter;
        right_prev = right;
        last_bottom = bottom;
    }

    out << "\n";
    QApplication::restoreOverrideCursor();
    return true;
}

bool ChildWidget::isBoxSelected() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return selectionModel->hasSelection();
}

bool ChildWidget::isItalic() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.isValid()) {
        return model->index(index.row(), 6).data().toBool();
    }
    return false;
}

bool ChildWidget::isBold() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.isValid()) {
        return model->index(index.row(), 7).data().toBool();
    }
    return false;
}

bool ChildWidget::isUnderLine() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.isValid()) {
        return model->index(index.row(), 8).data().toBool();
    }
    return false;
}

bool ChildWidget::isShowSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return symbolShown;
}

bool ChildWidget::isDirectTypingMode() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return directTypingMode;
}

bool ChildWidget::isFontColumnsShown() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return (!table->isColumnHidden(6));
}

void ChildWidget::setDirectTypingMode(bool v) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    directTypingMode = v;
}

void ChildWidget::setShowFontColumns(bool v) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    table->setColumnHidden(6, !v);
    table->setColumnHidden(7, !v);
    table->setColumnHidden(8, !v);
    calculateLettersTableWidth();
    table->horizontalHeader()->resizeSections(QHeaderView::Stretch);
}

bool ChildWidget::isDrawBoxes() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return boxesVisible;
}

bool ChildWidget::isDrawRect() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return drawnRectangle;
}

void ChildWidget::setItalic(bool v) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    QModelIndex index;
    QFont letterFont;

    foreach(index, indexes) {
        // IsItalic?
        bool current = model->index(index.row(), 6).data().toBool();
        if (current != v) {
            UndoItem ui;
            ui.m_eop = euoChange;
            ui.m_origrow = index.row();

            for (int ii = 0; ii < model->columnCount(); ii++)
                ui.m_vdata[ii] = model->index(ui.m_origrow, ii).data();

            m_undostack.push(ui);

            letterFont = model->data(model->index(index.row(), 0, QModelIndex()),
                                     Qt::FontRole).value<QFont>();
            letterFont.setItalic(v);
            model->setData(model->index(index.row(), 0, QModelIndex()), letterFont,
                           Qt::FontRole);
            model->setData(model->index(index.row(), 6, QModelIndex()), v);
        }
    }
}

void ChildWidget::setBolded(bool v) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    QModelIndex index;
    QFont letterFont;

    foreach(index, indexes) {
        // IsBool?
        bool current = model->index(index.row(), 7).data().toBool();
        if (current != v) {
            UndoItem ui;
            ui.m_eop = euoChange;
            ui.m_origrow = index.row();

            for (int ii = 0; ii < model->columnCount(); ii++)
                ui.m_vdata[ii] = model->index(ui.m_origrow, ii).data();

            m_undostack.push(ui);

            letterFont = model->data(model->index(index.row(), 0, QModelIndex()),
                                     Qt::FontRole).value<QFont>();
            letterFont.setBold(v);

            model->setData(model->index(index.row(), 0, QModelIndex()), letterFont,
                           Qt::FontRole);
            model->setData(model->index(index.row(), 7, QModelIndex()), v);
        }
    }
}

void ChildWidget::setUnderline(bool v) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    QModelIndex index;
    QFont letterFont;

    foreach(index, indexes) {
        // IsUnderLine?
        bool current = model->index(index.row(), 8).data().toBool();
        if (current != v) {
            UndoItem ui;
            ui.m_eop = euoChange;
            ui.m_origrow = index.row();

            for (int ii = 0; ii < model->columnCount(); ii++)
                ui.m_vdata[ii] = model->index(ui.m_origrow, ii).data();

            m_undostack.push(ui);

            letterFont = model->data(model->index(index.row(), 0, QModelIndex()),
                                     Qt::FontRole).value<QFont>();
            letterFont.setUnderline(v);
            model->setData(model->index(index.row(), 0, QModelIndex()), letterFont,
                           Qt::FontRole);
            model->setData(model->index(index.row(), 8, QModelIndex()), v);
        }
    }
}

/*
 * Replace original image with Thresholded image from tesseract
 */
void ChildWidget::binarizeImage() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    imageScene->removeItem(imageItem);
    QImage image = gItem2qImage();
    delete imageItem;
    QImage bImage = TessTools::GetThresholded(image);
    imageItem = imageScene->addPixmap(QPixmap::fromImage(bImage));
}

/*
 * Convert QGraphicsItem to QImage
 */
QImage ChildWidget::gItem2qImage(){
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QImage image(imageItem->boundingRect().size().toSize(),  QImage::Format_RGB32);
    QPainter painter(&image);
    QStyleOptionGraphicsItem styleOption;
    imageItem->paint(&painter, &styleOption);
    painter.end();
    return image;
}

void ChildWidget::setSelectionRect() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       SETTING_ORGANIZATION, SETTING_APPLICATION);
    QFont imageFont = settings.value("GUI/ImageFont").value<QFont>();

    if (imageFont.family().isEmpty()) {
        imageFont.setFamily(TABLE_FONT);
        imageFont.setPointSize(TABLE_FONT_SIZE);
    }

    imageFont.setPointSize((imageFont.pointSize() * 2));
}

void ChildWidget::setZoomStatus() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    qreal _zoomratio = imageView->transform().m11();
    emit zoomRatioChanged(_zoomratio);
}

void ChildWidget::setZoom(float scale) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QTransform transform;

    transform.scale(scale, scale);
    imageView->setTransform(transform);
    qreal _zoomratio = scale;
    emit zoomRatioChanged(_zoomratio);
}

void ChildWidget::zoomIn() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    imageView->scale(1.2, 1.2);
    if (selectionModel->hasSelection())
        imageView->ensureVisible(modelItemBox());
    setZoomStatus();
}

void ChildWidget::zoomOut() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    imageView->scale(1 / 1.2, 1 / 1.2);
    if (selectionModel->hasSelection())
        imageView->ensureVisible(modelItemBox());
    setZoomStatus();
}

void ChildWidget::zoomToFit() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    float viewWidth = imageView->viewport()->width();
    float viewHeight = imageView->viewport()->height();
    float zoomFactor;
    float ratio = viewWidth / viewHeight;
    float aspectRatio = static_cast<float>(imageWidth / imageHeight);

    if (ratio > aspectRatio) {
        zoomFactor = viewHeight / imageHeight;
    } else {
        zoomFactor = viewWidth / imageWidth;
    }

    setZoom(zoomFactor);
}

void ChildWidget::zoomToHeight() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    float viewHeight = imageView->viewport()->height();
    float zoomFactor = viewHeight / imageHeight;

    setZoom(zoomFactor);
    if (selectionModel->hasSelection())
        imageView->ensureVisible(modelItemBox());
}

void ChildWidget::zoomToWidth() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    float viewWidth = imageView->viewport()->width();
    float zoomFactor = viewWidth / imageWidth;

    setZoom(zoomFactor);
    if (selectionModel->hasSelection())
        imageView->ensureVisible(modelItemBox());
}

void ChildWidget::zoomOriginal() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    setZoom(1);
    if (selectionModel->hasSelection())
        imageView->ensureVisible(modelItemBox());
}

void ChildWidget::zoomToSelection() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (selectionModel->hasSelection()) {
        imageView->fitInView(modelItemBox(), Qt::KeepAspectRatio);
        imageView->scale(1 / 1.1, 1 / 1.1);    // make small border
        if (selectionModel->hasSelection())
            imageView->ensureVisible(modelItemBox());
        imageView->centerOn(modelItemBox());
        setZoomStatus();
    }
}

void ChildWidget::showSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (symbolShown == false)
        symbolShown = true;
    else
        symbolShown = false;
    updateSelectionRects();
}

void ChildWidget::drawRectangle(bool checked) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (checked) {
        if (!m_DrawRectangle) {
            m_DrawRectangle = new DrawRectangle(this, userFriendlyCurrentFile(),
                                                imageWidth, imageHeight);
        }
        int ret = m_DrawRectangle->exec();
        QRect newCoords = m_DrawRectangle->getRectangle();
        int x1, y1, x2, y2;
        newCoords.getCoords(&x1, &y1, &x2, &y2);
        const bool rectNotSet(x1 == 0 && y1 == 0 && x2 == 0 && y2 == 0);

        if (ret && !rectNotSet) {
            if (rectangle) {
                qDebug() << "Something went wrong. Object rectangle should not exist!";
            }

            const QPen redPen(QColor(255, 0, 0, 255));

            rectangle = imageScene->addRect(newCoords.x(), newCoords.y(),
                                            newCoords.width(), newCoords.height(),
                                            redPen,
                                            QBrush(QColor(255, 0, 0, 100)));
            rectangle->setZValue(1);
            rectangle->setVisible(true);

            vertLineLeft = imageScene->addLine(newCoords.x(), 0,
                                               newCoords.x(), imageScene->height(), redPen);
            vertLineLeft->setZValue(1);
            vertLineLeft->setVisible(true);

            vertLineRight = imageScene->addLine(newCoords.bottomRight().x(), 0,
                                                newCoords.bottomRight().x(), imageScene->height(), redPen);
            vertLineRight->setZValue(1);
            vertLineRight->setVisible(true);

            horLineTop = imageScene->addLine(0, newCoords.y(),
                                             imageScene->width(), newCoords.y(), redPen);
            horLineTop->setZValue(1);
            horLineTop->setVisible(true);

            horLineBottom = imageScene->addLine(0, newCoords.bottomRight().y(),
                                                imageScene->width(), newCoords.bottomRight().y(), redPen);
            horLineBottom->setZValue(1);
            horLineBottom->setVisible(true);

            drawnRectangle = true;
        }
    } else {
        if (rectangle) {
            // I can not call rectangle->prepareGeometryChange();
            // http://www.qtcentre.org/threads/28749-Qt-4.6-GraphicsView-items-remove-problem
            imageScene->removeItem(rectangle);
            delete rectangle;

            imageScene->removeItem(vertLineLeft);
            delete vertLineLeft;

            imageScene->removeItem(vertLineRight);
            delete vertLineRight;

            imageScene->removeItem(horLineTop);
            delete horLineTop;

            imageScene->removeItem(horLineBottom);
            delete horLineBottom;

            // Workaround
            QRectF isCoords = imageScene->sceneRect();
            imageScene->update(isCoords);

            drawnRectangle = false;
        }
    }
    emit drawRectangleChoosen();
}

QGraphicsRectItem* ChildWidget::modelItemBox(int row) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (selectionModel->hasSelection()) {
        if (row == -1)
            row = table->selectionModel()->selectedRows().last().row();
        return model->index(row, 9).data().value<QGraphicsRectItem*>();
    } else {
        return NULL;
    }
}

QGraphicsRectItem* ChildWidget::createModelItemBox(int row) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int left = model->index(row, 1).data().toInt();
    int bottom = model->index(row, 2).data().toInt();
    int right = model->index(row, 3).data().toInt();
    int top = model->index(row, 4).data().toInt();
    QGraphicsRectItem* rectItem =
            imageScene->addRect(left, top, right - left, bottom - top, QPen(boxColor));
    rectItem->setZValue(2);
    rectItem->hide();

    model->setData(model->index(row, 9), QVariant::fromValue(rectItem));

    return rectItem;
}

void ChildWidget::updateModelItemBox(int row) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int left = model->index(row, 1).data().toInt();
    int bottom = model->index(row, 2).data().toInt();
    int right = model->index(row, 3).data().toInt();
    int top = model->index(row, 4).data().toInt();
    modelItemBox(row)->setRect(left, top, right - left, bottom - top);
}

void ChildWidget::deleteModelItemBox(int row) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // we can not use modelItemBox(row) because we clear selections...
    QGraphicsRectItem* rectItem =
            model->index(row, 9).data().value<QGraphicsRectItem*>();
    imageScene->removeItem(rectItem);
    delete rectItem;
}

void ChildWidget::drawBoxes() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    boxesVisible = !boxesVisible;
    // workaround:  modelItemBox(row) requires selection to not segfault
    if (!selectionModel->hasSelection())
        table->setCurrentIndex(model->index(0, 0));
    for (int row = 0; row < model->rowCount(); ++row)
        modelItemBox(row)->setVisible(boxesVisible);
    if (boxesVisible)
        updateSelectionRects();
}

void ChildWidget::mousePressEvent(QMouseEvent* event) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // This handler is for left click events only
    if (event->button() != Qt::LeftButton)
        return;

    // zoom should be proportional => m11=m22
    qreal zoomFactor = imageView->transform().m22();
    int offset = this->sizes().first() + 6;  // 6 is estimated width  of splitter
    int zoomedOffset = offset / zoomFactor;

    // Rubber band
    if (event->modifiers() == Qt::ControlModifier) {
        setCursor(Qt::CrossCursor);
        rbOrigin = imageView->mapFromParent(event->pos());
        rbOrigin.setX(rbOrigin.x() - offset);
        rubberBand->setGeometry(QRect(rbOrigin, QSize()));
        rubberBand->show();
        grabMouse();
    } else if (event->modifiers() == Qt::NoModifier) {  // BB click selection
        for (int row = 0; row < model->rowCount(); ++row) {
            int left = model->index(row, 1).data().toInt();
            int bottom = model->index(row, 2).data().toInt();
            int right = model->index(row, 3).data().toInt();
            int top = model->index(row, 4).data().toInt();
            QPointF mouseCoordinates = imageView->mapToScene(event->pos());
            if ((left <= (mouseCoordinates.x() - zoomedOffset) &&
                 (mouseCoordinates.x() - zoomedOffset) <= right) &&
                    (top <= mouseCoordinates.y()) &&
                    (mouseCoordinates.y() <= bottom)) {
                table->setCurrentIndex(model->index(row, 0));
                table->setFocus();
            }
        }
    }  // else (BB selection)
}

void ChildWidget::mouseMoveEvent(QMouseEvent* event) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QPoint topleft;
    QPoint botright;
    int offset = this->sizes().first() + 6;  // 6 is estimated width  of splitter
    QPoint rbCurPoint = imageView->mapFromParent(event->pos());
    rbCurPoint.setX(rbCurPoint.x() - offset);
    if (rbOrigin.x() > rbCurPoint.x()) {
        topleft.setX(rbCurPoint.x());
        botright.setX(rbOrigin.x());
    } else {
        topleft.setX(rbOrigin.x());
        botright.setX(rbCurPoint.x());
    }
    if (rbOrigin.y() > rbCurPoint.y()) {
        topleft.setY(rbCurPoint.y());
        botright.setY(rbOrigin.y());
    } else {
        topleft.setY(rbOrigin.y());
        botright.setY(rbCurPoint.y());
    }
    rubberBand->setGeometry(QRect(topleft, botright));
}

void ChildWidget::mouseReleaseEvent(QMouseEvent* /*event*/) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    setCursor(Qt::ArrowCursor);
    releaseMouse();
    rubberBand->hide();

    // If a Ctrl+Click - toggle
    if (!rubberBand->size().isValid() || (rubberBand->size().width() == 0 &&
                                          rubberBand->size().height() == 0)) {
        QPoint pos = imageView->mapToScene(rubberBand->pos()).toPoint();
        for (int row = 0; row < model->rowCount(); ++row) {
            int left = model->index(row, 1).data().toInt();
            int bottom = model->index(row, 2).data().toInt();
            int right = model->index(row, 3).data().toInt();
            int top = model->index(row, 4).data().toInt();

            if (left <= pos.x() && pos.x() <= right &&
                    top <= pos.y() && pos.y() <= bottom) {
                table->selectionModel()->select(model->index(row, 0),
                                                QItemSelectionModel::Toggle |
                                                QItemSelectionModel::Rows);
                break;
            }
        }  // for row
        // end of if click
    } else {  // If rubber band - add to selection
        QRect rect(rubberBand->pos(), rubberBand->size());
        QPoint topleft = imageView->mapToScene(rect.topLeft()).toPoint();
        QPoint botright = imageView->mapToScene(rect.bottomRight()).toPoint();
        QItemSelection selection;
        for (int row = 0; row < model->rowCount(); ++row) {
            int cx = (model->index(row, 1).data().toInt() +
                      model->index(row, 3).data().toInt())/2;
            int cy = (model->index(row, 2).data().toInt() +
                      model->index(row, 4).data().toInt())/2;

            if (cx >= topleft.x() && cx <= botright.x() && cy >= topleft.y() &&
                    cy <= botright.y())
                selection.push_back(QItemSelectionRange(model->index(row, 0)));
        }
        table->selectionModel()->select(selection, QItemSelectionModel::Select |
                                        QItemSelectionModel::Rows);
    }   // if rubber band

    table->setFocus();
    updateSelectionRects();

    // Focus the last symbol in the selection
    if (!table->selectionModel()->hasSelection() &&
            table->selectionModel()->selection().size() > 0)
        table->selectionModel()->setCurrentIndex(
                    table->selectionModel()->selectedRows().last(),
                    QItemSelectionModel::NoUpdate);
}

bool ChildWidget::eventFilter(QObject* object, QEvent* event) {
    if (DMESS > 11) qDebug() << Q_FUNC_INFO;
    switch (event->type())  {
    case QEvent::KeyPress: {
        // transforms QEvent into QKeyEvent
        QKeyEvent* pKeyEvent = static_cast<QKeyEvent*>(event);
        if (pKeyEvent->modifiers() & Qt::ControlModifier) {
            switch (pKeyEvent->key()) {
            case Qt::Key_C: {
                copyFromCell();
                break;
            }
            case Qt::Key_V: {
                pasteToCell();
                break;
            }
            }
            pKeyEvent->accept();
            return true;
        } else if (directTypingMode) {
            return directType(pKeyEvent);
        } else {
            return QWidget::eventFilter(object, pKeyEvent);
        }
    }  // end case KeyPress
    case QEvent::GraphicsSceneMouseMove: {
        rubberBand->setGeometry(QRect(QPoint(0, 0), QSize(0, 0)));
        rubberBand->show();
    }  // end case GraphicsSceneMouseMove

    case QEvent::GraphicsSceneWheel: {
        QGraphicsSceneWheelEvent *wheelEvent =
                static_cast<QGraphicsSceneWheelEvent*>(event);
        if (wheelEvent->modifiers().testFlag(Qt::ControlModifier)) {
            int delta = wheelEvent->delta();
            if (delta > 0) {
                imageView->scale(1.10, 1.10);
            } else {
                imageView->scale(1/1.10, 1/1.10);
            }
            wheelEvent->accept();
            setZoomStatus();
            return true;
        } else {
            return false;
        }
    }  // end case GraphicsSceneWheel
    default:
        return false;
    }  // end switch
    return false;
}

void ChildWidget::moveSymbolRow(int direction) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    int currentRow = index.row();
    // check if any row is selected
    if (currentRow < 0)
        return;

    // check top/bottom movements
    if ((direction + currentRow < 0) ||
            (direction + currentRow + 1 > model->rowCount())) {
        QString message = tr("Non existing destination row!");
        emit statusBarMessage(message);
        return;
    } else {
        if (abs(direction) == 1) {  // This works only for moveUp/moveDown!!!
            UndoItem ui;
            ui.m_eop = euoReplace;
            ui.m_origrow = currentRow;
            ui.m_extrarow = currentRow + direction;

            for (int j = 0; j < model->columnCount() - 1; j++) {
                ui.m_vdata[j] = model->index(currentRow, j).data();
                ui.m_vextradata[j] = model->index(ui.m_extrarow, j).data();

                model->setData(model->index(ui.m_extrarow, j), ui.m_vdata[j]);
                model->setData(model->index(ui.m_origrow, j), ui.m_vextradata[j]);
            }

            m_undostack.push(ui);
            updateModelItemBox(ui.m_extrarow);
            updateModelItemBox(ui.m_origrow);
            // activate new row
            table->setCurrentIndex(model->index(ui.m_extrarow, 0));
        } else {
            // TODO(zdenop): implement undo, or rewrite whole function
            UndoItem ui;
            ui.m_eop = euoMove;
            ui.m_origrow = currentRow;
            ui.m_extrarow = currentRow + direction;

            int newRow = ui.m_extrarow;
            if (direction > 0)  // insertRow change row id!
                newRow++;
            else
                currentRow++;
            model->insertRow(newRow);

            for (int i = 0; i < (model->columnCount() - 1); ++i) {
                ui.m_vdata[i] = model->index(currentRow, i).data();
                model->setData(model->index(newRow, i),
                               model->index(currentRow, i).data());
            }
            m_undostack.push(ui);

            QGraphicsRectItem* rectItem = createModelItemBox(newRow);
            // activate new row
            table->setCurrentIndex(model->index(newRow, 0));
            if (boxesVisible)
                rectItem->show();
            // delete original row
            model->removeRow(currentRow);
        }
        updateSelectionRects();
        emit modifiedChanged();
    }
}

void ChildWidget::copyFromCell() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(table->currentIndex().data().toString());
}

void ChildWidget::pasteToCell() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    const QClipboard* clipboard = QApplication::clipboard();
    QModelIndex index = selectionModel->currentIndex();

    UndoItem ui;
    ui.m_eop = euoChange;
    ui.m_origrow = index.row();

    for (int i = 0; i < model->columnCount(); i++)
        ui.m_vdata[i] = model->index(ui.m_origrow, i).data();

    // do not paste string to int fields
    if ((index.column() > 0 && index.column() < 5) &&
            (clipboard->text().toInt() > 0)) {
        model->setData(table->currentIndex(), clipboard->text().toInt());
        m_undostack.push(ui);
    }

    // paste string only to string field
    if (index.column() == 0) {
        model->setData(table->currentIndex(), clipboard->text());
        m_undostack.push(ui);
    }

    if (directTypingMode)
        table->setCurrentIndex(model->index(index.row() + 1, 0));

    updateModelItemBox(index.row());
    updateSelectionRects();
}

bool ChildWidget::directType(QKeyEvent* event) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.column() > 0) {
        table->setCurrentIndex(model->index(index.row(), 0));
        return true;
    } else {
        // TODO(zdenop): Find solution for QT5
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
        if (!event->text().toAscii().trimmed().isEmpty() &&
        #else
        if (!event->text().trimmed().isEmpty() &&
        #endif
                (event->key() !=  Qt::Key_Delete))  {
            // enter only text
            model->setData(model->index(index.row(), 0, QModelIndex()),
                           event->text());

            UndoItem ui;
            ui.m_eop = euoChange;
            ui.m_origrow = index.row();

            for (int i = 0; i < model->columnCount(); i++)
                ui.m_vdata[i] = model->index(ui.m_origrow, i).data();

            m_undostack.push(ui);
            table->setCurrentIndex(model->index(index.row() + 1, 0));
        } else {
            if ((event->key() ==  Qt::Key_Enter) ||
                    (event->key() ==  Qt::Key_Return)) {
                // enter/return move to next row
                table->setCurrentIndex(model->index(index.row() + 1, 0));
            } else {
                // no symbol, no enter (e.g. arrow keys
                return false;
            }
        }
        event->accept();
        return true;
    }
}

void ChildWidget::insertSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid())
        return;

    int leftBorder = model->index(index.row(), 3).data().toInt() + 1;
    int rightBorder = model->index(index.row() + 1, 1).data().toInt() - 1;

    if (leftBorder > rightBorder)  // end of line or overlapping boxes
        rightBorder = leftBorder +
                (leftBorder - model->index(index.row(), 1).data().toInt());

    int newrow = index.row() + 1;
    model->insertRow(newrow);
    model->setData(model->index(newrow, 0), "*");
    model->setData(model->index(newrow, 1), leftBorder);
    model->setData(model->index(newrow, 2),
                   model->index(index.row(), 2).data().toInt());
    model->setData(model->index(newrow, 3), rightBorder);
    model->setData(model->index(newrow, 4),
                   model->index(index.row(), 4).data().toInt());
    model->setData(model->index(newrow, 5),
                   model->index(index.row(), 5).data().toInt());
    model->setData(model->index(newrow, 6),
                   model->index(index.row(), 6).data().toBool());
    model->setData(model->index(newrow, 7),
                   model->index(index.row(), 7).data().toBool());
    model->setData(model->index(newrow, 8),
                   model->index(index.row(), 8).data().toBool());

    UndoItem ui;
    ui.m_eop = euoAdd;
    ui.m_origrow = newrow;

    // For redo
    for (int ii = 0; ii < model->columnCount(); ii++)
        ui.m_vdata[ii] = model->index(ui.m_origrow, ii).data();

    m_undostack.push(ui);

    QGraphicsRectItem* rectItem = createModelItemBox(newrow);
    if (boxesVisible)
        rectItem->show();
    table->setCurrentIndex(model->index(newrow, 0));
    table->setFocus();

    updateSelectionRects();
    emit modifiedChanged();
}

void ChildWidget::splitSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid())
        return;

    UndoItem ui;
    ui.m_eop = euoSplit;
    ui.m_origrow = index.row();
    ui.m_extrarow = ui.m_origrow + 1;

    for (int i = 0; i < model->columnCount(); i++)
        ui.m_vdata[i] = model->index(ui.m_origrow, i).data();

    m_undostack.push(ui);

    QModelIndex left = model->index(index.row(), 1);
    QModelIndex right = model->index(index.row(), 3);
    int width = right.data().toInt() - left.data().toInt();
    model->insertRow(index.row() + 1);
    model->setData(model->index(index.row() + 1, 0), "*");
    model->setData(model->index(index.row() + 1, 1),
                   right.data().toInt() - width / 2);
    model->setData(model->index(index.row() + 1, 2),
                   model->index(index.row(), 2).data().toInt());
    model->setData(model->index(index.row() + 1, 3), right.data().toInt());
    model->setData(model->index(index.row() + 1, 4),
                   model->index(index.row(), 4).data().toInt());
    model->setData(model->index(index.row() + 1, 5),
                   model->index(index.row(), 5).data().toInt());
    model->setData(model->index(index.row() + 1, 6),
                   model->index(index.row(), 6).data().toBool());
    model->setData(model->index(index.row() + 1, 7),
                   model->index(index.row(), 7).data().toBool());
    model->setData(model->index(index.row() + 1, 8),
                   model->index(index.row(), 8).data().toBool());
    model->setData(right, right.data().toInt() - width / 2);

    updateModelItemBox(index.row());
    QGraphicsRectItem* rectItem = createModelItemBox(index.row() + 1);
    if (boxesVisible)
        rectItem->show();
    updateSelectionRects();
    emit modifiedChanged();
}

void ChildWidget::joinSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = selectionModel->selectedRows();
    if (indexes.empty())
        return;
    // On single selected item join with the next ...
    if (indexes.size() == 1) {
        // ... if selected is not the last
        if (indexes.back().row() != model->rowCount() - 1) {
            indexes.push_back(model->index(indexes.back().row() + 1, 0));
        } else {
            return;
        }
    }

    QString letter = "";
    int left = INT_MAX;
    int top = INT_MAX;
    int right = INT_MIN;
    int bottom = INT_MIN;
    int page = INT_MAX;
    bool italic = false;
    bool bold = false;
    bool underline = false;

    int targetRow = indexes.front().row();

    QStack<UndoItem> joinstack;
    UndoItem tempJoin;
    tempJoin.m_origrow = targetRow;
    tempJoin.m_eop = euoChange;

    for (int i = 0; i < indexes.size(); ++i) {
        int row = indexes[i].row();
        letter += model->data(model->index(row, 0)).toString();
        left = my_min(left, model->data(model->index(row, 1)).toInt());
        bottom = my_max(bottom, model->data(model->index(row, 2)).toInt());
        right = my_max(right, model->data(model->index(row, 3)).toInt());
        top = my_min(top, model->data(model->index(row, 4)).toInt());
        page = my_min(page, model->data(model->index(row, 5)).toInt());
        italic = italic || model->data(model->index(row, 6)).toBool();
        bold = bold || model->data(model->index(row, 7)).toBool();
        underline = underline || model->data(model->index(row, 8)).toBool();

        tempJoin.m_vdata[0] = letter;
        tempJoin.m_vdata[1] = left;
        tempJoin.m_vdata[2] = bottom;
        tempJoin.m_vdata[3] = right;
        tempJoin.m_vdata[4] = top;
        tempJoin.m_vdata[5] = page;
        tempJoin.m_vdata[6] = italic;
        tempJoin.m_vdata[7] = bold;
        tempJoin.m_vdata[8] = underline;

        joinstack.push_front(tempJoin);
    }

    model->setData(model->index(targetRow, 0), letter);
    model->setData(model->index(targetRow, 1), left);
    model->setData(model->index(targetRow, 2), bottom);
    model->setData(model->index(targetRow, 3), right);
    model->setData(model->index(targetRow, 4), top);
    model->setData(model->index(targetRow, 5), page);
    model->setData(model->index(targetRow, 6), italic);
    model->setData(model->index(targetRow, 7), bold);
    model->setData(model->index(targetRow, 8), underline);
    updateModelItemBox(targetRow);

    selectionModel->clearSelection();

    int rowstodelete = indexes.size();
    int rownum = indexes.front().row();

    // Keep the first row with joined data
    rownum++;
    rowstodelete--;

    for (int i = rowstodelete; i > 0; i--) {
        m_undostack.push(joinstack.pop());
        deleteSymbolByRow(rownum);
    }

    table->setCurrentIndex(model->index(targetRow, 0));
    table->setFocus();
    updateSelectionRects();
    emit modifiedChanged();
}

void ChildWidget::deleteSymbolByRow(int row) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    UndoItem ui;
    ui.m_eop = euoDelete;
    ui.m_origrow = row;
    for (int j = 0; j < model->columnCount(); ++j)
        ui.m_vdata[j] = model->index(ui.m_origrow, j).data();
    m_undostack.push(ui);

    deleteModelItemBox(ui.m_origrow);
    model->removeRow(ui.m_origrow);
}

void ChildWidget::deleteSymbol() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = selectionModel->selectedRows();
    if (indexes.empty())
        return;
    // This prevents deselecting dead rows in selectionChanged() on removeRow()
    selectionModel->clearSelection();

    int rowstodelete = indexes.size();
    int rownum = indexes.front().row();
    int afterRow = my_min(indexes.back().row() - indexes.size() + 1,
                          model->rowCount() - 1);

    while (!indexes.empty())
    {
        deleteSymbolByRow(indexes.back().row());
        indexes.pop_back();
    }

    if (model->rowCount() != 0) {
        table->setCurrentIndex(model->index(afterRow, 0));
    }
    table->setFocus();
    updateSelectionRects();
    documentWasModified();
}

void ChildWidget::moveUp() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    moveSymbolRow(-1);
}

void ChildWidget::moveDown() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    moveSymbolRow(1);
}

void ChildWidget::moveTo() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (table->currentIndex().row() < 0)
        return;

    int sourceRow = table->currentIndex().row();
    int destRow = 0;
    GetRowIDDialog dialog(this);

    if (dialog.exec()) {
        QString string = dialog.lineEdit->text();

        if (string.toInt() == 0)
            destRow = string.toInt();
        else
            destRow = string.toInt() - 1;

        if (destRow > model->rowCount())
            destRow = model->rowCount() - 1;
    }

    moveSymbolRow(destRow - sourceRow);
    table->resizeRowToContents(destRow);
}

void ChildWidget::goToRow() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    GetRowIDDialog dialog(this);
    if (dialog.exec()) {
        QString string = dialog.lineEdit->text();
        int row;
        if (string.toInt() == 0)
            row = string.toInt();
        else
            row = string.toInt() - 1;

        if (row > model->rowCount())
            row = model->rowCount() - 1;

        table->setCurrentIndex(model->index(row, 0));
        table->setFocus();
        updateSelectionRects();
    }
}

void ChildWidget::find() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (!f_dialog) {
        f_dialog = new FindDialog(this, userFriendlyCurrentFile());
        connect(f_dialog, SIGNAL(findNext(const QString &,
                                          Qt::CaseSensitivity)),
                this, SLOT(findNext(const QString &,
                                    Qt::CaseSensitivity)));
        connect(f_dialog, SIGNAL(findPrev(const QString &,
                                          Qt::CaseSensitivity)),
                this, SLOT(findPrev(const QString &,
                                    Qt::CaseSensitivity)));
    }

    f_dialog->show();
    f_dialog->raise();
    f_dialog->activateWindow();
}

void ChildWidget::statistics()
{
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    qDebug() << Q_FUNC_INFO;
    if(!statisticsDialog){
        statisticsDialog = new StatisticsDialog(this);
    }
    qDebug() << this->boxFile;
    statisticsDialog->setBoxFile(this->boxFile);
    statisticsDialog->updateStats();
    statisticsDialog->show();
    statisticsDialog->raise();
    statisticsDialog->activateWindow();
}

QString ChildWidget::userFriendlyCurrentFile() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return strippedName(boxFile);
}

/* Get symbol string and convert it to hexadecimal codes */
QString ChildWidget::getSymbolHexCode() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.isValid()) {
        QString symbol = model->index(index.row(), 0).data().toString();
        QString result = "";
        for (int i = 0; i < symbol.size(); ++i) {
            QString str2 = QString::number(symbol[i].unicode(),
                                           16).toUpper().rightJustified(4, '0');
            result.append("0x" + str2 + " ");
        }
        return result;
    }
    return QString::null;
}

/* Get size of box */
QString ChildWidget::getBoxSize() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (index.isValid()) {
        int left = model->index(index.row(), 1).data().toInt();
        int bottom = model->index(index.row(), 2).data().toInt();
        int right = model->index(index.row(), 3).data().toInt();
        int top = model->index(index.row(), 4).data().toInt();
        QString result;
        result = QString("%1x%2").arg(right - left).arg(bottom - top);
        return result;
    }
    return QString::null;
}

QString ChildWidget::currentBoxFile() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return QFileInfo(boxFile).absoluteFilePath();
}

void ChildWidget::documentWasModified() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    modified = true;
    emit modifiedChanged();
}

void ChildWidget::emitBoxChanged() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    clearBalloons();
    // updateBalloons();
    emit boxChanged();
}

void ChildWidget::selectionChanged(const QItemSelection& /*selected*/,
                                   const QItemSelection& deselected) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // Set deselected bboxes' colors back to normal
    QModelIndexList indexes = deselected.indexes();
    if (!modelItemBox()) {
        // hide rectangle of last selected item
        if (!indexes.empty()) {
            QGraphicsRectItem* rectItem =
                    model->index(indexes[0].row(), 9).data().value<QGraphicsRectItem*>();
            rectItem->hide();
        }
        return;
    }
    for (int i = 0; i < indexes.size(); ++i)
        if (indexes[i].column() == 0) {
            modelItemBox(indexes[i].row())->setPen(QPen(boxColor));
            if (!boxesVisible)
                modelItemBox(indexes[i].row())->hide();
        }
    updateSelectionRects();

    emit boxChanged();
}

void ChildWidget::clearBalloons() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    for (int i = 0; i < balloons.size(); ++i) {
        imageScene->removeItem(balloons[i].symbol);
        delete balloons[i].symbol;
        for (int j = 0; j < BalloonSymbol::haloCompCount; ++j) {
            imageScene->removeItem(balloons[i].halo[j]);
            delete balloons[i].halo[j];
        }
    }
    balloons.clear();
}

void ChildWidget::updateBalloons() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int idx = table->selectionModel()->selectedRows().last().row();
    int min_idx = my_max(idx - balloonCount/2, 0);
    int max_idx = my_min(idx + balloonCount/2, model->rowCount() - 1);

    // calculate baseline for symbols based on selected symbol
    int baseline = 0;
    for (int i = min_idx; i <= max_idx; ++i) {
        if (baseline == 0) {
            baseline = model->index(i, 4).data().toInt();
        } else {
            baseline = my_min(baseline, model->index(i, 4).data().toInt());
        }
    }

    for (int i = min_idx; i <= max_idx; ++i) {
        balloons.push_back(BalloonSymbol());

        QString letter = model->index(i, 0).data().toString();
        int left = model->index(i, 1).data().toInt();
        int top = model->index(i, 4).data().toInt();
        // TODO(daemons2000): Bad when (i == 0)
        int botPrev = model->index(i-1, 2).data().toInt();
        if (top > botPrev) {
            baseline = top;  // new line? => problem with '", o'
        }

        balloons.back().symbol = imageScene->addText(letter, m_imageFont);
        QGraphicsTextItem* curSymbol = balloons.back().symbol;
        // TODO(zdenop): get font metrics and calculate better placement
        // (e.g. visible in case of narrow margin)
        curSymbol->setPos(QPoint(left, baseline - fontOffset));
        curSymbol->setDefaultTextColor(imageFontColor);
        curSymbol->setZValue(4);
        curSymbol->setVisible(true);

        for (int j = 0; j < BalloonSymbol::haloCompCount; ++j) {
            balloons.back().halo[j] = imageScene->addText(letter, m_imageFont);
            QGraphicsTextItem* curHalo = balloons.back().halo[j];
            curHalo->setDefaultTextColor(Qt::white);
            curHalo->setZValue(3);
        }
        balloons.back().halo[0]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(1, 0));
        balloons.back().halo[1]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(1, 1));
        balloons.back().halo[2]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(0, 1));
        balloons.back().halo[3]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(-1, 1));
        balloons.back().halo[4]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(-1, 0));
        balloons.back().halo[5]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(-1, -1));
        balloons.back().halo[6]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(0, -1));
        balloons.back().halo[7]->setPos(curSymbol->pos() +
                                        BalloonSymbol::haloShift*QPoint(1, -1));
        for (int j = 0; j < BalloonSymbol::haloCompCount; ++j)
            balloons.back().halo[j]->setVisible(true);
    }   // for i (idx)
}

void ChildWidget::updateSelectionRects() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if (!indexes.empty()) {
        clearBalloons();
        for (int i = 0; i < indexes.size(); ++i) {
            QGraphicsRectItem* rectItem = modelItemBox(indexes[i].row());
            if (rectItem) {
                rectItem->setPen(QPen(rectColor));
                rectItem->show();
            }
        }
        if (modelItemBox())
            imageView->ensureVisible(modelItemBox());
        if (symbolShown == true && indexes.size() == 1) {
            updateBalloons();
            resizer->setFromRect(modelItemBox()->rect().toRect());
        } else {
            resizer->setFromRect(modelItemBox()->rect().toRect());
        }
    } else {
        clearBalloons();
        resizer->disable();
    }
}

void ChildWidget::closeEvent(QCloseEvent* event) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (!maybeSave()) {
        event->ignore();
    }
    if (fileWatcher)
        delete fileWatcher;
    if (f_dialog)
        delete f_dialog;
    if(statisticsDialog)
        delete statisticsDialog;
}

bool ChildWidget::maybeSave() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, SETTING_APPLICATION,
                                   tr("'%1' has been modified.\n"
                                      "Do you want to save your changes?").arg(
                                       userFriendlyCurrentFile()), QMessageBox::Save
                                   | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save(boxFile);
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void ChildWidget::setCurrentImageFile(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    imageFile = QFileInfo(fileName).canonicalFilePath();
}

void ChildWidget::setCurrentBoxFile(const QString& fileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    boxFile = QFileInfo(fileName).canonicalFilePath();
    if (DMESS > 10) qDebug() << boxFile;
}

QString ChildWidget::strippedName(const QString& fullFileName) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return QFileInfo(fullFileName).fileName();
}

void ChildWidget::cbFontToggleProxy(bool checked, int column) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    switch (column) {
    case 6 :
        setItalic(checked);
        break;
    case 7 :
        setBolded(checked);
        break;
    case 8 :
        setUnderline(checked);
        break;
    default :
        break;
    }
}

void ChildWidget::letterStartEdit() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();
    int row = index.row();
    if (!bIsLineEditChanged) {
        // First event. Save undo
        UndoItem ui;
        ui.m_eop = euoChange;
        ui.m_origrow = row;

        for (int i = 0; i < model->columnCount(); i++)
            ui.m_vdata[i] = model->index(ui.m_origrow, i).data();

        m_undostack.push(ui);

        bIsLineEditChanged = true;
    }
}

void ChildWidget::letterEditFinished() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    bIsLineEditChanged = false;
}

void ChildWidget::sbValueChanged(int sbdValue) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();
    int row = index.row();

    int left = model->index(row, 1).data().toInt();
    int bottom = model->index(row, 2).data().toInt();
    int right = model->index(row, 3).data().toInt();
    int top = model->index(row, 4).data().toInt();

    switch (index.column()) {
    case 1:
        left = sbdValue;
        break;
    case 2:
        bottom = sbdValue;
        break;
    case 3:
        right = sbdValue;
        break;
    case 4:
        top = sbdValue;
        break;
    default :
        break;
    }

    if (!bIsSpinBoxChanged) {
        // First event. Save undo
        UndoItem ui;
        ui.m_eop = euoChange;
        ui.m_origrow = row;

        for (int i = 0; i < model->columnCount(); i++)
            ui.m_vdata[i] = model->index(ui.m_origrow, i).data();

        m_undostack.push(ui);

        bIsSpinBoxChanged = true;
    }

    modelItemBox(row)->setRect(QRectF(QPoint(left, top), QPointF(right, bottom)));

    imageView->ensureVisible(modelItemBox(row));
}

void ChildWidget::sbFinished() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    bIsSpinBoxChanged = false;
}

void ChildWidget::boxDragChanged() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    QModelIndex index = selectionModel->currentIndex();

    if (!index.isValid())
        return;

    int row = index.row();
    model->setData(model->index(row, 1, QModelIndex()), resizer->rect.left());
    model->setData(model->index(row, 2, QModelIndex()), resizer->rect.bottom());
    model->setData(model->index(row, 3, QModelIndex()), resizer->rect.right());
    model->setData(model->index(row, 4, QModelIndex()), resizer->rect.top());
    modelItemBox()->setRect(resizer->rect);
}

void ChildWidget::updateStats(const QModelIndex &index, int first, int last)
{

    //    for(int row=first; row<=last; ++row){
    //        QString currentChar =
    //                model->data(model->index(row-1, 0)).toString();
    //        if(!currentChar.isEmpty() && !currentChar.isNull()){
    //            insertOrUpdateCharStat(currentChar);
    //        }
    //    }
    //    // Update standard deviation and precentages
    //    updateSTD();
    //    statisticsModelProxy->sort(1, Qt::DescendingOrder);
}

void ChildWidget::updateStats(const QModelIndex &tl, const QModelIndex &rb)
{
    if(tl.column()==0) {
        statisticsModelProxy->removeRows(0,statisticsModel->rowCount());
        for(int row=0; row<=model->rowCount(); ++row){
            QString currentChar =
                    model->data(model->index(row, 0)).toString();
            qDebug() << currentChar << tl;
            if(!currentChar.isEmpty() && !currentChar.isNull()){
                insertOrUpdateCharStat(currentChar);
            }
        }
        // Update standard deviation and precentages
        updateSTD();
        statisticsModelProxy->sort(1, Qt::DescendingOrder);
    }
}

void ChildWidget::findNext(const QString &symbol, Qt::CaseSensitivity mc) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int row = table->currentIndex().row() + 1;
    while (row < model->rowCount()) {
        QString letter = model->index(row, 0).data().toString();
        if (letter.contains(symbol, mc)) {
            table->setCurrentIndex(model->index(row, 0));
            table->setFocus();
            updateSelectionRects();
            return;
        }
        ++row;
    }
    emit blinkFindDialog();
    emit statusBarMessage(tr("End of search!"));
}

void ChildWidget::findPrev(const QString &symbol,
                           Qt::CaseSensitivity mc) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int row = table->currentIndex().row() - 1;
    while (row >= 0) {
        QString letter = model->index(row, 0).data().toString();
        if (letter.contains(symbol, mc)) {
            table->setCurrentIndex(model->index(row, 0));
            table->setFocus();
            updateSelectionRects();
            return;
        }
        --row;
    }
    emit blinkFindDialog();
    emit statusBarMessage(tr("End of found!"));
}

bool ChildWidget::isUndoAvailable() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return m_undostack.isEmpty() ? false : true;
}

bool ChildWidget::isRedoAvailable() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    return m_redostack.isEmpty() ? false : true;
}

void ChildWidget::undo() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (m_undostack.isEmpty()) {
        emit boxChanged();  // update toolbar/menu to disable undo action
        // TODO(all): it is not working perfectly (are there some "ghost" undo
        // actions?). Maybe we need to implement UndoStackView similar to
        // http://doc-snapshot.qt-project.org/4.8/demos-undo.html
        return;
    }

    UndoItem ui = m_undostack.pop();

    switch (ui.m_eop) {
    case euoAdd:
        // Item was added. Reverse is remove it.
        undoDelete(ui);
        break;
    case euoDelete:
        // Item was deleted. Reverse is put it back.
        undoAdd(ui);
        break;
    case euoChange:
        // Item was edited. Put back old values.
        undoEdit(ui);
        break;
    case euoJoin:
        // Two items joined. Split back.
        undoSplit(ui);
        break;
    case euoSplit:
        // Item split in two. Join back.
        undoJoin(ui);
        break;
    case euoReplace:
        // Two item changed places. Change places back.
        undoMoveBack(ui);
        break;
    case euoMove:
        // Two item changed places. Change places back.
        undoMoveBack2(ui);
        break;
    default:
        // Nothing to do for other cases. Report error.

        QMessageBox::warning(
                    this,
                    SETTING_APPLICATION,
                    "Invalid undo operation.");
        break;
    }

    emit boxChanged();  // update toolbar/menu
}

// Delete item as undo operation of add
void ChildWidget::undoDelete(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    selectionModel->clearSelection();
    deleteModelItemBox(ui.m_origrow);
    model->removeRow(ui.m_origrow);

    int rows = model->rowCount();

    int newfocusrow = ui.m_origrow;

    if (newfocusrow > rows)
        newfocusrow = rows;

    table->setCurrentIndex(model->index(newfocusrow, 0));
    table->setFocus();
    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(ui, false);
    else
        m_redostack.push(ui);
}

// Add back item as undo operation of delete
void ChildWidget::undoAdd(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    model->insertRow(ui.m_origrow);
    undoEdit(ui, bIsRedo);
}

// Put back edited values
void ChildWidget::undoEdit(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (ui.m_eop == euoChange) {
        if (bIsRedo) {
            for (int i = 0; i < model->columnCount() - 1; i++)
                model->setData(model->index(ui.m_origrow, i), ui.m_vextradata[i]);
        } else {
            // Save for redo
            for (int ii = 0; ii < model->columnCount() - 1; ii++)
                ui.m_vextradata[ii] = model->index(ui.m_origrow, ii).data();

            for (int i = 0; i < model->columnCount() -1 ; i++)
                model->setData(model->index(ui.m_origrow, i), ui.m_vdata[i]);
        }
    } else {
        for (int i = 0; i < model->columnCount() - 1; i++)
            model->setData(model->index(ui.m_origrow, i), ui.m_vdata[i]);
    }

    if (ui.m_eop == euoDelete || (bIsRedo && ui.m_eop == euoAdd))
        createModelItemBox(ui.m_origrow);
    else
        updateModelItemBox(ui.m_origrow);

    table->setCurrentIndex(model->index(ui.m_origrow, 0));
    table->setFocus();
    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(ui, false);
    else
        m_redostack.push(ui);
}

// Re-join split rows
void ChildWidget::undoJoin(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // Save data for redo
    UndoItem rui;
    rui.m_eop = euoJoin;
    rui.m_origrow = ui.m_origrow;
    rui.m_extrarow = ui.m_extrarow;

    for (int i = 0; i < model->columnCount(); i++) {
        rui.m_vdata[i] = model->index(rui.m_origrow, i).data();
        rui.m_vextradata[i] = model->index(rui.m_extrarow, i).data();
    }

    deleteModelItemBox(ui.m_extrarow);
    model->removeRow(ui.m_extrarow);

    for (int i = 0; i < model->columnCount(); i++)
        model->setData(model->index(ui.m_origrow, i), ui.m_vdata[i]);
    updateModelItemBox(ui.m_origrow);

    table->setCurrentIndex(model->index(ui.m_origrow, 0));
    table->setFocus();
    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(rui, false);
    else
        m_redostack.push(rui);
}

// Split back joined lines
void ChildWidget::undoSplit(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    // Save data for redo
    UndoItem rui;
    rui.m_eop = euoSplit;
    rui.m_origrow = ui.m_origrow;
    rui.m_extrarow = ui.m_origrow + 1;

    for (int i = 0; i < model->columnCount(); i++) {
        rui.m_vdata[i] = model->index(rui.m_origrow, i).data();
    }

    model->insertRow(ui.m_extrarow);
    for (int i = 0; i < model->columnCount(); i++) {
        model->setData(model->index(ui.m_extrarow, i), ui.m_vextradata[i]);
        model->setData(model->index(ui.m_origrow, i), ui.m_vdata[i]);
    }
    updateModelItemBox(ui.m_origrow);
    createModelItemBox(ui.m_extrarow);

    table->setCurrentIndex(model->index(ui.m_origrow, 0));
    table->setFocus();
    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(rui, false);
    else
        m_redostack.push(rui);
}

// Put replaced rows back to original location
void ChildWidget::undoMoveBack(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int firstrow = ui.m_origrow;
    int secondrow = ui.m_extrarow;

    if (bIsRedo) {
        firstrow = ui.m_extrarow;
        secondrow = ui.m_origrow;
    }

    for (int i = 0; i < model->columnCount() - 1; i++) {
        model->setData(model->index(firstrow, i), ui.m_vdata[i]);
        model->setData(model->index(secondrow, i), ui.m_vextradata[i]);
    }
    updateModelItemBox(ui.m_origrow);
    updateModelItemBox(ui.m_extrarow);

    table->setCurrentIndex(model->index(firstrow, 0));
    table->setFocus();

    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(ui, false);
    else
        m_redostack.push(ui);
}

// Put moved row back to original location
void ChildWidget::undoMoveBack2(UndoItem& ui, bool bIsRedo) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    int firstrow = ui.m_origrow;
    int secondrow = ui.m_extrarow;

    if (bIsRedo) {
        firstrow = ui.m_extrarow;
        secondrow = ui.m_origrow;
    }

    if (firstrow < secondrow)
        secondrow++;
    else
        firstrow++;

    model->insertRow(firstrow);

    for (int i = 0; i < model->columnCount() - 1; i++) {
        model->setData(model->index(firstrow, i), ui.m_vdata[i]);
    }
    createModelItemBox(firstrow);

    model->removeRow(secondrow);

    if (firstrow > secondrow) firstrow--;
    table->setCurrentIndex(model->index(firstrow, 0));
    table->setFocus();

    updateSelectionRects();

    if (bIsRedo)
        m_undostack.push(ui, false);
    else
        m_redostack.push(ui);
}

void ChildWidget::updateSTD()
{
    u_int32_t totalCount = model->rowCount();
    for(int stat_row=0; stat_row < statisticsModel->rowCount(); ++stat_row){
        float c = statisticsModel->data(statisticsModel->index(stat_row,1)).toFloat();
        float precentage = 100.0* c /static_cast<float>(totalCount);
        statisticsModel->setData(statisticsModel->index(stat_row,2),precentage,Qt::EditRole);
    }
}

void ChildWidget::insertOrUpdateCharStat(const QString &currentChar)
{
    bool found = false;
    for(int stat_row=0; stat_row < statisticsModel->rowCount(); ++stat_row){
        QString statData = statisticsModel->data(statisticsModel->index(stat_row,0)).toString();
        if(statData == currentChar){
            int oldCount = statisticsModel->data(statisticsModel->index(stat_row,1)).toInt();
            statisticsModel->setData(statisticsModel->index(stat_row,1),oldCount+1,Qt::EditRole);
            found = true;
            break;
        }
    }
    if(!found) {
        QList<QStandardItem*> items;
        QStandardItem* ch = new QStandardItem();
        QStandardItem* c = new QStandardItem();
        QStandardItem* p = new QStandardItem();
        QStandardItem* s = new QStandardItem();
        ch->setData(currentChar, Qt::EditRole);
        ch->setData(currentChar, Qt::DisplayRole);
        c->setData(1, Qt::EditRole);
        p->setData(0.0, Qt::EditRole);
        s->setData(0.0, Qt::EditRole);
        items << ch;
        items << c;
        items << p;
        items << s;
        statisticsModel->appendRow(items);
    }
}

void ChildWidget::removeCharStat(const QString &currentChar)
{
    for(int stat_row=0; stat_row < statisticsModel->rowCount(); ++stat_row){
        QString statData = statisticsModel->data(statisticsModel->index(stat_row,0)).toString();
        if(statData == currentChar){
            int oldCount = statisticsModel->data(statisticsModel->index(stat_row,1)).toInt();
            if(oldCount==1){
                statisticsModel->removeRow(stat_row);
            } else{
                statisticsModel->setData(statisticsModel->index(stat_row,1),oldCount-1,Qt::EditRole);
            }
            break;
        }
    }
}

void ChildWidget::redo() {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    if (m_redostack.isEmpty()) {
        emit boxChanged();  // update toolbar/menu to disable undo action
        // TODO(all): it is not working perfectly (are there some "ghost" undo
        // actions?). Maybe we need to implement UndoStackView similar to
        // http://doc-snapshot.qt-project.org/4.8/demos-undo.html
        return;
    }

    UndoItem ui = m_redostack.pop();

    switch (ui.m_eop) {
    case euoAdd:
        // Item was added. Reverse is remove it. Redo: Add it again, with values
        undoAdd(ui, true);
        break;
    case euoDelete:
        // Item was deleted. Reverse is put it back. Redo: delete it again
        undoDelete(ui, true);
        break;
    case euoChange:
        // Item was edited. Put back old values.
        undoEdit(ui, true);
        break;
    case euoJoin:
        // Two items joined. Split back.
        undoSplit(ui, true);
        break;
    case euoSplit:
        // Item split in two. Join back.
        undoJoin(ui, true);
        break;
    case euoReplace:
        // Two item changed places. Change places back.
        undoMoveBack(ui, true);
        break;
    case euoMove:
        // Two item changed places. Change places back.
        undoMoveBack2(ui, true);
        break;
    default:
        // Nothing to do for other cases. Report error.

        QMessageBox::warning(
                    this,
                    SETTING_APPLICATION,
                    "Invalid redo operation.");
        break;
    }

    emit boxChanged();
}

bool ChildWidget::slotChangePage(int sbdPage) {
    if (DMESS > 10) qDebug() << Q_FUNC_INFO;
    PIX * pix;
    QImage image;
    storePage();
    currPage = sbdPage - 1;

    pix = pixReadTiff(imageFile.toLocal8Bit().data(), currPage);
    image = TessTools::PIX2qImage(pix);
    pixDestroy(&pix);
    if (image.isNull()) {
        QMessageBox::information(this, tr("Problem"),
                                 tr("Cannot load page %1 from file %1.")
                                 .arg(currPage).arg(imageFile));
        return false;
    }
    imageHeight = image.height();
    imageWidth = image.width();
    imageScene->removeItem(imageItem);
    imageItem = imageScene->addPixmap(QPixmap::fromImage(image));

    bool showFontColumns = isFontColumnsShown();
    cleanTable();
    initTable();
    setShowFontColumns(showFontColumns);
    if (fillTableData(currPage)) {
        table->setCurrentIndex(model->index(0, 0));
        updateSelectionRects();
    } else {
        return false;
    }
    return true;
}

/*
 * Store current page (in table view) to pages vector
 *
 */
void ChildWidget::storePage() {
    QModelIndex index = selectionModel->currentIndex();
    if (!index.isValid())
        return;

    QVector<QStringList> page;
    for (int row = 0; row < model->rowCount(); ++row) {
        QString letter = model->index(row, 0).data().toString();
        QString left = model->index(row, 1).data().toString();
        int bottom = model->index(row, 2).data().toInt();
        QString right = model->index(row, 3).data().toString();
        int top = model->index(row, 4).data().toInt();
        QString pageNum = model->index(row, 5).data().toString();
        bool italic = model->index(row, 6).data().toBool();
        bool bold = model->index(row, 7).data().toBool();
        bool underline = model->index(row, 8).data().toBool();
        if (underline)
            letter.prepend("\'");
        if (italic)
            letter.prepend("$");
        if (bold)
            letter.prepend("@");
        QStringList box;
        box << letter << left << QString::number(imageHeight - bottom)
            << right  << QString::number(imageHeight - top) << pageNum;
        page.append(box);
    }
    pages[currPage] = page;
}

void ChildWidget::cleanTable() {
    // Hide current selection - it is not valid on other page
    QModelIndexList indexes = table->selectionModel()->selectedRows();
    if (!indexes.empty()) {
        clearBalloons();
        for (int i = 0; i < indexes.size(); ++i) {
            QGraphicsRectItem* rectItem = modelItemBox(indexes[i].row());
            if (rectItem) {
                rectItem->hide();
            }
        }
    }

    selectionModel->clearSelection();
    model->clear();
    delete selectionModel;
    delete model;
}
