/*
 *
 *  Copyright (c) 2007 Cyrille Berger (cberger@cberger.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "panorama.h"

#include <stdlib.h>
#include <algorithm>

#include <kactioncollection.h>
#include <kcomponentdata.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>

#include <KoColorSpaceRegistry.h>

#include "kis_config.h"
#include "kis_cursor.h"
#include "kis_doc2.h"
#include "kis_filter_strategy.h"
#include "kis_global.h"
#include "kis_layer.h"
#include "kis_group_layer.h"
#include "kis_paint_device.h"
#include "kis_types.h"
#include "kis_view2.h"


#include "imagoptim_functions.h"
#include "harris_detector.h"
#include "kis_images_blender.h"
#include "kis_image_alignment.h"

#include "ui_wdgpanoramacreation.h"

typedef KGenericFactory<PanoramaPlugin> PanoramaPluginFactory;
K_EXPORT_COMPONENT_FACTORY( kritapanorama, PanoramaPluginFactory( "krita" ) )


PanoramaPlugin::PanoramaPlugin(QObject *parent, const QStringList &)
    : KParts::Plugin(parent), m_wdgPanoramaCreation(0)
{
    if ( parent->inherits("KisView2") )
    {
        m_view = (KisView2*) parent;

        setComponentData(PanoramaPluginFactory::componentData());

        setXMLFile(KStandardDirs::locate("data","kritaplugins/panorama.rc"), true);

        KAction *action  = new KAction(i18n("New panorama layer"), this);
        actionCollection()->addAction("PanoramaLayer", action );
        connect(action, SIGNAL(triggered()), this, SLOT(slotCreatePanoramaLayer()));
    }
    KisInterestPointsDetector::setInterestPointDetector(0, new HarrisPointDetector);
}

PanoramaPlugin::~PanoramaPlugin()
{
    m_view = 0;
    delete m_wdgPanoramaCreation;
}

void PanoramaPlugin::addImage(const QString& filename)
{
    m_wdgPanoramaCreation->listImages->addItem(filename);
}

void PanoramaPlugin::slotAddImages()
{
//     kDebug(41006) <<"Add image";
    QStringList openfiles = KFileDialog::getOpenFileNames(KUrl(),"*", m_view);
//     kDebug(41006) << openfiles.size() <<" files selected for inclusion";
    QString filename;
    foreach(filename, openfiles)
    {
        addImage( filename );
    }
}

void PanoramaPlugin::slotCreatePanoramaLayer()
{
    kDebug(41006) <<"Create a panorama layer";
    delete m_wdgPanoramaCreation;
    m_wdgPanoramaCreation = new Ui_WdgPanoramaCreation();
    QDialog* dialog = new QDialog(m_view);
    dialog->setModal(true);
    m_wdgPanoramaCreation->setupUi(dialog);

//     addImage("/home/cyrille/H0010632.JPG");
//     addImage("/home/cyrille/H0010633.JPG");
    addImage("/home/cyrille/0.png");
    addImage("/home/cyrille/1.png");
//     addImage("/home/cyrille/0.a.png");
//     addImage("/home/cyrille/1.a.png");

    connect(m_wdgPanoramaCreation->pushButtonCancel, SIGNAL(released()), dialog, SLOT(reject ()));
    connect(m_wdgPanoramaCreation->pushButtonCreatePanorama, SIGNAL(released()), dialog, SLOT(accept ()));
    connect(m_wdgPanoramaCreation->pushButtonAddImages, SIGNAL(released()), this, SLOT(slotAddImages()));
    connect(m_wdgPanoramaCreation->pushButtonPreview, SIGNAL(released()), this, SLOT(slotPreview()));

    if(dialog->exec()==QDialog::Accepted)
    {

        QList<KisImageAlignment::ImageInfo> images;
        for(int i = 0; i < m_wdgPanoramaCreation->listImages->count(); i++)
        {
            QString fileName = m_wdgPanoramaCreation->listImages->item(i)->text();
            kDebug(41006) <<"Loading fileName" << fileName;
            KisDoc2 d;
            d.import(fileName);
            KisImageAlignment::ImageInfo pi;
            KisImageSP img = d.image();
            if(not img) break;
//             img->scale(1000.0 / img->width(), 1000.0 / img->width(), 0, new KisBoxFilterStrategy);
            pi.device = img->projection();
            pi.rect = QRect(0,0, img->width(), img->height());
            images.push_back(pi);
        }


        KisPaintLayerSP layer = new KisPaintLayer(m_view->image(), i18n("Panorama Layer"), 255, images[0].device->colorSpace());
        Q_ASSERT(layer);
        KisGroupLayerSP parent;
        KisLayerSP above;
        if (KisGroupLayer* pactive = qobject_cast<KisGroupLayer*>(m_view->activeLayer().data()))
        {
            parent = pactive;
        }
        if(!parent)
            parent = m_view->image()->rootLayer();
        above = qobject_cast<KisLayer*>( parent->firstChild().data() );
        m_view->image()->addNode(layer.data(), parent.data(), above.data());


        QRect dstArea;
        createPanorama(images, layer->paintDevice(), dstArea);
        m_view->image()->rootLayer()->setDirty();
    }
    delete dialog;
}

#include <kurl.h>

void PanoramaPlugin::slotPreview()
{
    QList<KisImageAlignment::ImageInfo> images;
    for(int i = 0; i < m_wdgPanoramaCreation->listImages->count(); i++)
    {
        QString fileName = m_wdgPanoramaCreation->listImages->item(i)->text();
        kDebug(41006) <<"Loading fileName" << fileName;
        KisDoc2 d;
        d.import(fileName);
        KisImageAlignment::ImageInfo pi;
        KisImageSP img = d.image();
        if(not img) break;
//         img->scale(1000.0 / img->width(), 1000.0 / img->width(), 0, new KisBoxFilterStrategy);
//         img->scale(1000.0 / img->width(), 1000.0 / img->width(), 0, new KisMitchellFilterStrategy );
        pi.device = img->projection();
        pi.rect = QRect(0,0, img->width(), img->height());
        images.push_back(pi);
    }
    KisPaintDeviceSP dst = new KisPaintDevice( images[0].device->colorSpace(), "panorama preview" );
    QRect dstArea;
    createPanorama(images, dst, dstArea);
    QImage img = dst->convertToQImage(0,0.0);
    img = img.scaledToHeight(500);
    m_wdgPanoramaCreation->labelPreview->setPixmap(QPixmap::fromImage(img));
}

void PanoramaPlugin::createPanorama(QList<KisImageAlignment::ImageInfo>& images, KisPaintDeviceSP dstdevice, QRect& )
{
    KisImageAlignment ia( KisInterestPointsDetector::interestPointDetector());
    std::vector< KisImageAlignment::Result > p = ia.align(images);
    std::cout << "Number of results = " << p.size() << std::endl;
    // blend
    QList<KisImagesBlender::LayerSource> sources;
    // First layer
    KisImagesBlender::LayerSource firstLayer;
    firstLayer.layer = images[0].device;
    firstLayer.a = p[0].a;
    firstLayer.b = p[0].b;
    firstLayer.c = p[0].c;
    firstLayer.xc1 = images[0].rect.width()  * 0.5;
    firstLayer.xc2 = images[0].rect.width()  * 0.5;
    firstLayer.yc1 = images[0].rect.height() * 0.5;
    firstLayer.yc2 = images[0].rect.height() * 0.5;
    firstLayer.norm = (4.0 / ( images[0].rect.width() * images[0].rect.width() + images[0].rect.height() * images[0].rect.height() ) );
    firstLayer.homography = p[0].homography;
    firstLayer.rect = images[0].rect;
    sources.push_back(firstLayer);
    // Second Layer
    KisImagesBlender::LayerSource secondLayer;
    secondLayer.layer = images[1].device;
    secondLayer.a = p[1].a;
    secondLayer.b = p[1].b;
    secondLayer.c = p[1].c;
    secondLayer.xc1 = images[1].rect.width()  * 0.5;
    secondLayer.xc2 = images[1].rect.width()  * 0.5;
    secondLayer.yc1 = images[1].rect.height() * 0.5;
    secondLayer.yc2 = images[1].rect.height() * 0.5;
    secondLayer.norm = (4.0 / ( images[1].rect.width() * images[1].rect.width() + images[1].rect.height() * images[1].rect.height() ) );
    secondLayer.homography = p[1].homography;
    secondLayer.rect = images[1].rect;
    sources.push_back(secondLayer);

    KisImagesBlender::blend(sources, dstdevice);
}

#include "panorama.moc"
