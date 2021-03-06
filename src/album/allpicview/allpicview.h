#ifndef ALLPICVIEW_H
#define ALLPICVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"
#include "widgets/statusbar.h"

#include <QWidget>
#include <QVBoxLayout>
#include <DLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QImageReader>
#include <DPushButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <DStackedWidget>
#include <DSlider>
#include <DSpinner>

DWIDGET_USE_NAMESPACE

class AllPicView : public DWidget, public ImageEngineImportObject
{
    Q_OBJECT

public:
    AllPicView();

    bool imageImported(bool success) override
    {
        Q_UNUSED(success);
        emit dApp->signalM->closeWaitDialog();
        return true;
    }
    void restorePicNum();
    void updatePicNum();
    const ThumbnailListView *getAllPicThumbnailListViewModel();

private:
    void initConnections();
    void initStackedWidget();
    void updatePicsIntoThumbnailView();
    void onUpdateAllpicsNumLabel();
    void onKeyDelete();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

public slots:
    void updateStackedWidget();
    // 监控到新文件
    void monitorHaveNewFile(QStringList list);

private slots:
    void updatePicsIntoThumbnailViewWithCache();
    void onFinishLoad();
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, QString albumName);
    void onOpenImage(int index);
    void onMenuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow);
    void onImportViewImportBtnClicked();
    void onImportFailedToView();

public:
    DStackedWidget *m_pStackedWidget;
    StatusBar *m_pStatusBar;
    QWidget *m_pwidget;
    ImportView *m_pImportView;
    int step;

private:
    ThumbnailListView *m_pThumbnailListView;
    SearchView *m_pSearchView;
    DSpinner *m_spinner;
    DWidget *fatherwidget;

public:
    ThumbnailListView *getThumbnailListView();
    /**
     * @brief updatePicsIntoThumbnailView 针对单个或多个图片刷新
     * @param strpath 图片路径队列
     */
    void updatePicsThumbnailView(QStringList strpath = QStringList());
};

#endif // ALLPICVIEW_H
