/* This file is part of the KDE project
 *
 * Copyright (C) 2009 - 2011 Inge Wallin <inge@lysator.liu.se>
 * Copyright (C) 2011 Boudewijn Rempt <boud@valdyas.org>
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


// Own
#include "VectorShape.h"

// Posix
#include <math.h>

// Qt
#include <QPen>
#include <QPainter>
#include <QBuffer>
#include <QDataStream>
#include <QPixmap>
#include <QMutexLocker>
#include <QThreadPool>

// KDE
#include <KDebug>

// Calligra
#include "KoUnit.h"
#include "KoStore.h"
#include "KoXmlNS.h"
#include "KoXmlReader.h"
#include <KoEmbeddedDocumentSaver.h>
#include <KoShapeLoadingContext.h>
#include <KoOdfLoadingContext.h>
#include <KoShapeSavingContext.h>
#include <KoViewConverter.h>

// Wmf support
#include "WmfPainterBackend.h"

// Vector shape
#include "libemf/EmfParser.h"
#include "libemf/EmfOutputPainterStrategy.h"
#include "libemf/EmfOutputDebugStrategy.h"
#include "libsvm/SvmParser.h"
#include "libsvm/SvmPainterBackend.h"


VectorShape::VectorShape()
    : KoFrameShape( KoXmlNS::draw, "image" )
    , m_type(VectorTypeNone)
    , m_isRendering(false)
{
    setShapeId(VectorShape_SHAPEID);
    // Default size of the shape.
    KoShape::setSize( QSizeF( CM_TO_POINT( 8 ), CM_TO_POINT( 5 ) ) );
    m_cache.setMaxCost(3);
}

VectorShape::~VectorShape()
{
}

// Methods specific to the vector shape.
QByteArray  VectorShape::compressedContents() const
{
    return m_contents;
}

void VectorShape::setCompressedContents( const QByteArray &newContents )
{
    QMutexLocker locker(&m_mutex);

    m_contents = newContents;
    m_type = VectorTypeUndetermined;
    m_cache.clear();
    update();
}

// ----------------------------------------------------------------
//                             Painting

RenderThread::RenderThread(VectorShape *shape, const QSizeF &size, const QSize &boundingSize, qreal zoomX, qreal zoomY)
    : QObject(), QRunnable(), m_shape(shape), m_size(size), m_boundingSize(boundingSize), m_zoomX(zoomX), m_zoomY(zoomY)
{
    setAutoDelete(true);
}

RenderThread::~RenderThread()
{
}

void RenderThread::run()
{
    QMutexLocker locker(&m_shape->m_mutex);
    QImage *image = new QImage(m_boundingSize, QImage::Format_ARGB32);
    image->fill(0);
    QPainter painter;
    if (!painter.begin(image)) {
        kWarning(31000) << "Failed to create image-cache";
        delete image;
        image = 0;
    } else {
        painter.scale(m_zoomX, m_zoomY);
        draw(painter);
        painter.end();
        qDebug()<<"DONE";
    }
    emit finished(m_boundingSize, image);
}

void RenderThread::draw(QPainter &painter)
{
    // If the data is uninitialized, e.g. because loading failed, draw the null shape.
    if (m_shape->m_contents.count() == 0) {
        drawNull(painter);
        return;
    }

    m_shape->m_contents = qUncompress(m_shape->m_contents);

    // Check if the type is undetermined.  It could be that if we got
    // the contents via setCompressedContents().
    //
    // FIXME: make setCompressedContents() return a bool and check the
    //        contents there already.
    if (m_shape->m_type == VectorShape::VectorTypeUndetermined) {
        // FIXME: Break out into its own function.
        if (VectorShape::isWmf(m_shape->m_contents)) {
            m_shape->m_type = VectorShape::VectorTypeWmf;
        }
        else if (VectorShape::isEmf(m_shape->m_contents)) {
            m_shape->m_type = VectorShape::VectorTypeEmf;
        }
        else if (VectorShape::isSvm(m_shape->m_contents)) {
            m_shape->m_type = VectorShape::VectorTypeSvm;
        }
        else
            m_shape->m_type = VectorShape::VectorTypeNone;
    }

    // Actually draw the contents
    switch (m_shape->m_type) {
    case VectorShape::VectorTypeNone:
        drawNull(painter);
        break;
    case VectorShape::VectorTypeWmf:
        drawWmf(painter);
        break;
    case VectorShape::VectorTypeEmf:
        drawEmf(painter);
        break;
    case VectorShape::VectorTypeSvm:
        drawSvm(painter);
        break;
    default:
        drawNull(painter);
    }
    m_shape->m_contents = qCompress(m_shape->m_contents);
}

void RenderThread::drawNull(QPainter &painter) const
{
    QRectF  rect(QPointF(0,0), m_size);
    painter.save();

    // Draw a simple cross in a rectangle just to indicate that there is something here.
    painter.setPen(QPen(QColor(172, 196, 206)));
    painter.drawRect(rect);
    painter.drawLine(rect.topLeft(), rect.bottomRight());
    painter.drawLine(rect.bottomLeft(), rect.topRight());

    painter.restore();
}

void RenderThread::drawWmf(QPainter &painter) const
{
    Libwmf::WmfPainterBackend  wmfPainter(&painter, m_size);
    if (!wmfPainter.load(m_shape->m_contents)) {
        drawNull(painter);
        return;
    }
    painter.save();
    // Actually paint the WMF.
    wmfPainter.play();
    painter.restore();
}

void RenderThread::drawEmf(QPainter &painter) const
{
    // FIXME: Make emfOutput use QSizeF
    QSize  shapeSizeInt( m_size.width(), m_size.height() );
    //kDebug(31000) << "-------------------------------------------";
    //kDebug(31000) << "size:     " << shapeSizeInt << m_size;
    //kDebug(31000) << "position: " << position();
    //kDebug(31000) << "-------------------------------------------";

    // FIXME: Make it static to save time?
    Libemf::Parser  emfParser;

#if 1  // Set to 0 to get debug output
    // Create a new painter output strategy.  Last param = true means keep aspect ratio.
    Libemf::OutputPainterStrategy  emfPaintOutput( painter, shapeSizeInt, true );
    emfParser.setOutput( &emfPaintOutput );
#else
    Libemf::OutputDebugStrategy  emfDebugOutput;
    emfParser.setOutput( &emfDebugOutput );
#endif
    emfParser.load(m_shape->m_contents);
}

void RenderThread::drawSvm(QPainter &painter) const
{
    QSize  shapeSizeInt( m_size.width(), m_size.height() );

    // FIXME: Make it static to save time?
    Libsvm::SvmParser  svmParser;

    // Create a new painter backend.
    Libsvm::SvmPainterBackend  svmPaintOutput(&painter, shapeSizeInt);
    svmParser.setBackend(&svmPaintOutput);
    svmParser.parse(m_shape->m_contents);
}

void VectorShape::paint(QPainter &painter, const KoViewConverter &converter)
{
#if 1  // Set to 0 to get uncached painting, which is good for debugging
    QRectF rc = converter.documentToView(boundingRect());

    // If necessary, recreate the cached image.
    QImage *cache = m_cache[rc.size().toSize().height()];
    if (!cache || cache->isNull()) {
        if (!m_isRendering) {
            m_isRendering = true;
            qreal zoomX, zoomY;
            converter.zoom(&zoomX, &zoomY);
            RenderThread *t = new RenderThread(this, size(), rc.size().toSize(), zoomX, zoomY);
            connect(t, SIGNAL(finished(QSize,QImage*)), this, SLOT(renderFinished(QSize,QImage*)));
            // Since the backends may use QPainter::drawText we need to make sure to only
            // use threads if the font-backend supports that what is in most cases.
            if (QFontDatabase::supportsThreadedFontRendering()) {
                QThreadPool::globalInstance()->start(t);
            } else { // else just execute the logic non-threaded.
                t->run();
            }
        }
    } else {
        QVector<QRect> clipRects = painter.clipRegion().rects();
        foreach (const QRect &rc, clipRects) {
            painter.drawImage(rc.topLeft(), *cache, rc);
        }
    }
#else
    applyConversion(painter, converter);
    draw(painter);
#endif
}

void VectorShape::renderFinished(QSize boundingSize, QImage *image)
{
    if (image) {
        m_cache.insert(boundingSize.height(), image);
        update();
    }
    m_isRendering = false;
}


// ----------------------------------------------------------------
//                         Loading and Saving


void VectorShape::saveOdf(KoShapeSavingContext & context) const
{
    QMutexLocker locker(&m_mutex);

    KoEmbeddedDocumentSaver &fileSaver = context.embeddedSaver();
    KoXmlWriter             &xmlWriter = context.xmlWriter();

    QString fileName = fileSaver.getFilename("VectorImages/Image");
    QByteArray mimeType;

    switch (m_type) {
    case VectorTypeWmf:
        mimeType = "application/x-wmf";
        break;
    case VectorTypeEmf:
        mimeType = "application/x-emf";
        break;
    case VectorTypeSvm:
        mimeType = "application/x-svm";// FIXME: Check if this is true
        break;
    default:
        // FIXME: What here?
        mimeType = "application/x-what";
        break;
    }

    xmlWriter.startElement("draw:frame");
    saveOdfAttributes(context, OdfAllAttributes);
    QByteArray  uncompressedContents = qUncompress(m_contents);
    fileSaver.embedFile(xmlWriter, "draw:image", fileName, mimeType.constData(),
                        uncompressedContents);
    xmlWriter.endElement(); // draw:frame
}

bool VectorShape::loadOdf(const KoXmlElement & element, KoShapeLoadingContext &context)
{
    //kDebug(31000) <<"Loading ODF frame in the vector shape. Element = " << element.tagName();
    loadOdfAttributes(element, context, OdfAllAttributes);
    return loadOdfFrame(element, context);
}


inline static int read32(const char *buffer, const int offset)
{
    // little endian
    int result = (int) buffer[offset];
    result |= (int) buffer[offset+1] << 8;
    result |= (int) buffer[offset+2] << 16;
    result |= (int) buffer[offset+3] << 24;

    return result;
}

// Load the actual contents within the vector shape.
bool VectorShape::loadOdfFrameElement(const KoXmlElement & element,
                                      KoShapeLoadingContext &context)
{
    //kDebug(31000) <<"Loading ODF element: " << element.tagName();
    QMutexLocker locker(&m_mutex);

    // Get the reference to the vector file.  If there is no href, then just return.
    const QString href = element.attribute("href");
    if (href.isEmpty())
        return false;

    // Try to open the embedded file.
    KoStore *store  = context.odfLoadingContext().store();
    bool     result = store->open(href);

    if (!result) {
        return false;
    }

    int size = store->size();
    if (size < 88) {
        store->close();
        return false;
    }

    m_contents = store->read(size);
    store->close();
    if (m_contents.count() < size) {
        kDebug(31000) << "Too few bytes read: " << m_contents.count() << " instead of " << size;
        return false;
    }

    // Try to recognize the type.  We should do this before the
    // compression below, because that's a semi-expensive operation.
    m_type = VectorTypeUndetermined;
    if (isWmf(m_contents)) {
        m_type = VectorTypeWmf;
    }
    else if (isEmf(m_contents)) {
        m_type = VectorTypeEmf;
    }
    else if (isSvm(m_contents)) {
        m_type = VectorTypeSvm;
    }
    else
        m_type = VectorTypeNone;

    // Return false if we didn't manage to identify the type.
    if (m_type == VectorTypeNone)
        return false;

    // Compress for biiiig memory savings.
    m_contents = qCompress(m_contents);

    return true;
}


bool VectorShape::isWmf(const QByteArray &bytes)
{
    kDebug(31000) << "Check for WMF";

    const char *data = bytes.constData();
    const int   size = bytes.count();

    if (size < 10)
        return false;

    // This is how the 'file' command identifies a WMF.
    if (data[0] == '\327' && data[1] == '\315' && data[2] == '\306' && data[3] == '\232')
    {
        // FIXME: Is this a compressed wmf?  Check it up.
        kDebug(31000) << "WMF identified: header 1";
        return true;
    }

    if (data[0] == '\002' && data[1] == '\000' && data[2] == '\011' && data[3] == '\000')
    {
        kDebug(31000) << "WMF identified: header 2";
        return true;
    }

    if (data[0] == '\001' && data[1] == '\000' && data[2] == '\011' && data[3] == '\000')
    {
        kDebug(31000) << "WMF identified: header 3";
        return true;
    }

    return false;
}

bool VectorShape::isEmf(const QByteArray &bytes)
{
    kDebug(31000) << "Check for EMF";

    const char *data = bytes.constData();
    const int   size = bytes.count();

    // This is how the 'file' command identifies an EMF.
    // 1. Check type
    qint32 mark = read32(data, 0);
    if (mark != 0x00000001) {
        //kDebug(31000) << "Not an EMF: mark = " << mark << " instead of 0x00000001";
        return false;
    }

    // 2. An EMF has the string " EMF" at the start + offset 40.
    if (size > 44
        && data[40] == ' ' && data[41] == 'E' && data[42] == 'M' && data[43] == 'F')
    {
        kDebug(31000) << "EMF identified";
        return true;
    }

    return false;
}

bool VectorShape::isSvm(const QByteArray &bytes)
{
    kDebug(31000) << "Check for SVM";

    // Check the SVM signature.
    if (bytes.startsWith("VCLMTF")) {
        kDebug(31000) << "SVM identified";
        return true;
    }

    return false;
}
