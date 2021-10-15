/*
    SPDX-FileCopyrightText: 2005 Enrico Ros <eros.kde@email.it>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _OKULAR_PAGEPAINTER_H_
#define _OKULAR_PAGEPAINTER_H_

#include <QBrush>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QRectF>

#include "core/annotations.h"
#include "core/area.h" // for NormalizedPoint
#include "core/page.h"

class QRect;

namespace Okular
{
class DocumentObserver;
}

/**
 * @short Paints a Okular::Page to an open painter using given flags.
 */
class Q_DECL_EXPORT PagePainter // TODO Put in Okular namespace?
{
public:
    // list of flags passed to the painting function. by OR-ing those flags
    // you can decide whether or not to permit drawing of a certain feature.
    enum PagePainterFlags { Accessibility = 1, EnhanceLinks = 2, EnhanceImages = 4, Highlights = 8, TextSelection = 16, Annotations = 32, ViewPortPoint = 64 };

    /**
     * Draw @p page on @p destPainter.
     *
     * @param destPainter Page will be drawn on this painter.
     * @param page Which page do draw.
     * @param observer Request pixmaps generated for this DocumentObserver.
     * @param flags PagePainterFlags, which features to draw.
     * @param scaledWidth The requested width of uncropped page in @p destPainter coordinates.
     * @param scaledHeight The requested height of uncropped page in @p destPainter coordinates.
     * @param pageLimits Where to paint in @p destPainter coordinates. (I. e. painter crop.) Should begin at (0, 0).
     */
    static void paintPageOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, int flags, int scaledWidth, int scaledHeight, const QRect pageLimits);

    /**
     * Draw @p page on @p destPainter.
     *
     * To crop the page, adjust @p cropRect and translate @p destPainter to the top left corner of uncropped @p page.
     * Will respect the devicePixelRatioF() of @p destPainter’s paint device automatically.
     * This means the paint device should have the device pixel ratio of the output device.
     *
     * Example: paint the bottom left quarter of the page at 3x zoom at 150% hiDPI
     * (after you have requested pixmaps):
     * \code
     * QPixmap bottomLeftQuadrant(page->width() * 2.25, page->height() * 2.25);
     * bottomLeftQuadrant.setDevicePixelRatio(1.5);
     * QPainter p(bottomLeftQuadrant);
     * p.translate(0, page->height() * -1.5);
     * PagePainter::paintPageOnPainter(&p, page, this, QRectF(QPoint(0, page->height() * 1.5), QSizeF(page->width() * 1.5, page->height() * 1.5)), 3.0);
     * \endcode
     *
     * @param destPainter Page will be drawn on this painter. Coordinate system must start at top left corner of uncropped @p page.
     * @param page Which page do draw.
     * @param observer Request pixmaps generated for this DocumentObserver.
     * @param cropRect Painting area in @p destPainter coordinates. Makes sense to be aligned to device pixels.
     * // TODO We are assuming here that Page::width() is the width in pixels when pixmaps at scale 1.0 are requested.
     * @param scale The scale from Page::width() to @p destPainter coordinates. Higher values to zoom in.
     * @param viewPortPoint Which point of the page to highlight, e. g. a source location.
     */
    static void paintPageOnPainter(QPainter *destPainter,
                                   const Okular::Page *page,
                                   Okular::DocumentObserver *observer,
                                   const QRectF &cropRect,
                                   qreal scale,
                                   PagePainterFlags flags = Accessibility,
                                   const Okular::NormalizedPoint &viewPortPoint = Okular::NormalizedPoint());

    /**
     * Draw @p page on @p destPainter.
     *
     * Overload of paintPageOnPainter(), mainly for demonstration purpose.
     * For precise rendering, you should use the other function.
     * This overload has more intuitive, but ambiguous and less precise geometry parameters.
     *
     * Will respect the devicePixelRatioF() of @p destPainter’s paint device automatically.
     * This means the paint device should have the device pixel ratio of the output device.
     *
     * Example: paint the bottom left quarter of the page at 3x zoom at 150% hiDPI
     * (after you have requested pixmaps):
     * \code
     * QPixmap bottomLeftQuadrant(page->width() * 2.25, page->height() * 2.25);
     * bottomLeftQuadrant.setDevicePixelRatio(1.5);
     * QPainter p(bottomLeftQuadrant);
     * PagePainter::paintPageOnPainter(&p, page, this, Okular::NormalizedRect(0, 0.5, 0.5, 1), QRect(0, 0, page->width() * 1.5, page->height() * 1.5));
     * \endcode
     *
     * @param destPainter Page will be drawn on this painter.
     * @param page Which page do draw.
     * @param observer Request pixmaps generated for this DocumentObserver.
     * @param inputRect Which area of the page to draw.
     * @param outputRect Where to draw this area of the page in @p destPainter coordinates.
     * @param viewPortPoint Which point of the page to highlight, e. g. a source location.
     */
    static inline void paintPageOnPainter(QPainter *destPainter,
                                          const Okular::Page *page,
                                          Okular::DocumentObserver *observer,
                                          const Okular::NormalizedRect &inputRect,
                                          const QRectF &outputRect,
                                          PagePainterFlags flags = Accessibility,
                                          const Okular::NormalizedPoint &viewPortPoint = Okular::NormalizedPoint())
    {
        destPainter->save();

        // inputRect in page->width() x page->height() coordinate system:
        const QRect pageInputRect = inputRect.roundedGeometry(page->width(), page->height());

        // Calculate the scale from inputRect and outputRect width:
        const qreal scale = outputRect.width() / pageInputRect.width();

        // Move the painter’s origin to the output rect, and then to the pages’ origin:
        destPainter->translate(outputRect.topLeft());
        destPainter->translate(-pageInputRect.topLeft() * scale);

        paintPageOnPainter(destPainter, page, observer, QRectF(pageInputRect.topLeft() * scale, outputRect.size()), scale, flags, viewPortPoint);

        destPainter->restore();
    }

    /*
     * TODO: We make these arguments unambiguous:
     *  * Version A (Preferred to implement first): a QSize as crop, a qreal as scale.
     *    + The qreal can be used to request pixmaps of unambiguous scale.
     *    - Caller needs to calculate starting point.
     *  * Version B -> A: a QRect as painting area, a NormalizedPoint as starting point, a qreal as scale.
     *    + unambiguous scale
     *    - Call needs to calculate starting point.
     *  * Version C -> A: a QRect as painting area, a NormalizedRect as page area.
     */
    /**
     * Draw @p page on @p destPainter.
     *
     * @param destPainter Page will be drawn on this painter.
     * @param page Which page do draw.
     * @param observer Request pixmaps generated for this DocumentObserver.
     * @param flags PagePainterFlags, which features to draw.
     * @param scaledWidth The requested width of uncropped page in @p destPainter coordinates.
     * @param scaledHeight The requested height of uncropped page in @p destPainter coordinates.
     * @param pageLimits Where to paint in @p destPainter coordinates. (I. e. painter crop.) Should begin at (0, 0).
     * @param crop Which area of the page to paint in @p pageLimits.
     * @param viewPortPoint Which point of the page to highlight, e. g. a source location. @c nullptr to disable.
     */
    static void paintCroppedPageOnPainter(QPainter *destPainter,
                                          const Okular::Page *page,
                                          Okular::DocumentObserver *observer,
                                          int flags,
                                          int scaledWidth,
                                          int scaledHeight,
                                          const QRect pageLimits,
                                          const Okular::NormalizedRect &crop,
                                          Okular::NormalizedPoint *viewPortPoint);

