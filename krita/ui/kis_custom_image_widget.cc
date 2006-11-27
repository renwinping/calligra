/* This file is part of the KOffice project
 * Copyright (C) 2005 Thomas Zander <zander@kde.org>
 * Copyright (C) 2005 Casper Boemann <cbr@boemann.dk>
 *
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
#include "kis_custom_image_widget.h"

#include <QPushButton>
#include <QSlider>

#include <kcolorcombo.h>
#include <kdebug.h>

#include "KoUnitWidgets.h"
#include "KoColorSpaceRegistry.h"
#include "KoColorProfile.h"
#include "KoColorSpace.h"
#include "KoID.h"
#include "KoColor.h"
#include <KoUnit.h>

#include "kis_doc2.h"
#include "kis_meta_registry.h"
#include "kis_cmb_idlist.h"
#include "squeezedcombobox.h"
#include "kis_image.h"
#include "kis_layer.h"

KisCustomImageWidget::KisCustomImageWidget(QWidget *parent, KisDoc2 *doc, qint32 defWidth, qint32 defHeight, double resolution, const QString & defColorSpaceName, const QString & imageName)
    : WdgNewImage(parent) {
    m_doc = doc;

    txtName->setText(imageName);

    doubleWidth->setValue(defWidth);
    doubleHeight->setValue(defHeight);
    doubleResolution->setValue(resolution);

    cmbWidthUnit->addItem( i18n("Pixels") );
    cmbWidthUnit->addItems( KoUnit::listOfUnitName() );
    cmbHeightUnit->addItem( i18n("Pixels") );
    cmbHeightUnit->addItems( KoUnit::listOfUnitName() );

    cmbColorSpaces->setIDList(KisMetaRegistry::instance()->csRegistry()->listKeys());
    cmbColorSpaces->setCurrent(defColorSpaceName);

    connect(cmbColorSpaces, SIGNAL(activated(const KoID &)),
        this, SLOT(fillCmbProfiles(const KoID &)));
    connect (m_createButton, SIGNAL( clicked() ), this, SLOT (buttonClicked()) );
    m_createButton -> setDefault(true);

    fillCmbProfiles(cmbColorSpaces->currentItem());

}

void KisCustomImageWidget::buttonClicked() {
    KoColorSpace * cs = KisMetaRegistry::instance()->csRegistry()->colorSpace(cmbColorSpaces->currentItem(), cmbProfile->currentText());

    QColor qc(cmbColor->color());

    qint32 width, height;
    double resolution;
    resolution = doubleResolution->value() / 72.0;  // internal resolution is in pixels per pt

    switch(cmbWidthUnit->currentIndex())
    {
        case 0:
            width = int(doubleWidth->value());
            break;
        case 1:
            width = int(doubleWidth->value());
            break;
        case 2:
            width = int(doubleWidth->value());
            break;
        case 3:
            width = int(doubleWidth->value());
            break;
    }
     
    m_doc->newImage(txtName->text(), width, height, cs, KoColor(qc, cs), txtDescription->toPlainText(), resolution);

    KisImageSP img = m_doc->currentImage();
    if (img) {
        KisLayerSP layer = img->activeLayer();
        if (layer) {
            layer->setOpacity(backgroundOpacity());
        }
    }

    emit documentSelected();
}

quint8 KisCustomImageWidget::backgroundOpacity() const
{
    qint32 opacity = sliderOpacity->value();

    if (!opacity)
        return 0;

    return (opacity * 255) / 100;
}

void KisCustomImageWidget::fillCmbProfiles(const KoID & s)
{
    cmbProfile->clear();

    if (!KisMetaRegistry::instance()->csRegistry()->exists(s)) {
        return;
    }

    KoColorSpaceFactory * csf = KisMetaRegistry::instance()->csRegistry()->get(s);
    if (csf == 0) return;

    QList<KoColorProfile *>  profileList = KisMetaRegistry::instance()->csRegistry()->profilesFor( csf );

    foreach (KoColorProfile *profile, profileList) {
        cmbProfile->addSqueezedItem(profile->productName());
    }
    cmbProfile->setCurrent(csf->defaultProfile());
}

#include "kis_custom_image_widget.moc"
