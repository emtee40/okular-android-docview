/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>
    SPDX-FileCopyrightText: 2021 David Hurka <david.hurka@mailbox.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pagepainter.h"

// qt / kde includes
#include <KIconLoader>
#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRect>
#include <QTransform>

// system includes
#include <math.h>

// local includes
#include "core/annotations.h"
#include "core/page.h"
#include "core/tile.h"
#include "debug_ui.h"
#include "guiutils.h"
#include "settings.h"
#include "settings_core.h"

Q_GLOBAL_STATIC_WITH_ARGS(QPixmap, busyPixmap, (QIcon::fromTheme(QLatin1String("okular")).pixmap(KIconLoader::SizeLarge)))

#define TEXTANNOTATION_ICONSIZE 24

inline QPen buildPen(const Okular::Annotation *ann, double width, const QColor &color)
{
    QColor c = color;
    c.setAlphaF(ann->style().opacity());
    QPen p(QBrush(c), width, ann->style().lineStyle() == Okular::Annotation::Dashed ? Qt::DashLine : Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    return p;
}

void PagePainter::paintPageOnPainter(QPainter *destPainter,
                                     const Okular::Page *page,
                                     Okular::DocumentObserver *observer,
                                     const QRectF &cropRect,
                                     qreal scale,
                                     PagePainter::PagePainterFlags flags,
                                     const Okular::NormalizedPoint &viewPortPoint)
{
    // Variables prefixed with d are scaled to device pixels,
    // i. e. multiplied with the device pixel ratio of the target PaintDevice.
    // Variables prefixed with n are normalized to the page at current rotation.
    // Other geometry variables are in destPainter coordinates.

    destPainter->save();
    const qreal dpr = destPainter->device()->devicePixelRatioF();

    const QSizeF pageSize = QSizeF(page->width(), page->height()) * scale;

    // Clipping
    // Remember that QRect::bottomRight is misaligned by 1 pixel.
    /** cropRect parameter expanded to snap at device pixels. */
    const QRect dPaintingLimits = QRectF(cropRect.topLeft() * dpr, cropRect.bottomRight() * dpr).toAlignedRect();
    /** cropRect parameter expanded to snap at device pixels. */
    const QRectF paintingLimits(dPaintingLimits.topLeft() / dpr, (dPaintingLimits.bottomRight() + QPoint(1, 1)) / dpr);
    destPainter->setClipRect(paintingLimits, Qt::IntersectClip);

    // Paper background color
    QColor backgroundColor = Qt::white;
    if (Okular::SettingsCore::changeColors()) {
        switch (Okular::SettingsCore::renderMode()) {
        case Okular::SettingsCore::EnumRenderMode::Inverted:
        case Okular::SettingsCore::EnumRenderMode::InvertLightness:
        case Okular::SettingsCore::EnumRenderMode::InvertLuma:
        case Okular::SettingsCore::EnumRenderMode::InvertLumaSymmetric:
            backgroundColor = Qt::black;
            break;
        case Okular::SettingsCore::EnumRenderMode::Paper:
            backgroundColor = Okular::SettingsCore::paperColor();
            break;
        case Okular::SettingsCore::EnumRenderMode::Recolor:
            backgroundColor = Okular::Settings::recolorBackground();
            break;
        default:;
        }
    }
    destPainter->fillRect(paintingLimits, backgroundColor);

    // Draw page pixmaps which are prerendered by Generator
    const DrawPagePixmapsResult drawPixmapsResult = drawPagePixmapsOnPainter(destPainter, page, observer, cropRect, scale);

    if (drawPixmapsResult & NoPixmap) {
        drawLoadingPixmapOnPainter(destPainter, QRectF(QPointF(0.0, 0.0), pageSize));
    }

    // Draw other objects of the page
    drawPageHighlightsOnPainter(destPainter, page, scale, flags);
    drawPageObjectBordersOnPainter(destPainter, page, scale, flags);
    drawPageAnnotationsOnPainter(destPainter, page, scale, flags);

    if (flags & ViewPortPoint) {
        drawViewPortPointOnPainter(destPainter, pageSize, viewPortPoint);
    }

    destPainter->restore();
}

PagePainter::DrawPagePixmapsResult PagePainter::drawPagePixmapsOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, const QRectF &cropRect, qreal scale, PagePainterFlags flags)
{
    Q_UNUSED(flags)

    const qreal dpr = destPainter->device()->devicePixelRatioF();
    DrawPagePixmapsResult result = Fine;

    const QRect dPaintingLimits = QRectF(cropRect.topLeft() * dpr, cropRect.bottomRight() * dpr).toAlignedRect();
    const QSize dPageSize = QSize(int(page->width() * dpr * scale), int(page->height() * dpr * scale));
    const Okular::NormalizedRect ndPaintingLimits(dPaintingLimits, dPageSize.width(), dPageSize.height());

    if (!page->hasTilesManager(observer)) {
        return drawPagePixmapOnPainter(destPainter, page, observer, dPageSize);
    }

    // Get available tiles
    const QList<Okular::Tile> tiles = page->tilesAt(observer, ndPaintingLimits);

    // Draw tiles
    for (const Okular::Tile tile : tiles) {
        tile.pixmap()->setDevicePixelRatio(dpr);

        // Tile position: Appears to be correct with geometry() instead of roundedGeometry().
        // TODO This is still not 100% correct. Okular::Tile needs to be adjusted.
        // Tiles have different sizes if the page size is not even.
        // But the normalized geometry stored in the tile is not adjusted to their actual size.
        const QRect dTileGeometry = tile.rect().geometry(dPageSize.width(), dPageSize.height());

        // Calculate tile size, prefer rounding up to avoid gaps between tiles.
        // We can accept up to 1px tolerance per axis, because tiles have 1px margins.
        // In such cases, drawing with an overlap of 1px looks better than scaling by 1px.
        const QSize dTileSizeIs = tile.pixmap()->size();
        const QSize dTileSizeShould = QSize(ceil(qreal(dPageSize.width()) * tile.rect().width()), ceil(qreal(dPageSize.height()) * tile.rect().height()));
        const QSize dTileSizeMismatch = dTileSizeIs - dTileSizeShould;
        if (qAbs(dTileSizeMismatch.width()) > 1 || qAbs(dTileSizeMismatch.height()) > 1) {
            destPainter->save();
            destPainter->translate(QPointF(dTileGeometry.topLeft()) / dpr);
            destPainter->scale(qreal(dTileSizeShould.width()) / qreal(dTileSizeIs.width()), qreal(dTileSizeShould.height()) / qreal(dTileSizeIs.height()));
            drawPixmapWithColorMode(destPainter, QPointF(0.0, 0.0), *tile.pixmap(), flags);
            destPainter->restore();

            result = DrawPagePixmapsResult(result | PixmapsOfIncorrectSize);
        } else {
            drawPixmapWithColorMode(destPainter, QPointF(dTileGeometry.topLeft()) / dpr, *tile.pixmap(), flags);
        }
    }
    return result;
}