private:
    enum DrawPagePixmapsResult {
        Fine = 0x0,                   ///< All required pixmaps were found in the correct resolution and final rendering state.
        NoPixmap = 0x1,               ///< No pixmap was found for this page.
        PixmapsOfIncorrectSize = 0x2, ///< Some pixmaps/tiles were scaled.
        TilesMissing = 0x4,           ///< Some tiles were not drawn because they are missing.
        PartiallyRendered = 0x8       ///< Some pixmaps/tiles were drawn in a partially rendered state.
    };

    /**
     * Fetches pixmaps from @p page and paints them on @p destPainter.
     *
     * Will respect the devicePixelRatioF() of @p destPainter’s paint device automatically.
     * This means the paint device should have the device pixel ratio of the output device.
     *
     * @param destPainter Coordinate system should start at top left of uncropped @p page.
     * @param observer Request pixmaps generated for this observer.
     * @param cropRect Where to paint.
     * @param scale The scale from Page::width() to @p destPainter coordinates. Higher values to zoom in.
     * @param accessibility (Does nothing yet.)
     */
    static DrawPagePixmapsResult drawPagePixmapsOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, const QRectF &cropRect, qreal scale, PagePainterFlags flags = Accessibility);

    /**
     * Fetches the non-tile pixmap from @p page and paints it on @p destPainter.
     *
     * Will respect the devicePixelRatioF() of @p destPainter’s paint device automatically.
     * This means the paint device should have the device pixel ratio of the output device.
     *
     * @param destPainter Coordinate system should start at top left of uncropped @p page.
     * @param observer Request pixmap generated for this observer.
     * @param dSize The size at which the pixmap shall be painted in @p destPainter device pixels.
     * @param flags (Does nothing yet.)
     */
    static DrawPagePixmapsResult drawPagePixmapOnPainter(QPainter *destPainter, const Okular::Page *page, Okular::DocumentObserver *observer, QSize dSize, PagePainterFlags flags = Accessibility);

    // BEGIN Change Colors feature
    /**
     * Collapse color space (from white to black) to a line from @p foreground to @p background.
     */
    static void recolor(QImage *image, const QColor &foreground, const QColor &background);
    /**
     * Collapse color space to a line from white to black,
     * then move from @p threshold to 128 and stretch the line by @p contrast.
     */
    static void blackWhite(QImage *image, int contrast, int threshold);
    /**
     * Invert the lightness axis of the HSL color cone.
     */
    static void invertLightness(QImage *image);
    /**
     * Inverts luma of @p image using the luma coefficients @p Y_R, @p Y_G, @p Y_B (should sum up to 1),
     * and assuming linear 8bit RGB color space.
     */
    static void invertLuma(QImage *image, float Y_R, float Y_G, float Y_B);
    /**
     * Inverts luma of a pixel given in @p R, @p G, @p B,
     * using the luma coefficients @p Y_R, @p Y_G, @p Y_B (should sum up to 1),
     * and assuming linear 8bit RGB color space.
     */
    static void invertLumaPixel(uchar &R, uchar &G, uchar &B, float Y_R, float Y_G, float Y_B);
    /**
     * Shifts hue of each pixel by 120 degrees, by simply swapping channels.
     */
    static void hueShiftPositive(QImage *image);
    /**
     * Shifts hue of each pixel by 240 degrees, by simply swapping channels.
     */
    static void hueShiftNegative(QImage *image);
    // END Change Colors feature

    // my pretty dear raster function
    typedef QList<Okular::NormalizedPoint> NormalizedPath;
    enum RasterOperation { Normal, Multiply };

    /**
     * Draw @p normPath on @p image.
     *
     * @note @p normPath needs to be normalized in respect to @p image, not to the actual page.
     */
    static void drawShapeOnImage(QImage &image, const NormalizedPath &normPath, bool closeShape, const QPen &pen, const QBrush &brush = QBrush(), double penWidthMultiplier = 1.0, RasterOperation op = Normal);

    /**
     * Draw an ellipse described by @p rect on @p image.
     *
     * @param rect Two NormalizedPoints describing the bounding rect. Need to be normalized in respect to @p image, not to the actual page.
     */
    static void drawEllipseOnImage(QImage &image, const NormalizedPath &rect, const QPen &pen, const QBrush &brush, double penWidthMultiplier, RasterOperation op = Normal);

    friend class LineAnnotPainter;
};

