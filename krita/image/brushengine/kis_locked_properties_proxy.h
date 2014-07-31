/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
 *  Copyright (c) 2014 Mohit Goyal <mohit.bits2011@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef KIS_LOCKED_PROPERTIES_PROXY_H
#define KIS_LOCKED_PROPERTIES_PROXY_H

#include "kis_locked_properties.h"
#include "kis_properties_configuration.h"
#include "kis_locked_properties_server.h"
#include "kis_paintop_preset.h"

class KisLockedPropertiesServer;

class KisLockedPropertiesProxy: public KisPropertiesConfiguration
{
public:
    KisLockedPropertiesProxy() ;
    KisLockedPropertiesProxy(KisLockedProperties* p);
    KisLockedPropertiesProxy(const KisPropertiesConfiguration *, KisLockedProperties *);
    using KisPropertiesConfiguration::getProperty;
    QVariant getProperty(const QString &name) const;
    using KisPropertiesConfiguration::setProperty;
    void setProperty(const QString & name, const QVariant & value);







private:
    KisLockedProperties* m_lockedProperties;
    const KisPropertiesConfiguration* m_parent;


};

#endif // KIS_LOCKED_PROPERTIES_PROXY_H