PagePainter::DrawPagePixmapsResult PagePainter::drawPagePixmapOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, QSize dSize, PagePainterFlags flags)
{
    Q_UNUSED(flags)

    // Get pixmap:
    const QPixmap *nearestPixmap = page->_o_nearestPixmap(observer, dSize.width(), dSize.height());
    if (!nearestPixmap) {
        return NoPixmap;
    }

    // Draw:
    QPixmap pixmap(*nearestPixmap);
    pixmap.setDevicePixelRatio(destPainter->device()->devicePixelRatioF());

    if (pixmap.size() == dSize) {
        drawPixmapWithColorMode(destPainter, QPointF(0.0, 0.0), pixmap, flags);
        return Fine;
    } else {
        destPainter->save();
        // This scaling needs to be component wise because some generators create pixmaps with wrong aspect ratio.
        destPainter->scale(qreal(dSize.width()) / qreal(pixmap.width()), qreal(dSize.height() / qreal(pixmap.height())));
        drawPixmapWithColorMode(destPainter, QPointF(0.0, 0.0), pixmap, flags);
        destPainter->restore();
        return PixmapsOfIncorrectSize;
    }
}

void PagePainter::drawPixmapWithColorMode(QPainter *destPainter, QPointF position, const QPixmap &pixmap, PagePainter::PagePainterFlags flags)
{
    const bool changeColors = (flags & Accessibility) && Okular::Settings::changeColors() && (Okular::Settings::renderMode() != Okular::Settings::EnumRenderMode::Paper);

    if (!changeColors) {
        destPainter->drawPixmap(position, pixmap);
    } else {
        destPainter->save();

        // First, go to device pixel coordinate system of this pixmap (not the painter's device).
        const qreal dpr = pixmap.devicePixelRatioF();
        destPainter->translate(position);
        destPainter->scale(1.0 / dpr, 1.0 / dpr);

        // Get only the part of the pixmap that is going to be visible.
        const QRect pixmapPartToPaint = QRect(QPoint(0, 0), pixmap.size()).intersected(destPainter->clipBoundingRect().toAlignedRect());
        QImage image = pixmap.copy(pixmapPartToPaint).toImage();
        image.setDevicePixelRatio(1.0);

        // Do color modification on this part.
        switch (Okular::SettingsCore::renderMode()) {
        case Okular::SettingsCore::EnumRenderMode::Inverted:
            // Invert image pixels using QImage internal function
            image.invertPixels(QImage::InvertRgb);
            break;
        case Okular::SettingsCore::EnumRenderMode::Recolor:
            recolor(&image, Okular::Settings::recolorForeground(), Okular::Settings::recolorBackground());
            break;
        case Okular::SettingsCore::EnumRenderMode::BlackWhite:
            blackWhite(&image, Okular::Settings::bWContrast(), Okular::Settings::bWThreshold());
            break;
        case Okular::SettingsCore::EnumRenderMode::InvertLightness:
            invertLightness(&image);
            break;
        case Okular::SettingsCore::EnumRenderMode::InvertLuma:
            invertLuma(&image, 0.2126, 0.7152, 0.0722); // sRGB / Rec. 709 luma coefficients
            break;
        case Okular::SettingsCore::EnumRenderMode::InvertLumaSymmetric:
            invertLuma(&image, 0.3333, 0.3334, 0.3333); // Symmetric coefficients, to keep colors saturated.
            break;
        case Okular::SettingsCore::EnumRenderMode::HueShiftPositive:
            hueShiftPositive(&image);
            break;
        case Okular::SettingsCore::EnumRenderMode::HueShiftNegative:
            hueShiftNegative(&image);
            break;
        }

        // Paint this part.
        destPainter->drawImage(pixmapPartToPaint.topLeft(), image);

        destPainter->restore();
    }
}

void PagePainter::drawLoadingPixmapOnPainter(QPainter *destPainter, QRectF pagePosition)
{
    // draw something on the blank page: the okular icon or a cross (as a fallback)
    if (!busyPixmap()->isNull()) {
        busyPixmap->setDevicePixelRatio(destPainter->device()->devicePixelRatioF());
        destPainter->drawPixmap(pagePosition.topLeft() + QPointF(10.0, 10.0), *busyPixmap());
    } else {
        destPainter->setPen(Qt::gray);
        destPainter->drawLine(pagePosition.topLeft(), pagePosition.bottomRight());
        destPainter->drawLine(pagePosition.topRight(), pagePosition.bottomLeft());
    }
}

