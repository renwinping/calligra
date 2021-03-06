/* This file is part of the KDE project
 * Copyright (c) 2008 Jan Hambrecht <jaham@gmx.net>
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

#ifndef KOINPUTDEVICEHANDLER_H
#define KOINPUTDEVICEHANDLER_H

#include "flake_export.h"
#include <QObject>

/**
 * Base class for all custom input devices.
 */
class FLAKE_EXPORT KoInputDeviceHandler : public QObject
{
    Q_OBJECT
public:
    KoInputDeviceHandler(QObject *parent, const QString &id);
    ~KoInputDeviceHandler() override;

    /**
     * Return the id for the device.
     * @return the id for the device
     */
    QString id() const;

    /**
     * Starts the device.
     * @return true if the device was started, false otherwise
     */
    virtual bool start() = 0;

    /**
     * Stops the device.
     * @return true if the device was stopped, false otherwise
     */
    virtual bool stop() = 0;

private:
    class Private;
    Private * const d;
};

#endif // KOINPUTDEVICEHANDLER_H