/**
 * @short Painting helper for a single Okular::LineAnnotation.
 */
class LineAnnotPainter
{
public:
    /**
     * @param a The annotation to paint. Accessed by draw().
     * @param pageSizeA The full size of the page on which to paint.
     * @param pageScale The scale of the page when you call draw().
     * @param toNormalizedImage How to transform normalized coordinates of @p a to normalized coordinates of your paint device. (If your paint device represents the whole page, use the unit matrix QTransform().)
     */
    LineAnnotPainter(const Okular::LineAnnotation *a, QSizeF pageSizeA, double pageScale, const QTransform &toNormalizedImage);

    /**
     * Draw the annotation on @p image.
     */
    void draw(QImage &image) const;

private:
    void drawMainLine(QImage &image) const;
    void drawShortenedLine(double mainSegmentLength, double size, QImage &image, const QTransform &toNormalizedPage) const;
    void drawLineEnds(double mainSegmentLength, double size, QImage &image, const QTransform &transform) const;
    void drawLineEndArrow(double xEndPos, double size, double flipX, bool close, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndButt(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndCircle(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndSquare(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndDiamond(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLineEndSlash(double xEndPos, double size, const QTransform &toNormalizedPage, QImage &image) const;
    void drawLeaderLine(double xEndPos, QImage &image, const QTransform &toNormalizedPage) const;
    template<typename T> QList<Okular::NormalizedPoint> transformPath(const T &path, const QTransform &transform) const
    {
        QList<Okular::NormalizedPoint> transformedPath;
        for (const Okular::NormalizedPoint &item : path) {
            Okular::NormalizedPoint p;
            transform.map(item.x, item.y, &p.x, &p.y);
            transformedPath.append(p);
        }
        return transformedPath;
    }

    static double shortenForArrow(double size, Okular::LineAnnotation::TermStyle endStyle);

private:
    const Okular::LineAnnotation *la;
    QSizeF pageSize;
    double pageScale;
    QTransform toNormalizedImage;
    double aspectRatio;
    const QPen linePen;
    QBrush fillBrush;
};

#endif
