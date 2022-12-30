/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "pagepainter.h"

// qt / kde includes
#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QRect>
#include <QTransform>
#include <QVarLengthArray>

// system includes
#include <math.h>

// local includes
#include "core/annotations.h"
#include "core/observer.h"
#include "core/page.h"
#include "core/recolor.h"
#include "core/tile.h"
#include "core/utils.h"
#include "guiutils.h"
#include "settings.h"
#include "settings_core.h"

Q_GLOBAL_STATIC_WITH_ARGS(QPixmap, busyPixmap, (QIcon::fromTheme(QLatin1String("okular")).pixmap(48)))

#define TEXTANNOTATION_ICONSIZE 24

inline QPen buildPen(const Okular::Annotation *ann, double width, const QColor &color)
{
    QColor c = color;
    c.setAlphaF(ann->style().opacity());
    QPen p(QBrush(c), width, ann->style().lineStyle() == Okular::Annotation::Dashed ? Qt::DashLine : Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
    return p;
}

void PagePainter::paintPageOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, int flags, int scaledWidth, int scaledHeight, const QRect limits)
{
    paintCroppedPageOnPainter(destPainter, page, observer, flags, scaledWidth, scaledHeight, limits, Okular::NormalizedRect(0, 0, 1, 1), nullptr);
}