void PagePainter::drawPageHighlightsOnPainter(QPainter *destPainter, const Okular::Page *page, qreal scale, PagePainterFlags flags)
{
    const bool drawHighlights = (flags & Highlights);
    const bool drawTextSelection = (flags & TextSelection);

    if (!(drawHighlights || drawTextSelection)) {
        return;
    }

    // Highlight rects are painted in a device pixel coordinate system for two reasons:
    // * The outlines shall be pixel aligned.
    // * RegularArea::geometry() thinks in integers.

    // TODO The geometry transformation in RegularArea::geometry() could be skipped
    // if NormalizedRect was a QRectF, so RegularArea could be used as QVector<QRectF>.

    const qreal dpr = destPainter->device()->devicePixelRatioF();

    const QSize dPageSize = QSize(int(page->width() * dpr * scale), int(page->height() * dpr * scale));

    destPainter->save();
    destPainter->scale(1.0 / dpr, 1.0 / dpr);

    destPainter->setCompositionMode(QPainter::CompositionMode_Multiply);

    if (drawHighlights) {
        for (const Okular::HighlightAreaRect *highlight : qAsConst(page->m_highlights)) {
            destPainter->setPen(highlight->color.darker(150));
            destPainter->setBrush(highlight->color);

            const QList<QRect> dRects = highlight->geometry(dPageSize.width(), dPageSize.height());
            destPainter->drawRects(QVector<QRect>(dRects.cbegin(), dRects.cend()));
        }
    }

    if (drawTextSelection && page->textSelection()) {
        destPainter->setPen(page->textSelectionColor().darker(150));
        destPainter->setBrush(page->textSelectionColor());

        const QList<QRect> dRects = page->textSelection()->geometry(dPageSize.width(), dPageSize.height());
        destPainter->drawRects(QVector<QRect>(dRects.cbegin(), dRects.cend()));
    }

    destPainter->restore();
}

void PagePainter::drawPageObjectBordersOnPainter(QPainter *destPainter, const Okular::Page *page, qreal scale, PagePainter::PagePainterFlags flags)
{
    const bool enhanceLinks = (flags & EnhanceLinks) && Okular::Settings::highlightLinks();
    const bool enhanceImages = (flags & EnhanceImages) && Okular::Settings::highlightImages();

    if (!(enhanceLinks || enhanceImages)) {
        return;
    }

    // Object borders are painted in the normalized page coordinate system with hairline outlines,
    // because ObjectRect::region() thinks in normalized coordinates.

    destPainter->save();
    destPainter->scale(scale * page->width(), scale * page->height());

    destPainter->setPen(QPen(QApplication::palette().color(QPalette::Active, QPalette::Highlight), 0));
    destPainter->setBrush(Qt::NoBrush);

    for (const Okular::ObjectRect *object : qAsConst(page->m_rects)) {
        if ((enhanceLinks && object->objectType() == Okular::ObjectRect::Action) || (enhanceImages && object->objectType() == Okular::ObjectRect::Image)) {
            destPainter->drawPath(object->region());
        }
    }

    destPainter->restore();
}

void PagePainter::drawPageAnnotationsOnPainter(QPainter *destPainter, const Okular::Page *page, qreal scale, PagePainter::PagePainterFlags flags)
{
    if (!(flags & Annotations)) {
        return;
    }

    const QSizeF pageSize = QSizeF(page->width(), page->height()) * scale;

    // Draw annotation moving outlines on top of other annotations.
    QList<const Okular::Annotation *> boundingRectOnlyAnnotations;

    for (const Okular::Annotation *annotation : qAsConst(page->m_annotations)) {
        const Okular::Annotation::Flag flags = Okular::Annotation::Flag(annotation->flags());

        if (flags & Okular::Annotation::Hidden) {
            continue;
        }

        if (flags & Okular::Annotation::ExternallyDrawn) {
            if ((flags & Okular::Annotation::BeingMoved) || (flags & Okular::Annotation::BeingResized)) {
                boundingRectOnlyAnnotations.append(annotation);
            }
            continue;
        }

        drawAnnotationOnPainter(destPainter, annotation, pageSize, scale);
    }

    for (const Okular::Annotation *annotation : qAsConst(boundingRectOnlyAnnotations)) {
        drawAnnotationBoundingBoxOnPainter(destPainter, annotation, pageSize);
    }
}

void PagePainter::drawAnnotationBoundingBoxOnPainter(QPainter *destPainter, const Okular::Annotation *annotation, QSizeF pageSize)
{
    // The annotation outline is drawn as hairline to get pixel alignment.
    destPainter->save();
    destPainter->setPen(QPen(Qt::black, 0.0, Qt::DashLine));
    destPainter->setBrush(Qt::NoBrush);

    destPainter->drawRect(annotation->transformedBoundingRectangle().geometry(pageSize.width(), pageSize.height()));

    destPainter->restore();
}

