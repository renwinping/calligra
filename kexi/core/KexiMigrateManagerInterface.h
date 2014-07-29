/* This file is part of the KDE project
   Copyright (C) 2014 Jarosław Staniek <staniek@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXI_MIGRATEMANAGERINTERFACE_H
#define KEXI_MIGRATEMANAGERINTERFACE_H

#include <kexi_export.h>

#include <QList>
#include <QString>

//! @short Interface for the Migrate Manger library management.
class KEXICORE_EXPORT KexiMigrateManagerInterface
{
public:
    virtual ~KexiMigrateManagerInterface() {}

    //! Implement to return the list offile MIME types that are supported by migration drivers
    virtual QList<QString> supportedFileMimeTypes() = 0;
};

#endif
