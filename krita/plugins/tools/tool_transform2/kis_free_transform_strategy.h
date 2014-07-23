/*
 *  Copyright (c) 2014 Dmitry Kazakov <dimula73@gmail.com>
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

#ifndef __KIS_FREE_TRANSFORM_STRATEGY_H
#define __KIS_FREE_TRANSFORM_STRATEGY_H

#include <QObject>
#include <QScopedPointer>

class QPointF;
class QPainter;
class KisCoordinatesConverter;
class ToolTransformArgs;
class QTransform;
class TransformTransactionProperties;
class QCursor;
class QImage;

class KisFreeTransformStrategy : public QObject
{
    Q_OBJECT
public:
    KisFreeTransformStrategy(const KisCoordinatesConverter *converter,
                             ToolTransformArgs &currentArgs,
                             TransformTransactionProperties &transaction,
                             QTransform &transform);
    ~KisFreeTransformStrategy();

    void setTransformFunction(const QPointF &mousePos, bool usePerspective);
    void paint(QPainter &gc);
    QCursor getCurrentCursor() const;

    void setThumbnailImage(const QImage &image, QTransform thumbToImageTransform);

    void externalConfigChanged();
    bool beginPrimaryAction(const QPointF &pt);
    void continuePrimaryAction(const QPointF &pt, bool specialModifierActve);
    bool endPrimaryAction();


    void recalculateTransformationsWORKAROUND();

    enum StrokeFunction {
        ROTATE = 0,
        MOVE,
        RIGHTSCALE,
        TOPRIGHTSCALE,
        TOPSCALE,
        TOPLEFTSCALE,
        LEFTSCALE,
        BOTTOMLEFTSCALE,
        BOTTOMSCALE,
        BOTTOMRIGHTSCALE,
        BOTTOMSHEAR,
        RIGHTSHEAR,
        TOPSHEAR,
        LEFTSHEAR,
        MOVECENTER,
        PERSPECTIVE
    };

    StrokeFunction functionWORKAROUND() const;

signals:
    void requestCanvasUpdate();
    void requestResetRotationCenterButtons();
    void requestShowImageTooBig(bool value);

private:
    class Private;
    const QScopedPointer<Private> m_d;
};

#endif /* __KIS_FREE_TRANSFORM_STRATEGY_H */