void PagePainter::paintCroppedPageOnPainter(QPainter *destPainter,
                                            const Okular::Page *page,
                                            Okular::DocumentObserver *observer,
                                            int flags,
                                            int scaledWidth,
                                            int scaledHeight,
                                            const QRect limits,
                                            const Okular::NormalizedRect &crop,
                                            Okular::NormalizedPoint *viewPortPoint)
{
    qreal dpr = destPainter->device()->devicePixelRatioF();

    QSize scaledSize(scaledWidth, scaledHeight);

    /* Calculate the cropped geometry of the page */
    QRect scaledCrop = crop.geometry(scaledWidth, scaledHeight);

    /* variables prefixed with d are in the device pixels coordinate system, which translates to the rendered output - that means,
     * multiplied with the device pixel ratio of the target PaintDevice */
    const QRect dScaledCrop(QRectF(scaledCrop.x() * dpr, scaledCrop.y() * dpr, scaledCrop.width() * dpr, scaledCrop.height() * dpr).toAlignedRect());

    int croppedWidth = scaledCrop.width();
    int croppedHeight = scaledCrop.height();

    int dScaledWidth = ceil(scaledWidth * dpr);
    int dScaledHeight = ceil(scaledHeight * dpr);
    const QRect dLimits(QRectF(limits.x() * dpr, limits.y() * dpr, limits.width() * dpr, limits.height() * dpr).toAlignedRect());

    QColor paperColor = Qt::white;
    QColor backgroundColor = paperColor;
    if (Okular::SettingsCore::changeColors()) {
        switch (Okular::SettingsCore::renderMode()) {
        case Okular::SettingsCore::EnumRenderMode::Inverted:
        case Okular::SettingsCore::EnumRenderMode::InvertLightness:
        case Okular::SettingsCore::EnumRenderMode::InvertLuma:
        case Okular::SettingsCore::EnumRenderMode::InvertLumaSymmetric:
            backgroundColor = Qt::black;
            break;
        case Okular::SettingsCore::EnumRenderMode::Paper:
            paperColor = Okular::SettingsCore::paperColor();
            backgroundColor = paperColor;
            break;
        case Okular::SettingsCore::EnumRenderMode::Recolor:
            backgroundColor = Okular::Settings::recolorBackground();
            break;
        default:;
        }
    }
    destPainter->fillRect(limits, backgroundColor);

    const bool hasTilesManager = page->hasTilesManager(observer);
    QPixmap pixmap;

    if (!hasTilesManager) {
        /** 1 - RETRIEVE THE 'PAGE+ID' PIXMAP OR A SIMILAR 'PAGE' ONE **/
        const QPixmap *p = page->_o_nearestPixmap(observer, dScaledWidth, dScaledHeight);

        if (p != nullptr) {
            pixmap = *p;
        }

        /** 1B - IF NO PIXMAP, DRAW EMPTY PAGE **/
        double pixmapRescaleRatio = !pixmap.isNull() ? dScaledWidth / (double)pixmap.width() : -1;
        long pixmapPixels = !pixmap.isNull() ? (long)pixmap.width() * (long)pixmap.height() : 0;
        if (pixmap.isNull() || pixmapRescaleRatio > 20.0 || pixmapRescaleRatio < 0.25 || (dScaledWidth > pixmap.width() && pixmapPixels > 60000000L)) {
            // draw something on the blank page: the okular icon or a cross (as a fallback)
            if (!busyPixmap()->isNull()) {
                busyPixmap->setDevicePixelRatio(dpr);
                destPainter->drawPixmap(QPoint(10, 10), *busyPixmap());
            } else {
                destPainter->setPen(Qt::gray);
                destPainter->drawLine(0, 0, croppedWidth - 1, croppedHeight - 1);
                destPainter->drawLine(0, croppedHeight - 1, croppedWidth - 1, 0);
            }
            return;
        }
    }

    /** 2 - FIND OUT WHAT TO PAINT (Flags + Configuration + Presence) **/
    bool canDrawHighlights = (flags & Highlights) && !page->m_highlights.isEmpty();
    bool canDrawTextSelection = (flags & TextSelection) && page->textSelection();
    bool canDrawAnnotations = (flags & Annotations) && !page->m_annotations.isEmpty();
    bool enhanceLinks = (flags & EnhanceLinks) && Okular::Settings::highlightLinks();
    bool enhanceImages = (flags & EnhanceImages) && Okular::Settings::highlightImages();

    // vectors containing objects to draw
    // make this a qcolor, rect map, since we don't need
    // to know s_id here! we are only drawing this right?
    QList<QPair<QColor, Okular::NormalizedRect>> *drawnHighlights = nullptr;
    QList<Okular::Annotation *> *drawnAnnotations = nullptr;
    Okular::Annotation *boundingRectOnlyAnn = nullptr; // Paint the bounding rect of this annotation
    // fill up lists with visible annotation/highlight objects/text selections
    if (canDrawHighlights || canDrawTextSelection || canDrawAnnotations) {
        // precalc normalized 'limits rect' for intersection
        double nXMin = ((double)limits.left() / scaledWidth) + crop.left, nXMax = ((double)limits.right() / scaledWidth) + crop.left, nYMin = ((double)limits.top() / scaledHeight) + crop.top,
               nYMax = ((double)limits.bottom() / scaledHeight) + crop.top;
        // append all highlights inside limits to their list
        if (canDrawHighlights) {
            if (!drawnHighlights) {
                drawnHighlights = new QList<QPair<QColor, Okular::NormalizedRect>>();
            }
            /*            else
                        {*/

            Okular::NormalizedRect *limitRect = new Okular::NormalizedRect(nXMin, nYMin, nXMax, nYMax);
            Okular::HighlightAreaRect::const_iterator hIt;
            for (const Okular::HighlightAreaRect *highlight : page->m_highlights) {
                for (hIt = highlight->constBegin(); hIt != highlight->constEnd(); ++hIt) {
                    if ((*hIt).intersects(limitRect)) {
                        drawnHighlights->append(qMakePair(highlight->color, *hIt));
                    }
                }
            }
            delete limitRect;
            //}
        }
        if (canDrawTextSelection) {
            if (!drawnHighlights) {
                drawnHighlights = new QList<QPair<QColor, Okular::NormalizedRect>>();
            }
            /*            else
                        {*/
            Okular::NormalizedRect *limitRect = new Okular::NormalizedRect(nXMin, nYMin, nXMax, nYMax);
            const Okular::RegularAreaRect *textSelection = page->textSelection();
            Okular::HighlightAreaRect::const_iterator hIt = textSelection->constBegin(), hEnd = textSelection->constEnd();
            for (; hIt != hEnd; ++hIt) {
                if ((*hIt).intersects(limitRect)) {
                    drawnHighlights->append(qMakePair(page->textSelectionColor(), *hIt));
                }
            }
            delete limitRect;
            //}
        }
        // append annotations inside limits to the list
        if (canDrawAnnotations) {
            for (Okular::Annotation *ann : page->m_annotations) {
                int flags = ann->flags();

                if (flags & Okular::Annotation::Hidden) {
                    continue;
                }

                if (flags & Okular::Annotation::ExternallyDrawn) {
                    // ExternallyDrawn annots are never rendered by PagePainter.
                    // Just paint the boundingRect if the annot is moved or resized.
                    if (flags & (Okular::Annotation::BeingMoved | Okular::Annotation::BeingResized)) {
                        boundingRectOnlyAnn = ann;
                    }
                    continue;
                }

                bool intersects = ann->transformedBoundingRectangle().intersects(nXMin, nYMin, nXMax, nYMax);
                if (ann->subType() == Okular::Annotation::AText) {
                    Okular::TextAnnotation *ta = static_cast<Okular::TextAnnotation *>(ann);
                    if (ta->textType() == Okular::TextAnnotation::Linked) {
                        Okular::NormalizedRect iconrect(ann->transformedBoundingRectangle().left,
                                                        ann->transformedBoundingRectangle().top,
                                                        ann->transformedBoundingRectangle().left + TEXTANNOTATION_ICONSIZE / page->width(),
                                                        ann->transformedBoundingRectangle().top + TEXTANNOTATION_ICONSIZE / page->height());
                        intersects = iconrect.intersects(nXMin, nYMin, nXMax, nYMax);
                    }
                }
                if (intersects) {
                    if (!drawnAnnotations) {
                        drawnAnnotations = new QList<Okular::Annotation *>();
                    }
                    drawnAnnotations->append(ann);
                }
            }
        }
        // end of intersections checking
    }

    QRect limitsInPixmap = limits.translated(scaledCrop.topLeft());
    QRect dLimitsInPixmap = dLimits.translated(dScaledCrop.topLeft());

    // limits within full (scaled but uncropped) pixmap

    /** 4. PAINT PIXMAP NORMAL OR RESCALED USING GIVEN QPAINTER **/
    if (hasTilesManager) {
        const Okular::NormalizedRect normalizedLimits(limitsInPixmap, scaledWidth, scaledHeight);
        const QList<Okular::Tile> tiles = page->tilesAt(observer, normalizedLimits);
        QList<Okular::Tile>::const_iterator tIt = tiles.constBegin(), tEnd = tiles.constEnd();
        while (tIt != tEnd) {
            const Okular::Tile &tile = *tIt;
            QRectF tileRect = tile.rect().geometryF(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft());
            QRect dTileRect = tile.rect().geometry(dScaledWidth, dScaledHeight).translated(-dScaledCrop.topLeft());
            QRectF limitsInTile = QRectF(limits) & tileRect;
            QRect dLimitsInTile = dLimits & dTileRect;

            if (!limitsInTile.isEmpty()) {
                QPixmap *tilePixmap = tile.pixmap();

                if (tilePixmap->width() == dTileRect.width() && tilePixmap->height() == dTileRect.height()) {
                    destPainter->drawPixmap(limitsInTile, *tilePixmap, dLimitsInTile.translated(-dTileRect.topLeft()));
                } else {
                    destPainter->drawPixmap(tileRect, *tilePixmap, tilePixmap->rect());
                }
            }
            tIt++;
        }
    } else {
        destPainter->drawPixmap(limits, pixmap.scaled(dScaledWidth, dScaledHeight), dLimitsInPixmap);
    }

    /** 5. PAINT ANNOTATIONS **/
    if (drawnAnnotations) {
        double pageScale = (double)croppedWidth / page->width();

        QList<Okular::Annotation *>::const_iterator aIt = drawnAnnotations->constBegin(), aEnd = drawnAnnotations->constEnd();
        for (; aIt != aEnd; ++aIt) {
            Okular::Annotation *a = *aIt;
            Okular::Annotation::SubType type = a->subType();
            QColor acolor = a->style().color();
            if (!acolor.isValid()) {
                acolor = Qt::yellow;
            }
            // honor accessibility recoloring settings
            acolor = Okular::Recolor::applyCurrentRecolorModeToColor(acolor);
            // skip the annotation drawing if all the annotation is fully
            // transparent, but not with text annotations
            if (opacity <= 0 && a->subType() != Okular::Annotation::AText) {
                continue;
            }
            acolor.setAlpha(opacity);

            // draw LineAnnotation MISSING: caption, dash pattern, endings for multipoint lines
            if (type == Okular::Annotation::ALine) {
                double xScale = (double)scaledWidth / (double)page->width(), yScale = (double)scaledHeight / (double)page->height();
                LineAnnotPainter linepainter {(Okular::LineAnnotation *)a, {page->width(), page->height()}, pageScale, {xScale, 0., 0., yScale, 0., 0.}};
                linepainter.draw(*destPainter);
            }
            // draw HighlightAnnotation MISSING: under/strike width, feather, capping
            else if (type == Okular::Annotation::AHighlight) {
                // get the annotation
                Okular::HighlightAnnotation *ha = (Okular::HighlightAnnotation *)a;
                Okular::HighlightAnnotation::HighlightType type = ha->highlightType();

                RasterOperation multOp = (backgroundColor == Qt::black) ? Screen : Multiply;

                // draw each quad of the annotation
                int quads = ha->highlightQuads().size();
                for (int q = 0; q < quads; q++) {
                    NormalizedPath path;
                    const Okular::HighlightAnnotation::Quad &quad = ha->highlightQuads()[q];
                    // translate page points to window
                    for (int i = 0; i < 4; i++) {
                        path.append(quad.transformedPoint(i));
                    }
                    // draw the normalized path into image
                    switch (type) {
                    // highlight the whole rect
                    case Okular::HighlightAnnotation::Highlight:
                        drawShapeOnPainter(*mixedPainter, scaledSize, path, true, Qt::NoPen, acolor, pageScale, multOp);
                        break;
                    // highlight the bottom part of the rect
                    case Okular::HighlightAnnotation::Squiggly:
                        path[3].x = (path[0].x + path[3].x) / 2.0;
                        path[3].y = (path[0].y + path[3].y) / 2.0;
                        path[2].x = (path[1].x + path[2].x) / 2.0;
                        path[2].y = (path[1].y + path[2].y) / 2.0;
                        drawShapeOnPainter(*mixedPainter, scaledSize, path, true, Qt::NoPen, acolor, pageScale, multOp);
                        break;
                    // make a line at 3/4 of the height
                    case Okular::HighlightAnnotation::Underline:
                        path[0].x = (3 * path[0].x + path[3].x) / 4.0;
                        path[0].y = (3 * path[0].y + path[3].y) / 4.0;
                        path[1].x = (3 * path[1].x + path[2].x) / 4.0;
                        path[1].y = (3 * path[1].y + path[2].y) / 4.0;
                        path.pop_back();
                        path.pop_back();
                        drawShapeOnPainter(*destPainter, scaledSize, path, false, QPen(acolor, 2), QBrush(), pageScale);
                        break;
                    // make a line at 1/2 of the height
                    case Okular::HighlightAnnotation::StrikeOut:
                        path[0].x = (path[0].x + path[3].x) / 2.0;
                        path[0].y = (path[0].y + path[3].y) / 2.0;
                        path[1].x = (path[1].x + path[2].x) / 2.0;
                        path[1].y = (path[1].y + path[2].y) / 2.0;
                        path.pop_back();
                        path.pop_back();
                        drawShapeOnPainter(*destPainter, scaledSize, path, false, QPen(acolor, 2), QBrush(), pageScale);
                        break;
                    }
                }
            }
            // draw InkAnnotation MISSING:invar width, PENTRACER
            else if (type == Okular::Annotation::AInk) {
                // get the annotation
                Okular::InkAnnotation *ia = (Okular::InkAnnotation *)a;

                // draw each ink path
                const QList<QList<Okular::NormalizedPoint>> transformedInkPaths = ia->transformedInkPaths();

                const QPen inkPen = buildPen(a, a->style().width(), acolor);

                for (const QList<Okular::NormalizedPoint> &inkPath : transformedInkPaths) {
                    // normalize page point to image
                    NormalizedPath path;
                    for (const Okular::NormalizedPoint &inkPoint : inkPath) {
                        path.append(inkPoint);
                    }
                    // draw the normalized path into image
                    drawShapeOnPainter(*destPainter, scaledSize, path, false, inkPen, QBrush(), pageScale);
                }
            }
        // 6.3. viewport point -- for "Show cursor position in Viewer" in Kile

            // Annotation boundary in destPainter coordinates:
            QRect annotBoundary = a->transformedBoundingRectangle().geometry(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft());
            QRect dAnnotBoundary = a->transformedBoundingRectangle().geometry(dScaledWidth, dScaledHeight).translated(-dScaledCrop.topLeft());
            QRect annotRect = annotBoundary.intersected(limits);
            // Visible portion of the annotation at annotBoundary size:
            QRect innerRect = annotRect.translated(-annotBoundary.topLeft());
            QRectF dInnerRect(innerRect.x() * dpr, innerRect.y() * dpr, innerRect.width() * dpr, innerRect.height() * dpr);

            // draw TextAnnotation
            if (type == Okular::Annotation::AText) {
                Okular::TextAnnotation *text = (Okular::TextAnnotation *)a;
                if (text->textType() == Okular::TextAnnotation::InPlace) {
                    QImage image(dAnnotBoundary.size(), QImage::Format_ARGB32);
                    image.setDevicePixelRatio(dpr);
                    image.fill(acolor.rgba());
                    QPainter painter(&image);
                    painter.setFont(text->textFont());
                    painter.setPen(Okular::Recolor::applyCurrentRecolorModeToColor(text->textColor()));
                    Qt::AlignmentFlag halign = (text->inplaceAlignment() == 1 ? Qt::AlignHCenter : (text->inplaceAlignment() == 2 ? Qt::AlignRight : Qt::AlignLeft));
                    const double invXScale = (double)page->width() / scaledWidth;
                    const double invYScale = (double)page->height() / scaledHeight;
                    const double borderWidth = text->style().width();
                    painter.scale(1 / invXScale, 1 / invYScale);
                    painter.drawText(
                        borderWidth * invXScale, borderWidth * invYScale, (image.width() - 2 * borderWidth) * invXScale, (image.height() - 2 * borderWidth) * invYScale, Qt::AlignTop | halign | Qt::TextWordWrap, text->contents());
                    painter.resetTransform();
                    // Required as asking for a zero width pen results
                    // in a default width pen (1.0) being created
                    if (borderWidth != 0) {
                        QPen pen(Okular::Recolor::applyCurrentRecolorModeToColor(Qt::black), borderWidth);
                        painter.setPen(pen);
                        painter.drawRect(0, 0, image.width() - 1, image.height() - 1);
                    }
                    painter.end();

                    destPainter->drawImage(annotBoundary.topLeft(), image);
                } else if (text->textType() == Okular::TextAnnotation::Linked) {
                    // get pixmap, colorize and alpha-blend it
                    QPixmap pixmap = QIcon::fromTheme(text->textIcon().toLower()).pixmap(32);

                    QPixmap scaledCroppedPixmap = pixmap.scaled(TEXTANNOTATION_ICONSIZE * dpr, TEXTANNOTATION_ICONSIZE * dpr).copy(dInnerRect.toAlignedRect());
                    scaledCroppedPixmap.setDevicePixelRatio(dpr);
                    QImage scaledCroppedImage = scaledCroppedPixmap.toImage();

                    // if the annotation color is valid (ie it was set), then
                    // use it to colorize the icon, otherwise the icon will be
                    // "gray"
                    if (a->style().color().isValid()) {
                        GuiUtils::colorizeImage(scaledCroppedImage, acolor, opacity);
                    }
                    pixmap = QPixmap::fromImage(scaledCroppedImage);

                    // draw the mangled image to painter
                    destPainter->drawPixmap(annotRect.topLeft(), pixmap);
                }

            }
            // draw StampAnnotation
            else if (type == Okular::Annotation::AStamp) {
                Okular::StampAnnotation *stamp = (Okular::StampAnnotation *)a;

                // get pixmap and alpha blend it if needed
                // The performance of doing it like this (re-rendering the svg every frame) is terrible,
                // but painted annotations happen rarely enough that it's fine
                // Sometimes this needs to be qMin, other times it needs to be qMax. Really, loadStamp should take both a width and a height.
                QPixmap pixmap = Okular::AnnotationUtils::loadStamp(stamp->stampIconName(), qMax(annotBoundary.width(), annotBoundary.height()) * dpr);
                if (!pixmap.isNull()) // should never happen but can happen on huge sizes
                {
                    if (Okular::Recolor::settingEnabled()) {
                        QImage annotImg = pixmap.toImage();
                        Okular::Recolor::applyCurrentRecolorModeToImage(&annotImg);
                        pixmap = QPixmap::fromImage(annotImg);
                    }

                    // Draw pixmap with opacity:
                    destPainter->save();
                    destPainter->setOpacity(destPainter->opacity() * opacity / 255.0);

                    qreal xscale = pixmap.width() / (annotBoundary.width() * dpr);
                    qreal yscale = pixmap.height() / (annotBoundary.height() * dpr);
                    QRect dAreaInPixmap(dInnerRect.left() * xscale, dInnerRect.top() * yscale, dInnerRect.width() * xscale, dInnerRect.height() * yscale);
                    destPainter->drawPixmap(annotRect, pixmap, dAreaInPixmap);

                    destPainter->restore();
                }
            }
            // draw GeomAnnotation
            else if (type == Okular::Annotation::AGeom) {
                Okular::GeomAnnotation *geom = (Okular::GeomAnnotation *)a;
                // check whether there's anything to draw
                if (geom->style().width() || geom->geometricalInnerColor().isValid()) {
                    destPainter->save();
                    const double width = geom->style().width() * Okular::Utils::realDpi(nullptr).width() / (72.0 * 2.0) * scaledWidth / page->width();
                    QRectF r(.0, .0, annotBoundary.width(), annotBoundary.height());
                    r.adjust(width, width, -width, -width);
                    r.translate(annotBoundary.topLeft());
                    if (geom->geometricalInnerColor().isValid()) {
                        r.adjust(width, width, -width, -width);
                        const QColor color = Okular::Recolor::applyCurrentRecolorModeToColor(geom->geometricalInnerColor());
                        destPainter->setPen(Qt::NoPen);
                        destPainter->setBrush(QColor(color.red(), color.green(), color.blue(), opacity));
                        if (geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare) {
                            destPainter->drawRect(r);
                        } else {
                            destPainter->drawEllipse(r);
                        }
                        r.adjust(-width, -width, width, width);
                    }
                    if (geom->style().width()) // need to check the original size here..
                    {
                        destPainter->setPen(buildPen(a, width * 2, acolor));
                        destPainter->setBrush(Qt::NoBrush);
                        if (geom->geometricalType() == Okular::GeomAnnotation::InscribedSquare) {
                            destPainter->drawRect(r);
                        } else {
                            destPainter->drawEllipse(r);
                        }
                    }
                    destPainter->restore();
                }
            }

            // draw extents rectangle
            if (Okular::Settings::debugDrawAnnotationRect()) {
                destPainter->setPen(acolor);
                destPainter->drawRect(annotBoundary);
            }
        }
    }

    if (boundingRectOnlyAnn) {
        QRect annotBoundary = boundingRectOnlyAnn->transformedBoundingRectangle().geometry(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft());
        destPainter->setPen(Qt::DashLine);
        destPainter->drawRect(annotBoundary);
    }

    /** 6. Misc. things to draw **/
    // 6.1. link/image borders
    if (enhanceLinks || enhanceImages) {
        destPainter->save();
        destPainter->scale(scaledWidth, scaledHeight);
        destPainter->translate(-crop.left, -crop.top);

        QColor normalColor = QApplication::palette().color(QPalette::Active, QPalette::Highlight);
        // enlarging limits for intersection is like growing the 'rectGeometry' below
        QRect limitsEnlarged = limits;
        limitsEnlarged.adjust(-2, -2, 2, 2);
        // draw rects that are inside the 'limits' paint region as opaque rects
        for (Okular::ObjectRect *rect : page->m_rects) {
            if ((enhanceLinks && rect->objectType() == Okular::ObjectRect::Action) || (enhanceImages && rect->objectType() == Okular::ObjectRect::Image)) {
                if (limitsEnlarged.intersects(rect->boundingRect(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft()))) {
                    destPainter->strokePath(rect->region(), QPen(normalColor, 0));
                }
            }
        }
        destPainter->restore();
    }

    // 6.2. text selection highlights
    // NOT highlight annotations (drawn above) or rectangle/table selections (drawn by pageView)
    if (drawnHighlights) {
        // draw highlights that are inside the 'limits' paint region
        destPainter->save();
        for (const auto &highlight : qAsConst(*drawnHighlights)) {
            const Okular::NormalizedRect &r = highlight.second;
            // find out the rect to highlight on pixmap
            QRect highlightRect = r.geometry(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft()).intersected(limits);

            QColor highlightColor = highlight.first;

            if (backgroundColor == Qt::black) {
                destPainter->setCompositionMode(QPainter::CompositionMode_SourceOver);
                highlightColor.setAlpha(100);
            } else
                destPainter->setCompositionMode(QPainter::CompositionMode_Multiply);

            destPainter->fillRect(highlightRect, highlightColor);

            auto frameColor = highlightColor.darker(150);
            const QRect frameRect = r.geometry(scaledWidth, scaledHeight).translated(-scaledCrop.topLeft());
            destPainter->setPen(frameColor);
            destPainter->drawRect(frameRect);
        }
        destPainter->restore();
    }

    // 6.3. viewport point -- for "showing source location", syncTeX maybe? I'm not sure
    if (viewPortPoint) {
        destPainter->setPen(QApplication::palette().color(QPalette::Active, QPalette::Highlight));
        destPainter->drawLine(0, viewPortPoint->y * scaledHeight + 1, scaledWidth - 1, viewPortPoint->y * scaledHeight + 1);
        // ROTATION CURRENTLY NOT IMPLEMENTED
        /*
                    if( page->rotation() == Okular::Rotation0)
                    {

                    }
                    else if(page->rotation() == Okular::Rotation270)
                    {
                        painter.drawLine( viewPortPoint->y * scaledHeight + 1, 0, viewPortPoint->y * scaledHeight + 1, scaledWidth - 1);
                    }
                    else if(page->rotation() == Okular::Rotation180)
                    {
                        painter.drawLine( 0, (1.0 - viewPortPoint->y) * scaledHeight - 1, scaledWidth - 1, (1.0 - viewPortPoint->y) * scaledHeight - 1 );
                    }
                    else if(page->rotation() == Okular::Rotation90) // not right, rotation clock-wise
                    {
                        painter.drawLine( scaledWidth - (viewPortPoint->y * scaledHeight + 1), 0, scaledWidth - (viewPortPoint->y * scaledHeight + 1), scaledWidth - 1);
                    }
        */
    }

    // delete object containers
    delete drawnHighlights;
    delete drawnAnnotations;
}

