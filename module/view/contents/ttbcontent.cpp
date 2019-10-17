/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "ttbcontent.h"
#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"

#include "widgets/pushbutton.h"
#include "widgets/returnbutton.h"
#include "controller/dbmanager.h"
#include "controller/configsetter.h"
#include "widgets/elidedlabel.h"
#include "controller/signalmanager.h"

#include <QTimer>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QDebug>
#include <DLabel>
#include <QAbstractItemModel>
#include <DImageButton>
#include <DThumbnailProvider>

DWIDGET_USE_NAMESPACE
namespace {
const int LEFT_MARGIN = 10;
const QSize ICON_SIZE = QSize(50, 50);
const QString FAVORITES_ALBUM = "My favorite";
const int ICON_SPACING = 3;
const int RETURN_BTN_MAX = 200;
const int FILENAME_MAX_LENGTH = 600;
const int RIGHT_TITLEBAR_WIDTH = 100;
const int LEFT_SPACE = 20;

const unsigned int IMAGE_TYPE_JEPG = 0xFFD8FF;
const unsigned int IMAGE_TYPE_JPG1 = 0xFFD8FFE0;
const unsigned int IMAGE_TYPE_JPG2 = 0xFFD8FFE1;
const unsigned int IMAGE_TYPE_JPG3 = 0xFFD8FFE8;
const unsigned int IMAGE_TYPE_PNG = 0x89504e47;
const unsigned int IMAGE_TYPE_GIF = 0x47494638;
const unsigned int IMAGE_TYPE_TIFF = 0x49492a00;
const unsigned int IMAGE_TYPE_BMP = 0x424d;
}  // namespace

char* getImageType(QString filepath){
    char *ret=nullptr;
    QFile file(filepath);
     file.open(QIODevice::ReadOnly);
     QDataStream in(&file);

     // Read and check the header
     quint32 magic;
     in >> magic;
     switch (magic) {
     case IMAGE_TYPE_JEPG:
     case IMAGE_TYPE_JPG1:
     case IMAGE_TYPE_JPG2:
     case IMAGE_TYPE_JPG3:
         //文件类型为 JEPG
         ret ="JEPG";
         break;
     case IMAGE_TYPE_PNG:
         //文件类型为 png
         ret ="PNG";
         break;
     case IMAGE_TYPE_GIF:
         //文件类型为 GIF
         ret ="GIF";
         break;
     case IMAGE_TYPE_TIFF:
         //文件类型为 TIFF
         ret ="TIFF";
         break;
     case IMAGE_TYPE_BMP:
         //文件类型为 BMP
         ret ="BMP";
         break;
     default:
         ret =nullptr;
         break;
     }
     return ret;
};
TTBContent::TTBContent(bool inDB,
                       DBImgInfoList m_infos ,
                       QWidget *parent) : QLabel(parent)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    m_windowWidth = std::max(this->window()->width(),
        ConfigSetter::instance()->value("MAINWINDOW", "WindowWidth").toInt());
//    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH, 1);
    m_imgInfos = m_infos;
    if ( m_imgInfos.size() <= 1 ) {
        m_contentWidth = 420;
    } else {
        m_contentWidth = 1280;
    }

    m_bClBTChecked = false;

    setFixedWidth(m_contentWidth);
    setFixedHeight(70);
    QHBoxLayout *hb = new QHBoxLayout(this);
    hb->setContentsMargins(LEFT_MARGIN, 0, LEFT_MARGIN, 0);
    hb->setSpacing(0);
    m_inDB = inDB;
#ifndef LITE_DIV
    m_returnBtn = new ReturnButton();
    m_returnBtn->setMaxWidth(RETURN_BTN_MAX);
    m_returnBtn->setMaximumWidth(RETURN_BTN_MAX);
    m_returnBtn->setObjectName("ReturnBtn");
    m_returnBtn->setToolTip(tr("Back"));

    m_folderBtn = new PushButton();
    m_folderBtn->setFixedSize(QSize(24, 24));
    m_folderBtn->setObjectName("FolderBtn");
    m_folderBtn->setToolTip(tr("Image management"));
    if(m_inDB) {
        hb->addWidget(m_returnBtn);
    } else {
       hb->addWidget(m_folderBtn);
    }
    hb->addSpacing(20);

    connect(m_returnBtn, &ReturnButton::clicked, this, [=] {
        emit clicked();
    });
    connect(m_folderBtn, &PushButton::clicked, this, [=] {
        emit clicked();
    });
    connect(m_returnBtn, &ReturnButton::returnBtnWidthChanged, this, [=]{
        updateFilenameLayout();
    });