void PagePainter::drawAnnotationOnPainter(QPainter *destPainter, const Okular::Annotation *annotation, QSizeF pageSize, qreal scale)
{
    const qreal dpr = destPainter->device()->devicePixelRatioF();

    const Okular::Annotation::SubType type = annotation->subType();
    const int mainOpacity = annotation->style().color().alpha() * annotation->style().opacity();

    if (mainOpacity <= 0.0 && annotation->subType() != Okular::Annotation::AText) {
        // Text is not subject to `mainOpacity`. Otherwise, skip invisible annotations.
        return;
    }

    // This `boundingBox` here is assumed to be accurate for annotations which
    // create a graphical shape on the document.
    const QRect boundingBox = annotation->transformedBoundingRectangle().geometry(pageSize.width(), pageSize.height());

    if (type == Okular::Annotation::AText && static_cast<const Okular::TextAnnotation *>(annotation)->textType() == Okular::TextAnnotation::Linked) {
        // `boundingBox` is not accurate for popup text annotations.
        if (!destPainter->clipBoundingRect().intersects(QRectF(boundingBox.topLeft(), QSizeF(TEXTANNOTATION_ICONSIZE, TEXTANNOTATION_ICONSIZE)))) {
            return;
        }
    } else {
        if (!destPainter->clipBoundingRect().intersects(boundingBox)) {
            return;
        }
    }

    QColor mainColor = annotation->style().color();
    if (!mainColor.isValid()) {
        mainColor = Qt::yellow;
    }
    mainColor.setAlphaF(mainOpacity);

    destPainter->save();

    if (type == Okular::Annotation::AText) {
        const Okular::TextAnnotation *textAnnotation = static_cast<const Okular::TextAnnotation *>(annotation);
        if (textAnnotation->textType() == Okular::TextAnnotation::InPlace) {
            // Draw inline text annotation.
            QImage image(boundingBox.size(), QImage::Format_ARGB32);
            image.fill(mainColor);
            QPainter painter(&image);
            painter.scale(scale, scale);
            painter.setFont(textAnnotation->textFont());
            painter.setPen(textAnnotation->textColor());
            const Qt::AlignmentFlag horizontalAlignment = (textAnnotation->inplaceAlignment() == 1 ? Qt::AlignHCenter : textAnnotation->inplaceAlignment() == 2 ? Qt::AlignRight : Qt::AlignLeft);
            const qreal borderWidth = textAnnotation->style().width();
            painter.drawText(QRectF(QPointF(borderWidth, borderWidth), QPointF(image.width() / scale - borderWidth, image.height() / scale - borderWidth)), Qt::AlignTop | horizontalAlignment | Qt::TextWordWrap, textAnnotation->contents());

            if (borderWidth > 0.0) {
                painter.resetTransform();
                painter.setPen(QPen(Qt::black, borderWidth));
                painter.drawRect(QRect(QPoint(0, 0), QSize(image.width() - 1, image.height() - 1)));
            }

            painter.end();
            destPainter->drawImage(boundingBox.topLeft(), image);
        } else if (textAnnotation->textType() == Okular::TextAnnotation::Linked) {
            // Draw popup text annotation.
            QPixmap pixmap = QIcon::fromTheme(textAnnotation->textIcon().toLower()).pixmap(TEXTANNOTATION_ICONSIZE);

            if (textAnnotation->style().color().isValid()) {
                QImage image = pixmap.toImage();
                GuiUtils::colorizeImage(image, textAnnotation->style().color(), mainOpacity);
                pixmap = QPixmap::fromImage(image);
            }

            destPainter->drawPixmap(boundingBox.topLeft(), pixmap);
        }
    } else if (type == Okular::Annotation::ALine) {
        // Draw line annotation.
        // TODO caption, dash pattern, endings for multipoint lines.
        const Okular::LineAnnotation *lineAnnotation = static_cast<const Okular::LineAnnotation *>(annotation);

        // Approximated margin for line end decorations
        const int margin = lineAnnotation->style().width() * 20.0;
        QRect imageRect = boundingBox.adjusted(-margin, -margin, margin, margin);
        imageRect &= destPainter->clipBoundingRect().toAlignedRect();
        QImage image(imageRect.size() * dpr, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::transparent);
        QTransform imageTransform;
        imageTransform.scale(1.0 / imageRect.width(), 1.0 / imageRect.height());
        imageTransform.translate(-imageRect.left(), -imageRect.top());
        imageTransform.scale(pageSize.width(), pageSize.height());

        LineAnnotPainter linePainter(lineAnnotation, pageSize / scale, scale, imageTransform);
        linePainter.draw(image);

        destPainter->drawImage(imageRect.topLeft(), image);
    } else if (type == Okular::Annotation::AGeom) {
        // Draw geometric shape annotation.
        const Okular::GeomAnnotation *geomAnnotation = static_cast<const Okular::GeomAnnotation *>(annotation);

        const qreal lineWidth = geomAnnotation->style().width() * scale;
        destPainter->setPen(buildPen(geomAnnotation, lineWidth, mainColor));
        QColor fillColor = geomAnnotation->geometricalInnerColor();
        if (fillColor.isValid()) {
            fillColor.setAlpha(mainOpacity);
            destPainter->setBrush(fillColor);
        } else {
            destPainter->setBrush(Qt::NoBrush);
        }

        // boundingBox shall define the bounding box including the outline.
        const qreal w = lineWidth / 2.0;
        const QRectF shape = QRectF(boundingBox).adjusted(w, w, -w, -w);

        if (geomAnnotation->geometricalType() == Okular::GeomAnnotation::InscribedSquare) {
            destPainter->drawRect(shape);
        } else {
            destPainter->drawEllipse(shape);
        }
    } else if (type == Okular::Annotation::AHighlight) {
        // Draw text markup annotation.
        // TODO under/strike width, feather, capping. [sic]
        const Okular::HighlightAnnotation *highlightAnnotation = static_cast<const Okular::HighlightAnnotation *>(annotation);
        const Okular::HighlightAnnotation::HighlightType type = highlightAnnotation->highlightType();

        if (type == Okular::HighlightAnnotation::Highlight || type == Okular::HighlightAnnotation::Squiggly) {
            destPainter->setPen(Qt::NoPen);
            destPainter->setBrush(mainColor);
            destPainter->setCompositionMode(QPainter::CompositionMode_Multiply);
        } else {
            destPainter->setPen(QPen(mainColor, 2.0));
            destPainter->setBrush(Qt::NoBrush);
        }

        for (const Okular::HighlightAnnotation::Quad &quad : qAsConst(highlightAnnotation->highlightQuads())) {
            QPolygonF path;
            for (int i = 0; i < 4; ++i) {
                QPointF point;
                point.setX(quad.transformedPoint(i).x * pageSize.width());
                point.setY(quad.transformedPoint(i).y * pageSize.height());
                path.append(point);
            }

            if (type == Okular::HighlightAnnotation::Highlight) {
                // Highlight the whole quad.
                destPainter->drawPolygon(path);
            } else if (type == Okular::HighlightAnnotation::Squiggly) {
                // Highlight botton half of the quad.
                path[3] = (path[0] + path[3]) / 2.0;
                path[2] = (path[1] + path[2]) / 2.0;
                destPainter->drawPolygon(path);
            } else if (type == Okular::HighlightAnnotation::Underline) {
                // Make a line at bottom quarter of the quad.
                path[0] = (path[0] * 3.0 + path[3]) / 4.0;
                path[1] = (path[1] * 3.0 + path[2]) / 4.0;
                path.removeLast();
                path.removeLast();
                destPainter->drawPolygon(path);
            } else if (type == Okular::HighlightAnnotation::StrikeOut) {
                // Make a line at middle of the quad.
                path[0] = (path[0] + path[3]) / 2.0;
                path[1] = (path[1] + path[2]) / 2.0;
                path.removeLast();
                path.removeLast();
                destPainter->drawPolygon(path);
            }
        }
    } else if (type == Okular::Annotation::AStamp) {
        // Draw stamp annotation.
        const Okular::StampAnnotation *stampAnnotation = static_cast<const Okular::StampAnnotation *>(annotation);

        QPixmap pixmap = Okular::AnnotationUtils::loadStamp(stampAnnotation->stampIconName(), qMax(boundingBox.width(), boundingBox.height()) * dpr);

        destPainter->setOpacity(mainOpacity);
        destPainter->drawPixmap(boundingBox, pixmap);
    } else if (type == Okular::Annotation::AInk) {
        // Draw freehand line annotation.
        // TODO invar width, PENTRACER. [sic]
        const Okular::InkAnnotation *inkAnnotation = static_cast<const Okular::InkAnnotation *>(annotation);

        destPainter->setPen(buildPen(inkAnnotation, inkAnnotation->style().width() * scale, mainColor));
        destPainter->setBrush(Qt::NoBrush);

        for (const QLinkedList<Okular::NormalizedPoint> &points : inkAnnotation->transformedInkPaths()) {
            QPolygonF path;
            for (const Okular::NormalizedPoint &nPoint : points) {
                QPointF point;
                point.setX(nPoint.x * pageSize.width());
                point.setY(nPoint.y * pageSize.height());
                path.append(point);
            }

            destPainter->drawPolyline(path);
        }
    }

    destPainter->restore();

    if (Okular::Settings::debugDrawAnnotationRect()) {
        destPainter->save();
        destPainter->setPen(QPen(annotation->style().color(), 0.0));
        destPainter->setBrush(Qt::NoBrush);
        destPainter->drawRect(boundingBox);
        destPainter->restore();
    }
}

