/* This file is part of the KDE project

   Copyright 2008 Johannes Simon <johannes.simon@gmail.com>
   Copyright 2008 Inge Wallin    <inge@lysator.liu.se>
   Copyright 2018 Dag Andersen <danders@get2net.dk>

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
 * Boston, MA 02110-1301, USA.
*/


#ifndef KOCHART_RADARDATACONFIGWIDGET_H
#define KOCHART_RADARDATACONFIGWIDGET_H


#include "ConfigSubWidgetBase.h"
#include "ChartShape.h"


class KoShape;
class QAction;


namespace KoChart
{
class ChartShape;

/**
 * Chart type configuration widget.
 */
class RadarDataSetConfigWidget : public ConfigSubWidgetBase
{
    Q_OBJECT

public:
    RadarDataSetConfigWidget(QWidget *parent = 0);
    RadarDataSetConfigWidget(QList<ChartType> types, QWidget *parent = 0);
    ~RadarDataSetConfigWidget();

    void open(ChartShape* shape) override;
    QAction * createAction();

    /// reimplemented
    virtual bool showOnShapeCreate() { return true; }

    /// Delete all open dialogs.
    /// This is called when e.g. the tool is deactivated.
    void deleteSubDialogs(ChartType type = LastChartType);

    void updateMarkers();

public Q_SLOTS:
    void updateData(ChartType type, ChartSubtype subtype) override;

    void datasetMarkerSelected(QAction *action);
    void datasetBrushSelected(const QColor& color);
    void datasetPenSelected(const QColor& color);
    void ui_datasetShowCategoryChanged(bool b);
    void ui_dataSetShowNumberChanged(bool b);
    void ui_datasetShowPercentChanged(bool b);
    void ui_datasetShowSymbolChanged(bool b);
    void ui_dataSetSelectionChanged(int index);

Q_SIGNALS:
    void datasetPenChanged(DataSet *dataSet, const QColor& color, int section);
    void datasetBrushChanged(DataSet *dataSet, const QColor& color, int section);
    void dataSetMarkerChanged(DataSet *dataSet, OdfSymbolType type, OdfMarkerStyle style);
    void datasetShowCategoryChanged(DataSet *dataSet, bool b, int section);
    void dataSetShowNumberChanged(DataSet *dataSet, bool b, int section);
    void datasetShowPercentChanged(DataSet *dataSet, bool b, int section);
    void datasetShowSymbolChanged(DataSet *dataSet, bool b, int section);

private:

    class Private;
    Private * const d;
};

}  // namespace KoChart


#endif // KOCHART_RADARDATACONFIGWIDGET_H