#endif
    // Adapt buttons////////////////////////////////////////////////////////////
     m_backButton = new DImageButton();
     m_backButton->setFixedSize(ICON_SIZE);
     m_backButton->setObjectName("ReturnBtn");
     m_backButton->setToolTip(tr("Return"));
     hb->addWidget(m_backButton);
     hb->addSpacing(ICON_SPACING*2);
     connect(m_backButton, &DImageButton::clicked, this, [=] {
         emit dApp->signalM->hideImageView();
         emit ttbcontentClicked();
     });

     m_preButton = new DImageButton();
     m_nextButton = new DImageButton();
     m_preButton->setObjectName("PreviousButton");
     m_nextButton->setObjectName("NextButton");

     m_preButton->setFixedSize(ICON_SIZE);
     m_nextButton->setFixedSize(ICON_SIZE);
     m_preButton->setToolTip(tr("Previous"));
     m_nextButton->setToolTip(tr("Next"));
     m_preButton->hide();
     m_nextButton->hide();
     hb->addWidget(m_preButton);
     hb->addSpacing(ICON_SPACING);
     hb->addWidget(m_nextButton);
     hb->addSpacing(ICON_SPACING);
     connect(m_preButton, &DImageButton::clicked, this, [=] {
         emit showPrevious();
         emit ttbcontentClicked();
     });
     connect(m_nextButton, &DImageButton::clicked, this, [=] {
         emit showNext();
         emit ttbcontentClicked();
     });

    m_adaptImageBtn = new DImageButton();
    m_adaptImageBtn->setObjectName("AdaptBtn");
    m_adaptImageBtn->setFixedSize(ICON_SIZE);

    m_adaptImageBtn->setToolTip(tr("1:1 Size"));
    hb->addWidget(m_adaptImageBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptImageBtn, &DImageButton::clicked, this, [=] {
        emit resetTransform(false);
        emit ttbcontentClicked();
    });

    m_adaptScreenBtn = new DImageButton();
    m_adaptScreenBtn->setFixedSize(ICON_SIZE);
    m_adaptScreenBtn->setObjectName("AdaptScreenBtn");
    m_adaptScreenBtn->setToolTip(tr("Fit to window"));
    hb->addWidget(m_adaptScreenBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_adaptScreenBtn, &DImageButton::clicked, this, [=] {
        emit resetTransform(true);
        emit ttbcontentClicked();
    });

    // Collection button////////////////////////////////////////////////////////
    m_clBT = new DImageButton();
    m_clBT->setFixedSize(ICON_SIZE);
    m_clBT->setObjectName("CollectBtn");

    connect(m_clBT, &DImageButton::clicked, this, [=] {

        if (true == m_bClBTChecked)
        {
            DBManager::instance()->removeFromAlbum(FAVORITES_ALBUM, QStringList(m_imagePath));
        }
        else
        {
            DBManager::instance()->insertIntoAlbum(FAVORITES_ALBUM, QStringList(m_imagePath));
        }

        emit ttbcontentClicked();
    });

    hb->addWidget(m_clBT);
    hb->addSpacing(ICON_SPACING);


    m_rotateLBtn = new DImageButton();
    m_rotateLBtn->setFixedSize(ICON_SIZE);
    m_rotateLBtn->setObjectName("RotateBtn");
    m_rotateLBtn->setToolTip(tr("Rotate counterclockwise"));
    hb->addWidget(m_rotateLBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_rotateLBtn, &DImageButton::clicked, this, [=] {
        emit rotateCounterClockwise();
        emit ttbcontentClicked();
    });

    m_rotateRBtn = new DImageButton();
    m_rotateRBtn->setFixedSize(ICON_SIZE);
    m_rotateRBtn->setObjectName("RotateCounterBtn");
    m_rotateRBtn->setToolTip(tr("Rotate clockwise"));
    hb->addWidget(m_rotateRBtn);
    hb->addSpacing(ICON_SPACING);
    connect(m_rotateRBtn, &DImageButton::clicked, this, [=] {
        emit rotateClockwise();
        emit ttbcontentClicked();
    });

    m_imgListView = new DWidget();
    m_imgListView->setObjectName("ImgListView");
    m_imgList = new DWidget(m_imgListView);
    m_imgList->setFixedSize((m_imgInfos.size()+1)*32,60);
    m_imgList->setObjectName("ImgList");

    m_imgList->setDisabled(false);
    m_imgList->setHidden(true);
    m_imglayout= new QHBoxLayout();
    m_imglayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_imglayout->setMargin(0);
    m_imglayout->setSpacing(0);
    m_imgList->setLayout(m_imglayout);
    m_imgListView->setFixedSize(QSize(786,60));
    hb->addWidget(m_imgListView);

    m_trashBtn = new DImageButton();
    m_trashBtn->setFixedSize(ICON_SIZE);
    m_trashBtn->setObjectName("TrashBtn");
    m_trashBtn->setToolTip(tr("Delete"));
    hb->addWidget(m_trashBtn);

    m_fileNameLabel = new ElidedLabel();
