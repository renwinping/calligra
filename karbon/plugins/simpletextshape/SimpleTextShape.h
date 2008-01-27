/* This file is part of the KDE project
 * Copyright (C) 2007 Jan Hambrecht <jaham@gmx.net>
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

#ifndef SIMPLETEXTSHAPE_H
#define SIMPLETEXTSHAPE_H

#include <KoShape.h>

#include <QtGui/QFont>

class QPainter;
class KoPathShape;

#define SimpleTextShapeID "SimpleText"

class SimpleTextShape : public KoShape
{
public:
    enum TextAnchor { AnchorStart, AnchorMiddle, AnchorEnd };

    enum LayoutMode {
        Straight,    ///< baseline is a straight line
        OnPath,      ///< baseline is a QPainterPath
        OnPathShape  ///< baseline is the outline of a path shape
    };

    SimpleTextShape();
    virtual ~SimpleTextShape();

    /// reimplemented to be empty (this shape is fully non-printing)
    void paint(QPainter &painter, const KoViewConverter &converter);
    /// reimplemented
    void paintDecorations(QPainter &painter, const KoViewConverter &converter, const KoCanvasBase *canvas);
    /// reimplemented
    virtual void saveOdf(KoShapeSavingContext & context) const;
    /// reimplemented
    virtual bool loadOdf( const KoXmlElement & element, KoShapeLoadingContext &context );
    /// reimplemented
    virtual QSizeF size() const;
    /// reimplemented
    virtual void setSize( const QSizeF &size );
    /// reimplemented
    virtual const QPainterPath outline() const;

    /// Sets the text to display
    void setText( const QString & text );

    /// Returns the text content
    QString text() const;

    /**
     * Sets the font used for drawing
     * Note that it is expected that the font has its point size set
     * in postscript points.
     */
    void setFont( const QFont & font );

    /// Returns the font
    QFont font() const;

    /// Attaches this text shape to the given path shape
    bool putOnPath( KoPathShape * path );

    /// Puts the text on the given path, the path is expected to be in document coordinates
    bool putOnPath( const QPainterPath &path );

    /// Detaches this text shape from an already attached path shape
    void removeFromPath();

    /// Returns if shape is attached to a path shape
    bool isOnPath() const;

    /// Sets the offset for for text on path
    void setStartOffset( qreal offset );

    /// Returns the start offset for text on path
    qreal startOffset() const;

    /**
     * Returns the y-offset from the top-left corner to the baseline.
     * This is usable for being able to exactly position the texts baseline.
     * Note: The value makes only sense for text not attached to a path.
     */
    qreal baselineOffset() const;

    /// Sets the text anchor
    void setTextAnchor( TextAnchor anchor );

    /// Returns the actual text anchor
    TextAnchor textAnchor() const;

    /// Returns the current layout mode
    LayoutMode layout() const;

    /// Returns the baseline path
    QPainterPath baseline() const;

    /// Returns a pointer to the shape used as baseline
    const KoPathShape * baselineShape() const;

    void enableTextCursor( bool enable );
    int textCursor() const { return m_textCursor; }
    void setTextCursor( int textCursor );
    void removeFromTextCursor( unsigned int nr );
    void addToTextCursor( const QString &str );

private:
    void updateSizeAndPosition();
    void cacheGlyphOutlines();
    bool pathHasChanged() const;
    virtual void notifyShapeChanged( KoShape * shape, ChangeType type );
    /// reimplemented from KoShape
    virtual KoShape * cloneShape() const;

    void setTextCursorInternal( int textCursor );

    void createOutline();
    QString m_text; ///< the text content
    QFont m_font; ///< the font to use for drawing
    KoPathShape * m_path; ///< the path shape we are attached to
    QList<QPainterPath> m_charOutlines; ///< cached character oulines
    qreal m_startOffset; ///< the offset from the attached path start point
    qreal m_baselineOffset; ///< the y-offset from the top-left corner to the baseline
    QPainterPath m_outline; ///< the actual outline
    QPainterPath m_baseline; ///< the baseline path the text is on
    TextAnchor m_textAnchor; ///< the actual text anchor
    int m_textCursor;
};

#endif // SIMPLETEXTSHAPE_H
