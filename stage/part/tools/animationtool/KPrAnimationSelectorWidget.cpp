/* This file is part of the KDE project
   Copyright (C) 2012 Paul Mendez <paulestebanms@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "KPrAnimationSelectorWidget.h"

//Stage Headers
#include "KPrShapeAnimationDocker.h"
#include "KPrCollectionItemModel.h"
#include "KPrViewModePreviewShapeAnimations.h"
#include "animations/KPrShapeAnimation.h"
#include <KPrPredefinedAnimationsLoader.h>
#include "KPrFactory.h"

//Qt Headers
#include <QCheckBox>
#include <QGridLayout>
#include <QListWidget>
#include <QListView>
#include <QFont>

//KDE Headers
#include <KLocale>
#include <KIconLoader>
#include <KIcon>
#include <KConfigGroup>
#include <KGlobalSettings>
#include <KDebug>

//Calligra Headers
#include <KoXmlReader.h>
#include <KoOdfLoadingContext.h>
#include <KoShapeLoadingContext.h>
#include <KoOdfStylesReader.h>

KPrAnimationSelectorWidget::KPrAnimationSelectorWidget(KPrShapeAnimationDocker *docker, KPrPredefinedAnimationsLoader *animationsData,
                                                       QWidget *parent)
    : QWidget(parent)
    , m_docker(docker)
    , showAutomaticPreview(true)
    , m_animationsData(animationsData)
{   
    QGridLayout *containerLayout = new QGridLayout;

    QCheckBox *previewCheckBox = new QCheckBox(i18n("Automatic animation preview"), this);
    previewCheckBox->setChecked(loadPreviewConfig());
    showAutomaticPreview = previewCheckBox->isChecked();

    QFont viewWidgetFont  = KGlobalSettings::generalFont();
    qreal pointSize = KGlobalSettings::smallestReadableFont().pointSizeF();
    viewWidgetFont.setPointSizeF(pointSize);

    m_collectionChooser = new QListWidget;
    m_collectionChooser->setViewMode(QListView::IconMode);
    m_collectionChooser->setIconSize(QSize(KIconLoader::SizeLarge, KIconLoader::SizeLarge));
    m_collectionChooser->setSelectionMode(QListView::SingleSelection);
    m_collectionChooser->setResizeMode(QListView::Adjust);
    m_collectionChooser->setGridSize(QSize(75, 64));
    m_collectionChooser->setFixedWidth(90);
    m_collectionChooser->setMovement(QListView::Static);
    m_collectionChooser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_collectionChooser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_collectionChooser->setFont(viewWidgetFont);
    connect(m_collectionChooser, SIGNAL(itemClicked(QListWidgetItem *)),
            this, SLOT(activateShapeCollection(QListWidgetItem *)));

    m_collectionView = new QListView;
    m_collectionView->setViewMode(QListView::IconMode);
    m_collectionView->setIconSize(QSize(KIconLoader::SizeLarge, KIconLoader::SizeLarge));
    m_collectionView->setDragDropMode(QListView::DragOnly);
    m_collectionView->setSelectionMode(QListView::SingleSelection);
    m_collectionView->setResizeMode(QListView::Adjust);
    m_collectionView->setGridSize(QSize(75, 64));
    m_collectionView->setWordWrap(true);
    m_collectionView->viewport()->setMouseTracking(true);
    m_collectionView->setFont(viewWidgetFont);
    connect(m_collectionView, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(setAnimation(const QModelIndex&)));

    m_subTypeView = new QListView;
    m_subTypeView->setViewMode(QListView::IconMode);
    m_subTypeView->setIconSize(QSize(KIconLoader::SizeLarge, KIconLoader::SizeLarge));
    m_subTypeView->setDragDropMode(QListView::DragOnly);
    m_subTypeView->setSelectionMode(QListView::SingleSelection);
    m_subTypeView->setResizeMode(QListView::Adjust);
    m_subTypeView->setGridSize(QSize(75, 64));
    m_subTypeView->setFixedHeight(79);
    m_subTypeView->setWordWrap(true);
    m_subTypeView->viewport()->setMouseTracking(true);
    m_subTypeView->hide();
    m_subTypeView->setFont(viewWidgetFont);
    connect(m_subTypeView, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(setAnimation(const QModelIndex&)));

    containerLayout->addWidget(m_collectionChooser, 0, 0,2,1);
    containerLayout->addWidget(m_collectionView, 0, 1, 1, 1);
    containerLayout->addWidget(m_subTypeView, 1, 1, 1, 1);
    containerLayout->addWidget(previewCheckBox, 2, 0, 1, 2);


    // set signals
    connect(m_collectionView, SIGNAL(entered(QModelIndex)), this, SLOT(automaticPreviewRequested(QModelIndex)));
    connect(m_subTypeView, SIGNAL(entered(QModelIndex)), this, SLOT(automaticPreviewRequested(QModelIndex)));
    connect(previewCheckBox, SIGNAL(toggled(bool)), this, SLOT(setPreviewState(bool)));

    setLayout(containerLayout);
}

KPrAnimationSelectorWidget::~KPrAnimationSelectorWidget()
{
    savePreviewConfig();
    // stop animation
    if (m_docker->previewMode()) {
        m_docker->previewMode()->stopAnimation();
    }
}

void KPrAnimationSelectorWidget::init()
{
    //load predefined animations data
    m_animationsData->populateMainView(m_collectionChooser);
    m_collectionChooser->setCurrentRow(0);
    activateShapeCollection(m_collectionChooser->item(0));
}

void KPrAnimationSelectorWidget::automaticPreviewRequested(const QModelIndex &index)
{
    if(!index.isValid() || !showAutomaticPreview) {
        return;
    }
    KoXmlElement newAnimationContext;
    if (QObject::sender() == m_collectionView) {
        newAnimationContext = static_cast<KPrCollectionItemModel*>(m_collectionView->model())->animationContext(index);
    }
    else if (QObject::sender() == m_subTypeView) {
        newAnimationContext = static_cast<KPrCollectionItemModel*>(m_subTypeView->model())->animationContext(index);
    }
    else {
        return;
    }

    KoOdfStylesReader stylesReader;
    KoOdfLoadingContext context(stylesReader, 0);
    KoShapeLoadingContext shapeContext(context, 0);

    KoShape *shape = m_docker->getSelectedShape();
    if (!shape) {
        return;
    }
    m_previewAnimation = m_animationsData->loadOdfShapeAnimation(newAnimationContext, shapeContext, shape);
    if (m_previewAnimation) {
        emit requestPreviewAnimation(m_previewAnimation);
    }
}

void KPrAnimationSelectorWidget::activateShapeCollection(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    QString id = item->data(Qt::UserRole).toString();
    m_collectionView->setModel(m_animationsData->modelById(id));
    m_subTypeView->setModel(0);
    m_subTypeView->hide();
}

void KPrAnimationSelectorWidget::setAnimation(const QModelIndex &index)
{
    if(!index.isValid()) {
        return;
    }
    KoXmlElement newAnimationContext;
    if (QObject::sender() == m_collectionView) {
        m_subTypeView->hide();
        QString id = static_cast<KPrCollectionItemModel*>(m_collectionView->model())->data(index, Qt::UserRole).toString();
        if (m_animationsData->subModelById(id)){
            m_subTypeView->setModel(m_animationsData->subModelById(id));
            m_subTypeView->show();
            return;
        }
        newAnimationContext = static_cast<KPrCollectionItemModel*>(m_collectionView->model())->animationContext(index);
    }
    else if (QObject::sender() == m_subTypeView) {
        newAnimationContext = static_cast<KPrCollectionItemModel*>(m_subTypeView->model())->animationContext(index);
    }
    else {
        return;
    }

    KoOdfStylesReader stylesReader;
    KoOdfLoadingContext context(stylesReader, 0);
    KoShapeLoadingContext shapeContext(context, 0);

    KoShape *shape = m_docker->getSelectedShape();
    if (!shape) {
        kWarning(31000) << "No shape found";
        return;
    }

    KPrShapeAnimation *newAnimation = m_animationsData->loadOdfShapeAnimation(newAnimationContext, shapeContext, shape);
    if (newAnimation) {
        emit requestAcceptAnimation(newAnimation);
    }
}

void KPrAnimationSelectorWidget::setPreviewState(bool isEnable)
{
    showAutomaticPreview = isEnable;
}

bool KPrAnimationSelectorWidget::loadPreviewConfig()
{
    KSharedConfigPtr config = KPrFactory::componentData().config();
    bool showPreview = true;

    if (config->hasGroup("Interface")) {
        const KConfigGroup interface = config->group("Interface");
        showPreview = interface.readEntry("ShowAutomaticPreviewAnimationEditDocker", showPreview);
    }
    return showPreview;
}

void KPrAnimationSelectorWidget::savePreviewConfig()
{
    KSharedConfigPtr config = KPrFactory::componentData().config();
    KConfigGroup interface = config->group("Interface");
    interface.writeEntry("ShowAutomaticPreviewAnimationEditDocker", showAutomaticPreview);
}