//    hb->addWidget(m_fileNameLabel);
    connect(m_trashBtn, &DImageButton::clicked, this, [=] {
        emit removed();
        emit ttbcontentClicked();
    });

    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
            &TTBContent::onThemeChanged);
    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged,
            this, &TTBContent::onThemeChanged);
//     connect(dApp->signalM, &SignalManager::updateTopToolbar, this, [=]{
//         updateFilenameLayout();
//     });
    if (dApp->viewerTheme->getCurrentTheme() == ViewerThemeManager::Dark) {
        m_backButton->setNormalPic(":/resources/dark/images/previous_normal.svg");
        m_backButton->setHoverPic(":/resources/dark/images/previous_hover.svg");
        m_backButton->setPressPic(":/resources/dark/images/previous_press.svg");
        m_backButton->setCheckedPic(":/resources/dark/images/previous_press.svg");

        m_preButton->setNormalPic(":/resources/dark/images/previous_normal.svg");
        m_preButton->setHoverPic(":/resources/dark/images/previous_hover.svg");
        m_preButton->setPressPic(":/resources/dark/images/previous_press.svg");
        m_preButton->setCheckedPic(":/resources/dark/images/previous_press.svg");

        m_nextButton->setNormalPic(":/resources/dark/images/next_normal.svg");
        m_nextButton->setHoverPic(":/resources/dark/images/next_hover.svg");
        m_nextButton->setPressPic(":/resources/dark/images/next_press.svg");
        m_nextButton->setCheckedPic(":/resources/dark/images/next_press.svg");

        m_adaptImageBtn->setNormalPic(":/resources/dark/images/adapt_image_normal.svg");
        m_adaptImageBtn->setHoverPic(":/resources/dark/images/adapt_image_hover.svg");
        m_adaptImageBtn->setPressPic(":/resources/dark/images/adapt_image_active.svg");
        m_adaptImageBtn->setCheckedPic(":/resources/dark/images/adapt_image_active.svg");

        m_adaptScreenBtn->setNormalPic(":/resources/dark/images/adapt_screen_normal.svg");
        m_adaptScreenBtn->setHoverPic(":/resources/dark/images/adapt_screen_hover.svg");
        m_adaptScreenBtn->setPressPic(":/resources/dark/images/adapt_screen_active.svg");
        m_adaptScreenBtn->setCheckedPic(":/resources/dark/images/adapt_screen_active.svg");

        m_clBT->setNormalPic(":/resources/dark/images/collect_normal.svg");
        m_clBT->setHoverPic(":/resources/dark/images/collect_hover.svg");
        m_clBT->setPressPic(":/resources/dark/images/collect_active.svg");
        m_clBT->setCheckedPic(":/resources/dark/images/collect_active.svg");

        m_rotateLBtn->setNormalPic(":/resources/dark/images/clockwise_rotation_normal.svg");
        m_rotateLBtn->setHoverPic(":/resources/dark/images/clockwise_rotation_hover.svg");
        m_rotateLBtn->setPressPic(":/resources/dark/images/clockwise_rotation_press.svg");
        m_rotateLBtn->setCheckedPic(":/resources/dark/images/clockwise_rotation_press.svg");

        m_rotateRBtn->setNormalPic(":/resources/dark/images/contrarotate_normal.svg");
        m_rotateRBtn->setHoverPic(":/resources/dark/images/contrarotate_hover.svg");
        m_rotateRBtn->setPressPic(":/resources/dark/images/contrarotate_press.svg");
        m_rotateRBtn->setCheckedPic(":/resources/dark/images/contrarotate_press.svg");

        m_trashBtn->setNormalPic(":/resources/dark/images/delete_normal.svg");
        m_trashBtn->setHoverPic(":/resources/dark/images/delete_hover.svg");
        m_trashBtn->setPressPic(":/resources/dark/images/delete_active.svg");
        m_trashBtn->setCheckedPic(":/resources/dark/images/delete_active.svg");

    } else {
        m_backButton->setNormalPic(":/resources/light/images/previous_normal.svg");
        m_backButton->setHoverPic(":/resources/light/images/previous_hover.svg");
        m_backButton->setPressPic(":/resources/light/images/previous_press.svg");
        m_backButton->setCheckedPic(":/resources/light/images/previous_press.svg");

        m_preButton->setNormalPic(":/resources/light/images/previous_normal.svg");
        m_preButton->setHoverPic(":/resources/light/images/previous_hover.svg");
        m_preButton->setPressPic(":/resources/light/images/previous_press.svg");
        m_preButton->setCheckedPic(":/resources/light/images/previous_press.svg");

        m_nextButton->setNormalPic(":/resources/light/images/next_normal.svg");
        m_nextButton->setHoverPic(":/resources/light/images/next_hover.svg");
        m_nextButton->setPressPic(":/resources/light/images/next_press.svg");
        m_nextButton->setCheckedPic(":/resources/light/images/next_press.svg");

        m_adaptImageBtn->setNormalPic(":/resources/light/images/adapt_image_normal.svg");
        m_adaptImageBtn->setHoverPic(":/resources/light/images/adapt_image_hover.svg");
        m_adaptImageBtn->setPressPic(":/resources/light/images/adapt_image_active.svg");
        m_adaptImageBtn->setCheckedPic(":/resources/light/images/adapt_image_active.svg");

        m_adaptScreenBtn->setNormalPic(":/resources/light/images/adapt_screen_normal.svg");
        m_adaptScreenBtn->setHoverPic(":/resources/light/images/adapt_screen_hover.svg");
        m_adaptScreenBtn->setPressPic(":/resources/light/images/adapt_screen_active.svg");
        m_adaptScreenBtn->setCheckedPic(":/resources/light/images/adapt_screen_active.svg");

        m_clBT->setNormalPic(":/resources/images/photo preview/normal/collection_normal.svg");
        m_clBT->setHoverPic(":/resources/images/photo preview/hover/collection_hover.svg");
        m_clBT->setPressPic(":/resources/images/photo preview/press/collection_press.svg");
        m_clBT->setCheckedPic(":/resources/images/photo preview/checked/collection_checked.svg");

        m_rotateLBtn->setNormalPic(":/resources/light/images/clockwise_rotation_normal.svg");
        m_rotateLBtn->setHoverPic(":/resources/light/images/clockwise_rotation_hover.svg");
        m_rotateLBtn->setPressPic(":/resources/light/images/clockwise_rotation_press.svg");
        m_rotateLBtn->setCheckedPic(":/resources/light/images/clockwise_rotation_press.svg");

        m_rotateRBtn->setNormalPic(":/resources/light/images/contrarotate_normal.svg");
        m_rotateRBtn->setHoverPic(":/resources/light/images/contrarotate_hover.svg");
        m_rotateRBtn->setPressPic(":/resources/light/images/contrarotate_press.svg");
        m_rotateRBtn->setCheckedPic(":/resources/light/images/contrarotate_press.svg");

        m_trashBtn->setNormalPic(":/resources/light/images/delete_normal.svg");
        m_trashBtn->setHoverPic(":/resources/light/images/delete_hover.svg");
        m_trashBtn->setPressPic(":/resources/light/images/delete_active.svg");
        m_trashBtn->setCheckedPic(":/resources/light/images/delete_active.svg");
    }
}

