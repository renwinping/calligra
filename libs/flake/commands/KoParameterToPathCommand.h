/* This file is part of the KDE project
 * Copyright (C) 2006,2008 Jan Hambrecht <jaham@gmx.net>
 * Copyright (C) 2006,2007 Thorsten Zachmann <zachmann@kde.org>
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

#ifndef KOPARAMETERTOPATHCOMMAND_H
#define KOPARAMETERTOPATHCOMMAND_H

#include <kundo2command.h>
#include <QList>

#include "flake_export.h"

class KoParameterShape;
class KoParameterToPathCommandPrivate;

/// The undo / redo command for changing a KoParameterShape into a KoPathShape
class FLAKE_EXPORT KoParameterToPathCommand : public KUndo2Command
{
public:
    /**
     * Constructor.
     * @param shape the shape this command works on
     * @param parent the parent command if this is a compound undo command.
     */
    explicit KoParameterToPathCommand(KoParameterShape *shape, KUndo2Command *parent = 0);
    /**
     * Constructor.
     * @param shapes the list of shapes this command works on
     * @param parent the parent command if this is a compound undo command.
     */
    explicit KoParameterToPathCommand(const QList<KoParameterShape*> &shapes, KUndo2Command *parent = 0);
    ~KoParameterToPathCommand() override;

    /// redo the command
    void redo() override;
    /// revert the actions done in redo
    void undo() override;
private:
    KoParameterToPathCommandPrivate * const d;
};

#endif // KOPARAMETERTOPATHCOMMAND_H