void PagePainter::drawViewPortPointOnPainter(QPainter *destPainter, QSizeF pageSize, Okular::NormalizedPoint point)
{
    destPainter->save();
    destPainter->setPen(QPen(QApplication::palette().color(QPalette::Active, QPalette::Highlight), 0.0));

    const qreal y = point.y * pageSize.height();
    destPainter->drawLine(QPointF(0.0, y), QPointF(pageSize.width(), y));

    destPainter->restore();
}

void PagePainter::recolor(QImage *image, const QColor &foreground, const QColor &background)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularUiDebug) << "Wrong image format! Converting...";
        *image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    Q_ASSERT(image->format() == QImage::Format_ARGB32_Premultiplied);

    const float scaleRed = background.redF() - foreground.redF();
    const float scaleGreen = background.greenF() - foreground.greenF();
    const float scaleBlue = background.blueF() - foreground.blueF();

    const int foreground_red = foreground.red();
    const int foreground_green = foreground.green();
    const int foreground_blue = foreground.blue();

    QRgb *data = reinterpret_cast<QRgb *>(image->bits());
    const int pixels = image->width() * image->height();

    for (int i = 0; i < pixels; ++i) {
        const int lightness = qGray(data[i]);

        const float r = scaleRed * lightness + foreground_red;
        const float g = scaleGreen * lightness + foreground_green;
        const float b = scaleBlue * lightness + foreground_blue;

        const unsigned a = qAlpha(data[i]);
        data[i] = qRgba(r, g, b, a);
    }
}

void PagePainter::blackWhite(QImage *image, int contrast, int threshold)
{
    unsigned int *data = reinterpret_cast<unsigned int *>(image->bits());
    int con = contrast;
    int thr = 255 - threshold;

    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        // Piecewise linear function of val, through (0, 0), (thr, 128), (255, 255)
        int val = qGray(data[i]);
        if (val > thr)
            val = 128 + (127 * (val - thr)) / (255 - thr);
        else if (val < thr)
            val = (128 * val) / thr;

        // Linear contrast stretching through (thr, thr)
        if (con > 2) {
            val = thr + (val - thr) * con / 2;
            val = qBound(0, val, 255);
        }

        const unsigned a = qAlpha(data[i]);
        data[i] = qRgba(val, val, val, a);
    }
}