void TTBContent::updateFilenameLayout()
{
    using namespace utils::base;
    QFont font;
    font.setPixelSize(12);
    m_fileNameLabel->setFont(font);
    QFontMetrics fm(font);
    QString filename = QFileInfo(m_imagePath).fileName();
    QString name;

    int strWidth = fm.boundingRect(filename).width();
    int leftMargin = 0;
    int m_leftContentWidth = 0;
#ifndef LITE_DIV
    if (m_inDB)
        m_leftContentWidth = m_returnBtn->buttonWidth() + 6
                + (ICON_SIZE.width()+2)*6 + LEFT_SPACE;
    else
    {
        m_leftContentWidth = m_folderBtn->width()  + 8
                + (ICON_SIZE.width()+2)*5 + LEFT_SPACE;
    }
#else
    // 39 为logo以及它的左右margin
    m_leftContentWidth = 5 + (ICON_SIZE.width() + 2) * 5 + 39;
#endif

    int ww = dApp->setter->value("MAINWINDOW",  "WindowWidth").toInt();
    m_windowWidth =  std::max(std::max(this->window()->geometry().width(), this->width()), ww);
    m_contentWidth = std::max(m_windowWidth - RIGHT_TITLEBAR_WIDTH + 2, 1);
    setFixedWidth(m_contentWidth);
    m_contentWidth = this->width() - m_leftContentWidth;

    if (strWidth > m_contentWidth || strWidth > FILENAME_MAX_LENGTH)
    {
        name = fm.elidedText(filename, Qt::ElideMiddle, std::min(m_contentWidth - 32,
                                                                 FILENAME_MAX_LENGTH));
        strWidth = fm.boundingRect(name).width();
        leftMargin = std::max(0, (m_windowWidth - strWidth)/2
                              - m_leftContentWidth - LEFT_MARGIN - 2);
    } else {
        leftMargin = std::max(0, (m_windowWidth - strWidth)/2
                              - m_leftContentWidth - 6);
        name = filename;
    }

    m_fileNameLabel->setText(name, leftMargin);
}