void PagePainter::drawShapeOnPainter(QPainter &painter, QSizeF imageSize, const NormalizedPath &normPath, bool closeShape, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op
                                     // float antiAliasRadius
)
{
    // safety checks
    int pointsNumber = normPath.size();
    if (pointsNumber < 2) {
        return;
    }

    painter.save();

    qreal fImageWidth = imageSize.width(), fImageHeight = imageSize.height();

    // stroke outline
    double penWidth = (double)pen.width() * penWidthMultiplier;
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen2 = pen;
    pen2.setWidthF(penWidth);
    painter.setPen(pen2);
    painter.setBrush(brush);

    switch (op) {
    case Normal:
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        break;
    case Multiply:
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        break;
    case Screen:
        painter.setCompositionMode(QPainter::CompositionMode_Screen);
        break;
    }

    if (brush.style() == Qt::NoBrush) {
        // create a polygon
        QPolygonF poly(closeShape ? pointsNumber + 1 : pointsNumber);
        for (int i = 0; i < pointsNumber; ++i) {
            poly[i] = QPointF(normPath[i].x * fImageWidth, normPath[i].y * fImageHeight);
        }
        if (closeShape) {
            poly[pointsNumber] = poly[0];
        }

        painter.drawPolyline(poly);
    } else {
        // create a 'path'
        QPainterPath path;
        path.setFillRule(Qt::WindingFill);

        path.moveTo(normPath[0].x * fImageWidth, normPath[0].y * fImageHeight);
        for (int i = 1; i < pointsNumber; i++) {
            path.lineTo(normPath[i].x * fImageWidth, normPath[i].y * fImageHeight);
        }
        if (closeShape) {
            path.closeSubpath();
        }

        painter.drawPath(path);
    }
    painter.restore();
}