void PagePainter::invertLightness(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularUiDebug) << "Wrong image format! Converting...";
        *image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    Q_ASSERT(image->format() == QImage::Format_ARGB32_Premultiplied);

    QRgb *data = reinterpret_cast<QRgb *>(image->bits());
    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        // Invert lightness of the pixel using the cylindric HSL color model.
        // Algorithm is based on https://en.wikipedia.org/wiki/HSL_and_HSV#HSL_to_RGB (2019-03-17).
        // Important simplifications are that inverting lightness does not change chroma and hue.
        // This means the sector (of the chroma/hue plane) is not changed,
        // so we can use a linear calculation after determining the sector using qMin() and qMax().
        uchar R = qRed(data[i]);
        uchar G = qGreen(data[i]);
        uchar B = qBlue(data[i]);

        // Get only the needed HSL components. These are chroma C and the common component m.
        // Get common component m
        uchar m = qMin(R, qMin(G, B));
        // Remove m from color components
        R -= m;
        G -= m;
        B -= m;
        // Get chroma C
        uchar C = qMax(R, qMax(G, B));

        // Get common component m' after inverting lightness L.
        // Hint: Lightness L = m + C / 2; L' = 255 - L = 255 - (m + C / 2) => m' = 255 - C - m
        uchar m_ = 255 - C - m;

        // Add m' to color compontents
        R += m_;
        G += m_;
        B += m_;

        // Save new color
        const unsigned A = qAlpha(data[i]);
        data[i] = qRgba(R, G, B, A);
    }
}

void PagePainter::invertLuma(QImage *image, float Y_R, float Y_G, float Y_B)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularUiDebug) << "Wrong image format! Converting...";
        *image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    Q_ASSERT(image->format() == QImage::Format_ARGB32_Premultiplied);

    QRgb *data = reinterpret_cast<QRgb *>(image->bits());
    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        uchar R = qRed(data[i]);
        uchar G = qGreen(data[i]);
        uchar B = qBlue(data[i]);

        invertLumaPixel(R, G, B, Y_R, Y_G, Y_B);

        // Save new color
        const unsigned A = qAlpha(data[i]);
        data[i] = qRgba(R, G, B, A);
    }
}

void PagePainter::invertLumaPixel(uchar &R, uchar &G, uchar &B, float Y_R, float Y_G, float Y_B)
{
    // Invert luma of the pixel using the bicone HCY color model, stretched to cylindric HSY.
    // Algorithm is based on https://en.wikipedia.org/wiki/HSL_and_HSV#Luma,_chroma_and_hue_to_RGB (2019-03-19).
    // For an illustration see https://experilous.com/1/product/make-it-colorful/ (2019-03-19).

    // Special case: The algorithm does not work when hue is undefined.
    if (R == G && G == B) {
        R = 255 - R;
        G = 255 - G;
        B = 255 - B;
        return;
    }

    // Get input and output luma Y, Y_inv in range 0..255
    float Y = R * Y_R + G * Y_G + B * Y_B;
    float Y_inv = 255 - Y;

    // Get common component m and remove from color components.
    // This moves us to the bottom faces of the HCY bicone, i. e. we get C and X in R, G, B.
    uint_fast8_t m = qMin(R, qMin(G, B));
    R -= m;
    G -= m;
    B -= m;

    // We operate in a hue plane of the luma/chroma/hue bicone.
    // The hue plane is a triangle.
    // This bicone is distorted, so we can not simply mirror the triangle.
    // We need to stretch it to a luma/saturation rectangle, so we need to stretch chroma C and the proportional X.

    // First, we need to calculate luma Y_full_C for the outer corner of the triangle.
    // Then we can interpolate the max chroma C_max, C_inv_max for our luma Y, Y_inv.
    // Then we calculate C_inv and X_inv by scaling them by the ratio of C_max and C_inv_max.

    // Calculate luma Y_full_C (in range equivalent to gray 0..255) for chroma = 1 at this hue.
    // Piecewise linear, with the corners of the bicone at the sum of one or two luma coefficients.
    float Y_full_C;
    if (R >= B && B >= G) {
        Y_full_C = 255 * Y_R + 255 * Y_B * B / R;
    } else if (R >= G && G >= B) {
        Y_full_C = 255 * Y_R + 255 * Y_G * G / R;
    } else if (G >= R && R >= B) {
        Y_full_C = 255 * Y_G + 255 * Y_R * R / G;
    } else if (G >= B && B >= R) {
        Y_full_C = 255 * Y_G + 255 * Y_B * B / G;
    } else if (B >= G && G >= R) {
        Y_full_C = 255 * Y_B + 255 * Y_G * G / B;
    } else {
        Y_full_C = 255 * Y_B + 255 * Y_R * R / B;
    }

    // Calculate C_max, C_inv_max, to scale C and X.
    float C_max, C_inv_max;
    if (Y >= Y_full_C) {
        C_max = Y_inv / (255 - Y_full_C);
    } else {
        C_max = Y / Y_full_C;
    }
    if (Y_inv >= Y_full_C) {
        C_inv_max = Y / (255 - Y_full_C);
    } else {
        C_inv_max = Y_inv / Y_full_C;
    }

    // Scale C and X. C and X already lie in R, G, B.
    float C_scale = C_inv_max / C_max;
    float R_ = R * C_scale;
    float G_ = G * C_scale;
    float B_ = B * C_scale;

    // Calculate missing luma (in range 0..255), to get common component m_inv
    float m_inv = Y_inv - (Y_R * R_ + Y_G * G_ + Y_B * B_);

    // Add m_inv to color compontents
    R_ += m_inv;
    G_ += m_inv;
    B_ += m_inv;

    // Return colors rounded
    R = R_ + 0.5;
    G = G_ + 0.5;
    B = B_ + 0.5;
}

void PagePainter::hueShiftPositive(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularUiDebug) << "Wrong image format! Converting...";
        *image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    Q_ASSERT(image->format() == QImage::Format_ARGB32_Premultiplied);

    QRgb *data = reinterpret_cast<QRgb *>(image->bits());
    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        uchar R = qRed(data[i]);
        uchar G = qGreen(data[i]);
        uchar B = qBlue(data[i]);

        // Save new color
        const unsigned A = qAlpha(data[i]);
        data[i] = qRgba(B, R, G, A);
    }
}

