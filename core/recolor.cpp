#include "recolor.h"
#include "core/debug_p.h"

using namespace Okular;

void Recolor::paperColor(QImage *image, const QColor &foreground, const QColor &background)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularCoreDebug) << "Wrong image format! Converting...";
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

void Recolor::blackWhite(QImage *image, int contrast, int threshold)
{
    unsigned int *data = reinterpret_cast<unsigned int *>(image->bits());
    int con = contrast;
    int thr = 255 - threshold;

    int pixels = image->width() * image->height();
    for (int i = 0; i < pixels; ++i) {
        // Piecewise linear function of val, through (0, 0), (thr, 128), (255, 255)
        int val = qGray(data[i]);
        if (val > thr) {
            val = 128 + (127 * (val - thr)) / (255 - thr);
        } else if (val < thr) {
            val = (128 * val) / thr;
        }

        // Linear contrast stretching through (thr, thr)
        if (con > 2) {
            val = thr + (val - thr) * con / 2;
            val = qBound(0, val, 255);
        }

        const unsigned a = qAlpha(data[i]);
        data[i] = qRgba(val, val, val, a);
    }
}

void Recolor::invertLightness(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularCoreDebug) << "Wrong image format! Converting...";
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

void Recolor::invertLuma(QImage *image, float Y_R, float Y_G, float Y_B)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularCoreDebug) << "Wrong image format! Converting...";
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

inline void Recolor::invertLumaPixel(uchar &R, uchar &G, uchar &B, float Y_R, float Y_G, float Y_B)
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

void Recolor::hueShiftPositive(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularCoreDebug) << "Wrong image format! Converting...";
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

void Recolor::hueShiftNegative(QImage *image)
{
    if (image->format() != QImage::Format_ARGB32_Premultiplied) {
        qCWarning(OkularCoreDebug) << "Wrong image format! Converting...";
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