void PagePainter::drawEllipseOnPainter(QPainter &painter, QSizeF imageSize, const NormalizedPath &rect, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op)
{
    painter.save();
    qreal fImageWidth = imageSize.width(), fImageHeight = imageSize.height();

    // stroke outline
    const double penWidth = (double)pen.width() * penWidthMultiplier;
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
    painter.restore();
}

LineAnnotPainter::LineAnnotPainter(const Okular::LineAnnotation *a, QSizeF pageSize, double pageScale, const QTransform &toNormalizedImage)
    : la {a}
    , pageSize {pageSize}
    , pageScale {pageScale}
    , toNormalizedImage {toNormalizedImage}
    , aspectRatio {pageSize.height() / pageSize.width()}
    , linePen {buildPen(a, a->style().width(), Okular::Recolor::applyCurrentRecolorModeToColor(a->style().color()))}
{
    if ((la->lineClosed() || la->transformedLinePoints().count() == 2) && la->lineInnerColor().isValid()) {
        fillBrush = QBrush(la->lineInnerColor());
    }
}

void LineAnnotPainter::draw(QPainter &painter) const
{
    const QList<Okular::NormalizedPoint> transformedLinePoints = la->transformedLinePoints();
    if (transformedLinePoints.count() == 2) {
        const Okular::NormalizedPoint delta {transformedLinePoints.last().x - transformedLinePoints.first().x, transformedLinePoints.first().y - transformedLinePoints.last().y};
        const double angle {atan2(delta.y * aspectRatio, delta.x)};
        const double cosA {cos(-angle)};
        const double sinA {sin(-angle)};
        const QTransform tmpMatrix = QTransform {cosA, sinA / aspectRatio, -sinA, cosA / aspectRatio, transformedLinePoints.first().x, transformedLinePoints.first().y};
        const double deaspectedY {delta.y * aspectRatio};
        const double mainSegmentLength {sqrt(delta.x * delta.x + deaspectedY * deaspectedY)};
        const double lineendSize {std::min(6. * la->style().width() / pageSize.width(), mainSegmentLength / 2.)};

        drawShortenedLine(mainSegmentLength, lineendSize, painter, tmpMatrix);
        drawLineEnds(mainSegmentLength, lineendSize, painter, tmpMatrix);
        drawLeaderLine(0., painter, tmpMatrix);
        drawLeaderLine(mainSegmentLength, painter, tmpMatrix);
    } else if (transformedLinePoints.count() > 2) {
        drawMainLine(painter);
    }
}

