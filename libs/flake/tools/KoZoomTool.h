/* This file is part of the KDE project
 *
 * Copyright (C) 2006-2007 Thomas Zander <zander@kde.org>
 * Copyright (C) 2006 Thorsten Zachmann <zachmann@kde.org>
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

#ifndef KOZOOMTOOL_H
#define KOZOOMTOOL_H

#include "KoInteractionTool.h"

#include <QCursor>

class KoCanvasBase;
class KoCanvasController;

/// \internal
class KoZoomTool : public KoInteractionTool
{
Q_OBJECT
public:
    /**
     * Create a new tool; typically not called by applications, only by the KoToolManager
     * @param canvas the canvas this tool works for.
     */
    explicit KoZoomTool(KoCanvasBase *canvas);
    /// reimplemented method
    void wheelEvent(KoPointerEvent *event) override;
    /// reimplemented method
    void mouseReleaseEvent(KoPointerEvent *event) override;
    /// reimplemented method
    void mouseMoveEvent(KoPointerEvent *event) override;
    /// reimplemented method
    void keyPressEvent(QKeyEvent *event) override;
    /// reimplemented method
    void keyReleaseEvent(QKeyEvent *event) override;
    /// reimplemented method
    void activate(ToolActivation toolActivation, const QSet<KoShape*> &shapes) override;
    /// reimplemented method
    void mouseDoubleClickEvent(KoPointerEvent *event) override;

    void setCanvasController(KoCanvasController *controller) {
        m_controller = controller;
    }

    void setZoomInMode(bool zoomIn);

protected:
    QWidget *createOptionWidget() override;

private:
    KoInteractionStrategy *createStrategy(KoPointerEvent *event) override;

    void updateCursor(bool swap);

    KoCanvasController *m_controller;
    QCursor m_inCursor;
    QCursor m_outCursor;
    bool m_temporary;
    bool m_zoomInMode;
};

#endif