void TTBContent::onThemeChanged(ViewerThemeManager::AppTheme theme) {
    if (theme == ViewerThemeManager::Dark) {
        this->setStyleSheet(utils::base::getFileContent(
                                ":/resources/dark/qss/ttl.qss"));
    } else {
        this->setStyleSheet(utils::base::getFileContent(
                                ":/resources/light/qss/ttl.qss"));
    }
}

void TTBContent::setCurrentDir(QString text) {
    if (text == FAVORITES_ALBUM) {
        text = tr("My favorite");
    }

#ifndef LITE_DIV
    m_returnBtn->setText(text);
#endif
}

void TTBContent::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_windowWidth =  this->window()->geometry().width();
    QList<ImageItem*> labelList = m_imgList->findChildren<ImageItem*>();
    for(int j = 0; j < labelList.size(); j++){
        labelList.at(j)->setFixedSize (QSize(30,40));
        labelList.at(j)->resize (QSize(30,40));
        labelList.at(j)->setContentsMargins(1,5,1,5);
        labelList.at(j)->setFrameShape (QFrame::NoFrame);
        labelList.at(j)->setStyleSheet("border-width: 0px;border-style: solid;border-color: #2ca7f8;");
        labelList.at(j)->setIndexNow(m_nowIndex);
    }
    if(labelList.size()>0){
        labelList.at(m_nowIndex)->setFixedSize (QSize(60,58));
        labelList.at(m_nowIndex)->resize (QSize(60,58));
        labelList.at(m_nowIndex)->setFrameShape (QFrame::Box);
        labelList.at(m_nowIndex)->setContentsMargins(0,0,0,0);
        labelList.at(m_nowIndex)->setStyleSheet("border-width: 4px;border-style: solid;border-color: #2ca7f8;");
    }
//    m_contentWidth = std::max(m_windowWidth - 100, 1);
//    m_contentWidth = 310;
}

