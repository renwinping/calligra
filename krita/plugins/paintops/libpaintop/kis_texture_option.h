/* This file is part of the KDE project
 * Copyright (C) Boudewijn Rempt <boud@valdyas.org>, (C) 2012
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KIS_TEXTURE_OPTION_H
#define KIS_TEXTURE_OPTION_H

#include <krita_export.h>

#include "kis_paintop_option.h"

class KisTextureOptionWidget;
class KisPattern;
class KoResource;
class KisPropertiesConfiguration;

class PAINTOP_EXPORT KisTextureOption : public KisPaintOpOption
{
    Q_OBJECT
public:

    enum TextureChannel {
        ALPHA,
        HUE,
        SATURATION,
        LIGHT
    };

    explicit KisTextureOption(QObject *parent= 0);
    virtual ~KisTextureOption();

public slots:
    
    virtual void writeOptionSetting(KisPropertiesConfiguration* setting) const;
    virtual void readOptionSetting(const KisPropertiesConfiguration* setting);

private slots:

    void resetGUI(KoResource*); /// called when a new pattern is selected

private:
    KisTextureOptionWidget *m_optionWidget;


};

class PAINTOP_EXPORT KisTextureProperties
{
public:
    KisTextureProperties()
        : pattern(0)
        , m_mask(0)
    {}

    bool enabled;
    qreal scale;
    int offsetX;
    int offsetY;
    qreal strength;
    bool invert;
    KisTextureOption::TextureChannel activeChannel;
    KisPattern *pattern;

    void apply(KisFixedPaintDeviceSP dab, const QPoint& _offset);
    void fillProperties(const KisPropertiesConfiguration *setting);

private:

    KisFixedPaintDevice *m_mask;

    void recalculateMask();
};

#endif // KIS_TEXTURE_OPTION_H
