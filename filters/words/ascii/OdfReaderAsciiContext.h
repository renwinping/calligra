/* This file is part of the KDE project

   Copyright (C) 2013 Inge Wallin <inge@lysator.liu.se>

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
   Boston, MA 02110-1301, USA.
*/

#ifndef ODFREADERASCIICONTEXT_H
#define ODFREADERASCIICONTEXT_H

// Qt
#include <QHash>
#include <QTextStream>

// Calligra
#include <KoFilter.h>

// libodfreader
#include "OdfReaderContext.h"


class QFile;
class KoStore;



class OdfReaderAsciiContext : public OdfReaderContext 
{
 public: 
    OdfReaderAsciiContext(KoStore *store, QFile &file);
    ~OdfReaderAsciiContext() override;

    QTextStream  outStream;
};


#endif // ODFREADERASCIICONTEXT_H