void PagePainter::hueShiftNegative(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularUiDebug) << "Wrong image format! Converting...";
        *image = image->convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    Q_ASSERT(image->format() == QImage::Format_ARGB32_Premultiplied);

    QRgb *data = reinterpret_cast<QRgb *>(image->bits());
    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        uchar R = qRed(data[i]);
        uchar G = qGreen(data[i]);
        uchar B = qBlue(data[i]);

        // Save new color
        const unsigned A = qAlpha(data[i]);
        data[i] = qRgba(G, B, R, A);
    }
}

void PagePainter::drawShapeOnImage(QImage &image, const NormalizedPath &normPath, bool closeShape, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op
                                   // float antiAliasRadius
)
{
    // safety checks
    int pointsNumber = normPath.size();
    if (pointsNumber < 2)
        return;

    const double dpr = image.devicePixelRatio();
    const double fImageWidth = image.width() / dpr;
    const double fImageHeight = image.height() / dpr;

    // stroke outline
    double penWidth = (double)pen.width() * penWidthMultiplier;
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen2 = pen;
    pen2.setWidthF(penWidth);
    painter.setPen(pen2);
    painter.setBrush(brush);

    if (op == Multiply) {
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    }

    if (brush.style() == Qt::NoBrush) {
        // create a polygon
        QPolygonF poly(closeShape ? pointsNumber + 1 : pointsNumber);
        for (int i = 0; i < pointsNumber; ++i) {
            poly[i] = QPointF(normPath[i].x * fImageWidth, normPath[i].y * fImageHeight);
        }
        if (closeShape)
            poly[pointsNumber] = poly[0];

        painter.drawPolyline(poly);
    } else {
        // create a 'path'
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);

        path.moveTo(normPath[0].x * fImageWidth, normPath[0].y * fImageHeight);
        for (int i = 1; i < pointsNumber; i++) {
            path.lineTo(normPath[i].x * fImageWidth, normPath[i].y * fImageHeight);
        }
        if (closeShape)
            path.closeSubpath();

        painter.drawPath(path);
    }
}

void PagePainter::drawEllipseOnImage(QImage &image, const NormalizedPath &rect, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op)
{
    const double dpr = image.devicePixelRatio();
    const double fImageWidth = image.width() / dpr;
    const double fImageHeight = image.height() / dpr;

    // stroke outline
    const double penWidth = (double)pen.width() * penWidthMultiplier;
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen2 = pen;
    pen2.setWidthF(penWidth);
    painter.setPen(pen2);
    painter.setBrush(brush);

    if (op == Multiply) {
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    }

    const QPointF &topLeft {rect[0].x * fImageWidth, rect[0].y * fImageHeight};
    const QSizeF &size {(rect[1].x - rect[0].x) * fImageWidth, (rect[1].y - rect[0].y) * fImageHeight};
    const QRectF imgRect {topLeft, size};
    if (brush.style() == Qt::NoBrush) {
        painter.drawArc(imgRect, 0, 16 * 360);
    } else {
        painter.drawEllipse(imgRect);
    }
}

LineAnnotPainter::LineAnnotPainter(const Okular::LineAnnotation *a, QSizeF pageSize, double pageScale, const QTransform &toNormalizedImage)
    : la {a}
    , pageSize {pageSize}
    , pageScale {pageScale}
    , toNormalizedImage {toNormalizedImage}
    , aspectRatio {pageSize.height() / pageSize.width()}
    , linePen {buildPen(a, a->style().width(), a->style().color())}
{
    if ((la->lineClosed() || la->transformedLinePoints().count() == 2) && la->lineInnerColor().isValid()) {
        fillBrush = QBrush(la->lineInnerColor());
    }
}

void LineAnnotPainter::draw(QImage &image) const
{
    const QLinkedList<Okular::NormalizedPoint> transformedLinePoints = la->transformedLinePoints();
    if (transformedLinePoints.count() == 2) {
        const Okular::NormalizedPoint delta {transformedLinePoints.last().x - transformedLinePoints.first().x, transformedLinePoints.first().y - transformedLinePoints.last().y};
        const double angle {atan2(delta.y * aspectRatio, delta.x)};
        const double cosA {cos(-angle)};
        const double sinA {sin(-angle)};
        const QTransform tmpMatrix = QTransform {cosA, sinA / aspectRatio, -sinA, cosA / aspectRatio, transformedLinePoints.first().x, transformedLinePoints.first().y};
        const double deaspectedY {delta.y * aspectRatio};
        const double mainSegmentLength {sqrt(delta.x * delta.x + deaspectedY * deaspectedY)};
        const double lineendSize {std::min(6. * la->style().width() / pageSize.width(), mainSegmentLength / 2.)};

        drawShortenedLine(mainSegmentLength, lineendSize, image, tmpMatrix);
        drawLineEnds(mainSegmentLength, lineendSize, image, tmpMatrix);
        drawLeaderLine(0., image, tmpMatrix);
        drawLeaderLine(mainSegmentLength, image, tmpMatrix);
    } else if (transformedLinePoints.count() > 2) {
        drawMainLine(image);
    }
}