void LineAnnotPainter::drawMainLine(QPainter &painter) const
{
    // draw the line as normalized path into image
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(la->transformedLinePoints(), toNormalizedImage), la->lineClosed(), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawShortenedLine(double mainSegmentLength, double size, QPainter &painter, const QTransform &toNormalizedPage) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{shortenForArrow(size, la->lineStartStyle()), 0}, {mainSegmentLength - shortenForArrow(size, la->lineEndStyle()), 0}};
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), la->lineClosed(), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEnds(double mainSegmentLength, double size, QPainter &painter, const QTransform &transform) const
{
    switch (la->lineStartStyle()) {
    case Okular::LineAnnotation::Square:
        drawLineEndSquare(0, -size, transform, painter);
        break;
    case Okular::LineAnnotation::Circle:
        drawLineEndCircle(0, -size, transform, painter);
        break;
    case Okular::LineAnnotation::Diamond:
        drawLineEndDiamond(0, -size, transform, painter);
        break;
    case Okular::LineAnnotation::OpenArrow:
        drawLineEndArrow(0, -size, 1., false, transform, painter);
        break;
    case Okular::LineAnnotation::ClosedArrow:
        drawLineEndArrow(0, -size, 1., true, transform, painter);
        break;
    case Okular::LineAnnotation::None:
        break;
    case Okular::LineAnnotation::Butt:
        drawLineEndButt(0, size, transform, painter);
        break;
    case Okular::LineAnnotation::ROpenArrow:
        drawLineEndArrow(0, size, 1., false, transform, painter);
        break;
    case Okular::LineAnnotation::RClosedArrow:
        drawLineEndArrow(0, size, 1., true, transform, painter);
        break;
    case Okular::LineAnnotation::Slash:
        drawLineEndSlash(0, -size, transform, painter);
        break;
    }
    switch (la->lineEndStyle()) {
    case Okular::LineAnnotation::Square:
        drawLineEndSquare(mainSegmentLength, size, transform, painter);
        break;
    case Okular::LineAnnotation::Circle:
        drawLineEndCircle(mainSegmentLength, size, transform, painter);
        break;
    case Okular::LineAnnotation::Diamond:
        drawLineEndDiamond(mainSegmentLength, size, transform, painter);
        break;
    case Okular::LineAnnotation::OpenArrow:
        drawLineEndArrow(mainSegmentLength, size, 1., false, transform, painter);
        break;
    case Okular::LineAnnotation::ClosedArrow:
        drawLineEndArrow(mainSegmentLength, size, 1., true, transform, painter);
        break;
    case Okular::LineAnnotation::None:
        break;
    case Okular::LineAnnotation::Butt:
        drawLineEndButt(mainSegmentLength, size, transform, painter);
        break;
    case Okular::LineAnnotation::ROpenArrow:
        drawLineEndArrow(mainSegmentLength, size, -1., false, transform, painter);
        break;
    case Okular::LineAnnotation::RClosedArrow:
        drawLineEndArrow(mainSegmentLength, size, -1., true, transform, painter);
        break;
    case Okular::LineAnnotation::Slash:
        drawLineEndSlash(mainSegmentLength, size, transform, painter);
        break;
    }
}