void TTBContent::setImage(const QString &path,DBImgInfoList infos)
{
    if (infos.size()!=m_imgInfos.size()){
        m_imgInfos.clear();
        m_imgInfos = infos;

        QLayoutItem *child;
         while ((child = m_imglayout->takeAt(0)) != 0)
         {
             m_imglayout->removeWidget(child->widget());
             child->widget()->setParent(0);
             delete child;

         }
    }
    if (path.isEmpty() || !QFileInfo(path).exists()
            || !QFileInfo(path).isReadable()) {
        m_adaptImageBtn->setDisabled(true);
        m_adaptScreenBtn->setDisabled(true);
        m_rotateLBtn->setDisabled(true);
        m_rotateRBtn->setDisabled(true);
        m_trashBtn->setDisabled(true);
        m_imgList->setDisabled(false);
    } else {
        m_adaptImageBtn->setDisabled(false);
        m_adaptScreenBtn->setDisabled(false);


        int t=0;
        if ( m_imgInfos.size() > 1 ) {
            m_imgList->setFixedSize((m_imgInfos.size()+1)*32,60);
            m_imgList->resize((m_imgInfos.size()+1)*32,60);

            m_imgList->setContentsMargins(0,0,0,0);

            auto num=30;
//            QHBoxLayout *layout= new QHBoxLayout();
//            layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//            layout->setMargin(0);
//            layout->setSpacing(0);

            int i=0;
            QList<ImageItem*> labelList = m_imgList->findChildren<ImageItem*>();

            for (DBImgInfo info : m_imgInfos) {
                if(labelList.size()!=m_imgInfos.size()){
                    ImageItem *imageItem = new ImageItem(i);
                    char *imageType=getImageType(info.filePath);
                    QImage image(info.filePath,imageType);
//                    imageItem->setPixmap(QPixmap(info.filePath).scaled(60,50));
                    imageItem->setPixmap(QPixmap::fromImage(image.scaled(60,50)));
                    imageItem->setContentsMargins(1,5,1,5);
                    imageItem->setFixedSize(QSize(num,40));
                    imageItem->resize(QSize(num,40));
                    
//                    m_imgList->setLayout(layout);
                    m_imglayout->addWidget(imageItem);
                    connect(imageItem,&ImageItem::imageItemclicked,this,[=](int index,int indexNow){
                        emit imageClicked(index,(index-indexNow));
                        emit ttbcontentClicked();
                    });
                }
                if ( path == info.filePath ) {
                    t=i;
                }
                i++;
            }
            labelList = m_imgList->findChildren<ImageItem*>();
            m_nowIndex = t;
            for(int j = 0; j < labelList.size(); j++){
                labelList.at(j)->setFixedSize (QSize(30,40));
                labelList.at(j)->resize (QSize(30,40));
                labelList.at(j)->setContentsMargins(1,5,1,5);
                labelList.at(j)->setFrameShape (QFrame::NoFrame);
                labelList.at(j)->setStyleSheet("border-width: 0px;border-style: solid;border-color: #2ca7f8;");
                labelList.at(j)->setIndexNow(t);
            }
            if(labelList.size()>0){
                labelList.at(t)->setFixedSize (QSize(60,58));
                labelList.at(t)->resize (QSize(60,58));
                labelList.at(t)->setFrameShape (QFrame::Box);
                labelList.at(t)->setContentsMargins(0,0,0,0);
                labelList.at(t)->setStyleSheet("border-width: 4px;border-style: solid;border-color: #2ca7f8;");
            }


            m_imgList->show();
            m_imgListView->show();

            QPropertyAnimation *animation = new QPropertyAnimation(m_imgList, "pos");
            animation->setDuration(500);
            animation->setEasingCurve(QEasingCurve::NCurveTypes);
            animation->setStartValue(m_imgList->pos());
            animation->setKeyValueAt(1,  QPoint(320-(32*t),0));
            animation->setEndValue(QPoint(320-(32*t),0));
            animation->start(QAbstractAnimation::DeleteWhenStopped);
            connect(animation, &QPropertyAnimation::finished,
                    animation, &QPropertyAnimation::deleteLater);


            m_imgListView->update();
            m_imgList->update();
            m_preButton->show();
            m_nextButton->show();
        }else {
            m_imgList->hide();
            m_preButton->hide();
            m_nextButton->hide();
            m_contentWidth = 420;
            setFixedWidth(m_contentWidth);
        }


        if (QFileInfo(path).isReadable() &&
                !QFileInfo(path).isWritable()) {
//            m_trashBtn->setDisabled(true);
            m_rotateLBtn->setDisabled(true);
            m_rotateRBtn->setDisabled(true);
        } else {
//            m_trashBtn->setDisabled(false);
            if (utils::image::imageSupportSave(path)) {
                m_rotateLBtn->setDisabled(false);
                m_rotateRBtn->setDisabled(false);
            } else {
                m_rotateLBtn->setDisabled(true);
                m_rotateRBtn->setDisabled(true);
            }
        }
    }

    m_imagePath = path;
    QString fileName = "";
    if(m_imagePath != ""){
        fileName = QFileInfo(m_imagePath).fileName();
    }
    emit dApp->signalM->updateFileName(fileName);
//    updateFilenameLayout();
    updateCollectButton();
}

void TTBContent::updateCollectButton()
{
    if (m_imagePath.isEmpty())
    {
        return;
    }

    if (DBManager::instance()->isImgExistInAlbum(FAVORITES_ALBUM, m_imagePath))
    {
        m_clBT->setNormalPic(":/resources/images/photo preview/checked/collection_checked.svg");
        m_clBT->setHoverPic(":/resources/images/photo preview/checked/collection_checked.svg");
        m_bClBTChecked = true;
    }
    else
    {
        m_clBT->setNormalPic(":/resources/images/photo preview/normal/collection_normal.svg");
        m_clBT->setHoverPic(":/resources/images/photo preview/hover/collection_hover.svg");
        m_bClBTChecked = false;
    }
}