void LineAnnotPainter::drawMainLine(QImage &image) const
{
    // draw the line as normalized path into image
    PagePainter::drawShapeOnImage(image, transformPath(la->transformedLinePoints(), toNormalizedImage), la->lineClosed(), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawShortenedLine(double mainSegmentLength, double size, QImage &image, const QTransform &toNormalizedPage) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{shortenForArrow(size, la->lineStartStyle()), 0}, {mainSegmentLength - shortenForArrow(size, la->lineEndStyle()), 0}};
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), la->lineClosed(), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEnds(double mainSegmentLength, double size, QImage &image, const QTransform &transform) const
{
    switch (la->lineStartStyle()) {
    case Okular::LineAnnotation::Square:
        drawLineEndSquare(0, -size, transform, image);
        break;
    case Okular::LineAnnotation::Circle:
        drawLineEndCircle(0, -size, transform, image);
        break;
    case Okular::LineAnnotation::Diamond:
        drawLineEndDiamond(0, -size, transform, image);
        break;
    case Okular::LineAnnotation::OpenArrow:
        drawLineEndArrow(0, -size, 1., false, transform, image);
        break;
    case Okular::LineAnnotation::ClosedArrow:
        drawLineEndArrow(0, -size, 1., true, transform, image);
        break;
    case Okular::LineAnnotation::None:
        break;
    case Okular::LineAnnotation::Butt:
        drawLineEndButt(0, size, transform, image);
        break;
    case Okular::LineAnnotation::ROpenArrow:
        drawLineEndArrow(0, size, 1., false, transform, image);
        break;
    case Okular::LineAnnotation::RClosedArrow:
        drawLineEndArrow(0, size, 1., true, transform, image);
        break;
    case Okular::LineAnnotation::Slash:
        drawLineEndSlash(0, -size, transform, image);
        break;
    }
    switch (la->lineEndStyle()) {
    case Okular::LineAnnotation::Square:
        drawLineEndSquare(mainSegmentLength, size, transform, image);
        break;
    case Okular::LineAnnotation::Circle:
        drawLineEndCircle(mainSegmentLength, size, transform, image);
        break;
    case Okular::LineAnnotation::Diamond:
        drawLineEndDiamond(mainSegmentLength, size, transform, image);
        break;
    case Okular::LineAnnotation::OpenArrow:
        drawLineEndArrow(mainSegmentLength, size, 1., false, transform, image);
        break;
    case Okular::LineAnnotation::ClosedArrow:
        drawLineEndArrow(mainSegmentLength, size, 1., true, transform, image);
        break;
    case Okular::LineAnnotation::None:
        break;
    case Okular::LineAnnotation::Butt:
        drawLineEndButt(mainSegmentLength, size, transform, image);
        break;
    case Okular::LineAnnotation::ROpenArrow:
        drawLineEndArrow(mainSegmentLength, size, -1., false, transform, image);
        break;
    case Okular::LineAnnotation::RClosedArrow:
        drawLineEndArrow(mainSegmentLength, size, -1., true, transform, image);
        break;
    case Okular::LineAnnotation::Slash:
        drawLineEndSlash(mainSegmentLength, size, transform, image);
        break;
    }
}

void LineAnnotPainter::drawLineEndArrow(double xEndPos, double size, double flipX, bool close, const QTransform &toNormalizedPage, QImage &image) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos - size * flipX, size / 2.},
        {xEndPos, 0},
        {xEndPos - size * flipX, -size / 2.},
    };
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), close, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndButt(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const double halfSize {size / 2.};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos, halfSize},
        {xEndPos, -halfSize},
    };
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndCircle(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const
{
    /* transform the circle midpoint to intermediate normalized coordinates
     * where it's easy to construct the bounding rect of the circle */
    Okular::NormalizedPoint center;
    toNormalizedPage.map(xEndPos - size / 2., 0, &center.x, &center.y);
    const double halfSize {size / 2.};
    const QList<Okular::NormalizedPoint> path {
        {center.x - halfSize, center.y - halfSize / aspectRatio},
        {center.x + halfSize, center.y + halfSize / aspectRatio},
    };

    /* then transform bounding rect with toNormalizedImage */
    PagePainter::drawEllipseOnImage(image, transformPath(path, toNormalizedImage), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndSquare(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{xEndPos, size / 2.}, {xEndPos - size, size / 2.}, {xEndPos - size, -size / 2.}, {xEndPos, -size / 2.}};
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndDiamond(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{xEndPos, 0}, {xEndPos - size / 2., size / 2.}, {xEndPos - size, 0}, {xEndPos - size / 2., -size / 2.}};
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndSlash(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const double halfSize {size / 2.};
    const double xOffset {cos(M_PI / 3.) * halfSize};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos - xOffset, halfSize},
        {xEndPos + xOffset, -halfSize},
    };
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLeaderLine(double xEndPos, QImage &image, const QTransform &toNormalizedPage) const
{
    const QTransform combinedTransform = toNormalizedPage * toNormalizedImage;
    const double ll = aspectRatio * la->lineLeadingForwardPoint() / pageSize.height();
    const double lle = aspectRatio * la->lineLeadingBackwardPoint() / pageSize.height();
    const int sign {ll > 0 ? -1 : 1};
    QList<Okular::NormalizedPoint> path;

    if (fabs(ll) > 0) {
        path.append({xEndPos, ll});
        // do we have the extension on the "back"?
        if (fabs(lle) > 0) {
            path.append({xEndPos, sign * lle});
        } else {
            path.append({xEndPos, 0});
        }
    }
    PagePainter::drawShapeOnImage(image, transformPath(path, combinedTransform), false, linePen, fillBrush, pageScale);
}

double LineAnnotPainter::shortenForArrow(double size, Okular::LineAnnotation::TermStyle endStyle)
{
    double shortenBy {0};

    if (endStyle == Okular::LineAnnotation::Square || endStyle == Okular::LineAnnotation::Circle || endStyle == Okular::LineAnnotation::Diamond || endStyle == Okular::LineAnnotation::ClosedArrow) {
        shortenBy = size;
    }

    return shortenBy;
}

/* kate: replace-tabs on; indent-width 4; */
