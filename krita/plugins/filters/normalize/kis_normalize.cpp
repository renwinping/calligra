/*
 *
 *  Copyright (c) 2015 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "kis_normalize.h"
#include <stdlib.h>
#include <vector>

#include <QPoint>
#include <QTime>
#include <QVector3D>

#include <klocale.h>

#include <kis_debug.h>
#include <kpluginfactory.h>

#include <kis_processing_information.h>
#include <kis_types.h>
#include <kis_selection.h>
#include <kis_layer.h>
#include <filter/kis_filter_registry.h>
#include <kis_global.h>

#include <KoColorSpaceMaths.h>
#include <filter/kis_color_transformation_configuration.h>

K_PLUGIN_FACTORY(KritaNormalizeFactory, registerPlugin<Normalize>();)
K_EXPORT_PLUGIN(KritaNormalizeFactory("krita"))

Normalize::Normalize(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KisFilterRegistry::instance()->add(KisFilterSP(new KisFilterNormalize()));
}

Normalize::~Normalize()
{
}


KisFilterNormalize::KisFilterNormalize()
                      : KisColorTransformationFilter(KoID("normalize"     , i18n("Normalize")),
                                  KisFilter::categoryMap(), i18n("&Normalize..."))
{
    setColorSpaceIndependence(FULLY_INDEPENDENT);
    setSupportsPainting(true);
    setShowConfigurationWidget(false);
}

KoColorTransformation* KisFilterNormalize::createTransformation(const KoColorSpace* cs, const KisFilterConfiguration* config) const
{
    Q_UNUSED(config);
    return new KisNormalizeTransformation(cs);
}

KisNormalizeTransformation::KisNormalizeTransformation(const KoColorSpace* cs) : m_colorSpace(cs), m_psize(cs->pixelSize())
{

}

void KisNormalizeTransformation::transform(const quint8* src, quint8* dst, qint32 nPixels) const
{
    QVector3D normal_vector;
    QVector<float> channelValues(4);
    while (nPixels--) {

	m_colorSpace->normalisedChannelsValue(src, channelValues);
        normal_vector.setX(channelValues[2]*2-1.0);
	normal_vector.setY(channelValues[1]*2-1.0);
	normal_vector.setZ(channelValues[0]*2-1.0);
	normal_vector.normalize();

        channelValues[0]=normal_vector.z()*0.5+0.5;
	channelValues[1]=normal_vector.y()*0.5+0.5;
	channelValues[2]=normal_vector.x()*0.5+0.5;
	channelValues[3]=1.0;
	
	m_colorSpace->fromNormalisedChannelsValue(dst, channelValues);

	dst[3]=src[3];
        src += m_psize;
        dst += m_psize;
    }
}