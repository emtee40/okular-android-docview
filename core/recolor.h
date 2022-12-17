#ifndef OKULAR_RECOLOR_H
#define OKULAR_RECOLOR_H
#include "okularcore_export.h"

#include <QImage>

namespace Okular {

class OKULARCORE_EXPORT Recolor
{
public:
    /**
     * Collapse color space (from white to black) to a line from @p foreground to @p background.
     */
    static void paperColor(QImage *image, const QColor &foreground, const QColor &background);
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
     * Shifts hue of each pixel by 120 degrees, by simply swapping channels.
     */
    static void hueShiftPositive(QImage *image);
    /**
     * Shifts hue of each pixel by 240 degrees, by simply swapping channels.
     */
    static void hueShiftNegative(QImage *image);
private:
    /**
     * Inverts luma of a pixel given in @p R, @p G, @p B,
     * using the luma coefficients @p Y_R, @p Y_G, @p Y_B (should sum up to 1),
     * and assuming linear 8bit RGB color space.
     */
    static void invertLumaPixel(uchar &R, uchar &G, uchar &B, float Y_R, float Y_G, float Y_B);
};

#endif // OKULAR_RECOLOR_H

}; // namespace Okular