void LineAnnotPainter::drawLineEndArrow(double xEndPos, double size, double flipX, bool close, const QTransform &toNormalizedPage, QPainter &painter) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos - size * flipX, size / 2.},
        {xEndPos, 0},
        {xEndPos - size * flipX, -size / 2.},
    };
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), close, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndButt(double xEndPos, double size, const QTransform &toNormalizedPage, QPainter &painter) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const double halfSize {size / 2.};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos, halfSize},
        {xEndPos, -halfSize},
    };
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndCircle(double xEndPos, double size, const QTransform &toNormalizedPage, QPainter &painter) const
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
    PagePainter::drawEllipseOnPainter(painter, pageSize, transformPath(path, toNormalizedImage), linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndSquare(double xEndPos, double size, const QTransform &toNormalizedPage, QPainter &painter) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{xEndPos, size / 2.}, {xEndPos - size, size / 2.}, {xEndPos - size, -size / 2.}, {xEndPos, -size / 2.}};
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndDiamond(double xEndPos, double size, const QTransform &toNormalizedPage, QPainter &painter) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const QList<Okular::NormalizedPoint> path {{xEndPos, 0}, {xEndPos - size / 2., size / 2.}, {xEndPos - size, 0}, {xEndPos - size / 2., -size / 2.}};
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLineEndSlash(double xEndPos, double size, const QTransform &toNormalizedPage, QPainter &painter) const
{
    const QTransform combinedTransform {toNormalizedPage * toNormalizedImage};
    const double halfSize {size / 2.};
    const double xOffset {cos(M_PI / 3.) * halfSize};
    const QList<Okular::NormalizedPoint> path {
        {xEndPos - xOffset, halfSize},
        {xEndPos + xOffset, -halfSize},
    };
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), true, linePen, fillBrush, pageScale);
}

void LineAnnotPainter::drawLeaderLine(double xEndPos, QPainter &painter, const QTransform &toNormalizedPage) const
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
    PagePainter::drawShapeOnPainter(painter, pageSize, transformPath(path, combinedTransform), false, linePen, fillBrush, pageScale);
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
