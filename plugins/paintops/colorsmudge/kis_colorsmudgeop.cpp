/*
 *  SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_colorsmudgeop.h"

#include <cmath>
#include <memory>
#include <QRect>

#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoMixColorsOp.h>
#include <KoCompositeOpRegistry.h>

#include <kis_brush.h>
#include <kis_global.h>
#include <kis_paint_device.h>
#include <kis_painter.h>
#include <kis_iterator_ng.h>
#include <kis_image.h>
#include <kis_selection.h>
#include <kis_brush_based_paintop_settings.h>
#include <kis_cross_device_color_sampler.h>
#include <kis_fixed_paint_device.h>
#include <kis_lod_transform.h>
#include <kis_spacing_information.h>
#include <KoColorModelStandardIds.h>
#include "kis_paintop_plugin_utils.h"


struct ColorSmudgeStrategy {
    ColorSmudgeStrategy(KisPrecisePaintDeviceWrapper &srcWrapper,
                        KisPainter *painter,
                        KisPaintDeviceSP dstDevice,
                        KoColor paintColor,
                        bool smearAlpha)
        : m_maskDab(new KisFixedPaintDevice(KoColorSpaceRegistry::instance()->alpha8()))
        , m_origDab(new KisFixedPaintDevice(KoColorSpaceRegistry::instance()->rgb8()))
        , m_tempDevice(new KisFixedPaintDevice(dstDevice->colorSpace()))
        , m_heigtmapDevice(new KisPaintDevice(KoColorSpaceRegistry::instance()->rgb8()))
        , m_colorDevice(new KisPaintDevice(dstDevice->colorSpace()))
        , m_srcWrapper(srcWrapper)
        , m_smearPainter(m_colorDevice)
        , m_colorRatePainter(m_colorDevice)
        , m_finalPainter(dstDevice)
        , m_paintColor(paintColor)
    {
        m_smearPainter.setCompositeOp(smearAlpha ? COMPOSITE_COPY : COMPOSITE_OVER);
        m_colorRatePainter.setCompositeOp(painter->compositeOp()->id());
        m_paintColor.convertTo(dstDevice->colorSpace());
        m_colorDevice->makeCloneFrom(dstDevice, dstDevice->extent());

        m_finalPainter.setCompositeOp(COMPOSITE_COPY);
        m_finalPainter.setSelection(painter->selection());
        m_finalPainter.setChannelFlags(painter->channelFlags());
        m_finalPainter.copyMirrorInformationFrom(painter);
    }

    void updateMask(KisDabCache *dabCache,
                    const KisPaintInformation& info,
                    const KisDabShape &shape,
                    const QPointF &cursorPoint,
                    QRect *dstDabRect) {

        m_origDab = dabCache->fetchImageDab(m_origDab->colorSpace(),
            cursorPoint,
            shape,
            info,
            1.0,
            dstDabRect);

        m_maskDab->setRect(m_origDab->bounds());
        m_maskDab->lazyGrowBufferWithoutInitialization();
        int numPixels = m_maskDab->bounds().width() * m_maskDab->bounds().height();
        m_origDab->colorSpace()->copyOpacityU8(m_origDab->data(), m_maskDab->data(), numPixels);
    }

    void paintDab(const QRect &srcRect, const QRect &dstRect,
                  const QPoint &canvasLocalSamplePoint,
                  qreal opacity,
                  qreal colorRateValue,
                  qreal smudgeRateValue,
                  qreal lightnessStrengthValue) {

        const quint8 colorAlpha = qRound(colorRateValue * colorRateValue * opacity * 255.0);
        const quint8 smearAlpha = qRound(smudgeRateValue * opacity * 255.0);
        const quint8 dullingAlpha = qRound(smudgeRateValue * 0.8 * opacity * 255.0);

        static int i = -1; i++;

        m_tempDevice->setRect(dstRect);
        m_tempDevice->lazyGrowBufferWithoutInitialization();

        m_srcWrapper.readRect(srcRect);
        //m_srcWrapper.preciseDevice()->readBytes(m_tempDevice->data(), srcRect);

        const int nPixels = m_tempDevice->bounds().width() * m_tempDevice->bounds().height();

//        m_tempDevice->convertToQImage(0).save(QString("dd_%1_00_smear.png").arg(i));


//        m_tempDevice->colorSpace()->
//            modulateLightnessByGrayBrush(m_tempDevice->data(),
//                                         reinterpret_cast<const QRgb*>(m_origDab->data()),
//                                         m_tempDevice->data(),
//                                         lightnessStrengthValue,
//                                         nPixels);
//        m_tempDevice->convertToQImage(0).save(QString("dd_%1_01_mapped.png").arg(i));
//        // m_tempDevice->modulateByLightness()

        m_smearPainter.setOpacity(smearAlpha);
       // m_smearPainter.bltFixed(dstRect.topLeft(), m_tempDevice, m_tempDevice->bounds());
        m_smearPainter.bitBlt(dstRect.topLeft(), m_colorDevice, srcRect);
        //m_tempDevice->fill(dstRect, m_paintColor);

//        m_tempDevice->colorSpace()->
//            modulateLightnessByGrayBrush(m_tempDevice->data(),
//                                         reinterpret_cast<const QRgb*>(m_origDab->data()),
//                                         m_tempDevice->data(),
//                                         lightnessStrengthValue,
//                                         nPixels);

//        m_origDab->convertToQImage(0).save(QString("dd_%1_00_dab.png").arg(i));


        // m_tempDevice->modulateByLightness()

        m_colorRatePainter.setOpacity(colorAlpha);
        //m_colorRatePainter.bltFixed(dstRect.topLeft(), m_tempDevice, m_tempDevice->bounds());
        m_colorRatePainter.fill(dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(), m_paintColor);

        //m_finalPainter.bitBlt(dstRect)

    }

    bool needsDstInitialized() const {
        return false;
    }

    KisFixedPaintDeviceSP finalSelectionDevice() const {
        return m_maskDab;
    }

    KisFixedPaintDeviceSP m_maskDab;
    KisFixedPaintDeviceSP m_origDab;
    KisFixedPaintDeviceSP m_tempDevice;
    KisPaintDeviceSP m_heigtmapDevice;
    KisPaintDeviceSP m_colorDevice;
    KisPrecisePaintDeviceWrapper &m_srcWrapper;
    KisPainter m_smearPainter;
    KisPainter m_colorRatePainter;
    KisPainter m_finalPainter;
    KisPaintDeviceSP m_dstDevice;
    KoColor m_paintColor;
};

KisColorSmudgeOp::KisColorSmudgeOp(const KisPaintOpSettingsSP settings, KisPainter* painter, KisNodeSP node, KisImageSP image)
    : KisBrushBasedPaintOp(settings, painter)
    , m_firstRun(true)
    , m_image(image)
    , m_precisePainterWrapper(painter->device())
    , m_tempDev(m_precisePainterWrapper.createPreciseCompositionSourceDevice())
    , m_backgroundPainter(new KisPainter(m_tempDev))
    , m_smudgePainter(new KisPainter(m_tempDev))
    , m_overlayPainter(new KisPainter(m_tempDev))
    , m_colorRatePainter(new KisPainter(m_tempDev))
    , m_finalPainter(new KisPainter(m_precisePainterWrapper.preciseDevice()))
    , m_smudgeRateOption()
    , m_colorRateOption("ColorRate", KisPaintOpOption::GENERAL, false)
    , m_smudgeRadiusOption()
    , m_maskDab(new KisFixedPaintDevice(KoColorSpaceRegistry::instance()->alpha8()))
    , m_origDab(new KisFixedPaintDevice(m_tempDev->colorSpace()))
{
    Q_UNUSED(node);

    Q_ASSERT(settings);
    Q_ASSERT(painter);
    m_sizeOption.readOptionSetting(settings);
    m_opacityOption.readOptionSetting(settings);
    m_ratioOption.readOptionSetting(settings);
    m_spacingOption.readOptionSetting(settings);
    m_rateOption.readOptionSetting(settings);
    m_lightnessStrengthOption.readOptionSetting(settings);
    m_smudgeRateOption.readOptionSetting(settings);
    m_colorRateOption.readOptionSetting(settings);
    m_smudgeRadiusOption.readOptionSetting(settings);
    m_overlayModeOption.readOptionSetting(settings);
    m_rotationOption.readOptionSetting(settings);
    m_scatterOption.readOptionSetting(settings);
    m_gradientOption.readOptionSetting(settings);
    m_airbrushOption.readOptionSetting(settings);

    m_sizeOption.resetAllSensors();
    m_opacityOption.resetAllSensors();
    m_ratioOption.resetAllSensors();
    m_spacingOption.resetAllSensors();
    m_rateOption.resetAllSensors();
    m_lightnessStrengthOption.resetAllSensors();
    m_smudgeRateOption.resetAllSensors();
    m_colorRateOption.resetAllSensors();
    m_smudgeRadiusOption.resetAllSensors();
    m_rotationOption.resetAllSensors();
    m_scatterOption.resetAllSensors();
    m_gradientOption.resetAllSensors();

    m_gradient = painter->gradient();

    m_backgroundPainter->setCompositeOp(COMPOSITE_COPY);
    // Smudge Painter works in default COMPOSITE_OVER mode
    m_colorRatePainter->setCompositeOp(painter->compositeOp()->id());
    if (m_brush->brushApplication() != ALPHAMASK) { //Forces this, in case the widget somehow doesn't
        m_useNewEngine = true;
    } else {
        m_useNewEngine = m_smudgeRateOption.getUseNewEngine();
    }

    if (1 || m_useNewEngine){
        m_finalPainter->setCompositeOp(COMPOSITE_COPY);
        m_smudgePainter->setCompositeOp(m_smudgeRateOption.getSmearAlpha() ? COMPOSITE_COPY : COMPOSITE_OVER);
    } else {
        m_finalPainter->setCompositeOp(m_smudgeRateOption.getSmearAlpha() ? COMPOSITE_COPY : COMPOSITE_OVER);
        m_smudgePainter->setCompositeOp(COMPOSITE_OVER);
    }

    m_finalPainter->setSelection(painter->selection());
    m_finalPainter->setChannelFlags(painter->channelFlags());
    m_finalPainter->copyMirrorInformationFrom(painter);

    m_paintColor = painter->paintColor().convertedTo(m_tempDev->colorSpace());
    m_preciseColorRateCompositeOp =
        m_tempDev->colorSpace()->compositeOp(m_colorRatePainter->compositeOp()->id());

    m_hsvOptions.append(KisPressureHSVOption::createHueOption());
    m_hsvOptions.append(KisPressureHSVOption::createSaturationOption());
    m_hsvOptions.append(KisPressureHSVOption::createValueOption());

    Q_FOREACH (KisPressureHSVOption * option, m_hsvOptions) {
        option->readOptionSetting(settings);
        option->resetAllSensors();
        if (option->isChecked() && !m_hsvTransform) {
            m_hsvTransform = m_paintColor.colorSpace()->createColorTransformation("hsv_adjustment", QHash<QString, QVariant>());
        }
    }
    m_rotationOption.applyFanCornersInfo(this);

    if (m_overlayModeOption.isChecked() && m_image && m_image->projection()){
        m_preciseImageDeviceWrapper.reset(new KisPrecisePaintDeviceWrapper(m_image->projection()));
    }


    m_strategy.reset(new ColorSmudgeStrategy(m_precisePainterWrapper,
                                             painter,
                                             m_tempDev,
                                             m_paintColor,
                                             m_smudgeRateOption.getSmearAlpha()));
}

KisColorSmudgeOp::~KisColorSmudgeOp()
{
    qDeleteAll(m_hsvOptions);
    delete m_hsvTransform;
}


void KisColorSmudgeOp::updateMask(const KisPaintInformation& info, const KisDabShape &shape, const QPointF &cursorPoint)
{
    qreal lightnessStrength = m_lightnessStrengthOption.apply(info);

    if (!m_useNewEngine) {
        static const KoColorSpace* cs = KoColorSpaceRegistry::instance()->alpha8();
        static KoColor color(Qt::black, cs);

        m_maskDab = m_dabCache->fetchDab(cs,
            color,
            cursorPoint,
            shape,
            info,
            1.0,
            &m_dstDabRect);
    }
    else {
        KoColor color = m_paintColor;
        m_gradientOption.apply(color, m_gradient, info);
        if (m_hsvTransform) {
            Q_FOREACH(KisPressureHSVOption * option, m_hsvOptions) {
                option->apply(m_hsvTransform, info);
            }
            m_hsvTransform->transform(color.data(), color.data(), 1);
        }

        m_origDab = m_dabCache->fetchDab(m_tempDev->colorSpace(),
            color,
            cursorPoint,
            shape,
            info,
            1.0,
            &m_dstDabRect,
            lightnessStrength);

        m_maskDab->setRect(m_origDab->bounds());
        m_maskDab->initialize();
        int numPixels = m_maskDab->bounds().width() * m_maskDab->bounds().height();
        m_origDab->colorSpace()->copyOpacityU8(m_origDab->data(), m_maskDab->data(), numPixels);
    }


    // sanity check
    KIS_ASSERT_RECOVER_NOOP(m_dstDabRect.size() == m_maskDab->bounds().size());
}


inline void KisColorSmudgeOp::getTopLeftAligned(const QPointF &pos, const QPointF &hotSpot, qint32 *x, qint32 *y)
{
    QPointF topLeft = pos - hotSpot;

    qreal xFraction, yFraction; // will not be used
    splitCoordinate(topLeft.x(), x, &xFraction);
    splitCoordinate(topLeft.y(), y, &yFraction);
}

KoColor KisColorSmudgeOp::getOverlayDullingFillColor(QPoint canvasLocalSamplePoint) {
    // stored in the color space of the paintColor
    KoColor dullingFillColor = m_paintColor;
    // get the pixel on the canvas that lies beneath the hot spot
    // of the dab and fill  the temporary paint device with that color
    KisCrossDeviceColorPickerInt colorPicker(m_tempDev, dullingFillColor);
    colorPicker.pickColor(canvasLocalSamplePoint.x(), canvasLocalSamplePoint.y(), dullingFillColor.data());
    KIS_SAFE_ASSERT_RECOVER_NOOP(*dullingFillColor.colorSpace() == *m_tempDev->colorSpace());

    return dullingFillColor;
}

KoColor KisColorSmudgeOp::getDullingFillColor(const KisPaintInformation& info, KisPrecisePaintDeviceWrapper& activeWrapper, QPoint canvasLocalSamplePoint) {
    // stored in the color space of the paintColor
    KoColor dullingFillColor = m_paintColor;
    if (m_smudgeRadiusOption.isChecked()) {
        const qreal effectiveSize = 0.5 * (m_dstDabRect.width() + m_dstDabRect.height());

        const QRect sampleRect = m_smudgeRadiusOption.sampleRect(info, effectiveSize, canvasLocalSamplePoint);
        activeWrapper.readRect(sampleRect);

        m_smudgeRadiusOption.apply(&dullingFillColor, info, effectiveSize, canvasLocalSamplePoint.x(), canvasLocalSamplePoint.y(), activeWrapper.preciseDevice());
        KIS_SAFE_ASSERT_RECOVER_NOOP(*dullingFillColor.colorSpace() == *m_tempDev->colorSpace());
    }
    else {
        // get the pixel on the canvas that lies beneath the hot spot
        // of the dab and fill  the temporary paint device with that color
        activeWrapper.readRect(QRect(canvasLocalSamplePoint, QSize(1, 1)));
        KisCrossDeviceColorSamplerInt colorPicker(activeWrapper.preciseDevice(), dullingFillColor);
        colorPicker.sampleColor(canvasLocalSamplePoint.x(), canvasLocalSamplePoint.y(), dullingFillColor.data());
        KIS_SAFE_ASSERT_RECOVER_NOOP(*dullingFillColor.colorSpace() == *m_tempDev->colorSpace());
    }

    return dullingFillColor;
}

void KisColorSmudgeOp::mixSmudgePaintAt(const KisPaintInformation& info, KisPrecisePaintDeviceWrapper& activeWrapper, QRect srcDabRect, QPoint canvasLocalSamplePoint, bool useDullingMode)
{
    KisBrushSP brush = m_brush;

    QRect dabBounds = m_origDab->bounds();
    qint32 width = dabBounds.width();
    qint32 height = dabBounds.height();

    const KoColorSpace* preciseCS = m_tempDev->colorSpace();

    const qreal colorRate = m_colorRateOption.isChecked() ? m_colorRateOption.computeSizeLikeValue(info) : 0.0;
    const qreal smudgeLength = m_smudgeRateOption.isChecked() ? m_smudgeRateOption.computeSizeLikeValue(info) : 1.0;
    const qreal fpOpacity = m_opacityOption.getOpacityf(info);

    const int colorAlpha = qRound(colorRate * colorRate * fpOpacity * 255.0);
    const int smudgeAlpha = qRound(smudgeLength * fpOpacity * 255.0);
    const int dullingAlpha = qRound(smudgeLength * 0.8 * fpOpacity * 255.0);

    m_precisePainterWrapper.readRect(m_dstDabRect); //copy the current data in the destination
    m_backgroundPainter->bitBlt(QPoint(), m_precisePainterWrapper.preciseDevice(), m_dstDabRect);


    activeWrapper.readRect(srcDabRect);
    if (m_image && m_overlayModeOption.isChecked()) {
        m_smudgePainter->setOpacity(smudgeAlpha);
        m_image->blockUpdates();
        m_smudgePainter->bitBlt(QPoint(), m_image->projection(), srcDabRect);
        m_image->unblockUpdates();
        m_overlayPainter->setOpacity(smudgeAlpha);
        m_overlayPainter->bitBlt(QPoint(), activeWrapper.preciseDevice(), srcDabRect); //necessary because image->projection doesn't update each frame

        if (useDullingMode) {
            KoColor dullingFillColor = m_smudgeRadiusOption.isChecked() ?
                getDullingFillColor(info, activeWrapper, canvasLocalSamplePoint) : 
                getOverlayDullingFillColor(canvasLocalSamplePoint - srcDabRect.topLeft());
            dullingFillColor.convertTo(preciseCS); //convert to mix with background
            m_smudgePainter->setOpacity(dullingAlpha);
            m_smudgePainter->fill(0, 0, m_dstDabRect.width(), m_dstDabRect.height(), dullingFillColor);
        }

    } else {
        if (useDullingMode) {
            KoColor dullingFillColor = getDullingFillColor(info, activeWrapper, canvasLocalSamplePoint);
            dullingFillColor.convertTo(preciseCS); //convert to mix with background
            m_smudgePainter->setOpacity(dullingAlpha);
            m_smudgePainter->fill(0, 0, m_dstDabRect.width(), m_dstDabRect.height(), dullingFillColor);
        } else {
            m_smudgePainter->setOpacity(smudgeAlpha);
            m_smudgePainter->bitBlt(QPoint(), activeWrapper.preciseDevice(), srcDabRect);
        }
    }

    m_colorRatePainter->setOpacity(colorAlpha);
    m_colorRatePainter->bltFixed(0, 0, m_origDab, 0, 0, width, height);

    m_precisePainterWrapper.readRects(m_finalPainter->calculateAllMirroredRects(m_dstDabRect));

    // then blit the temporary painting device on the canvas at the current brush position
    // the alpha mask (maskDab) will be used here to only blit the pixels that are in the area (shape) of the brush

    m_finalPainter->bitBltWithFixedSelection(m_dstDabRect.x(), m_dstDabRect.y(), m_tempDev, m_maskDab, m_dstDabRect.width(), m_dstDabRect.height());
    m_finalPainter->renderMirrorMaskSafe(m_dstDabRect, m_tempDev, 0, 0, m_maskDab, !m_dabCache->needSeparateOriginal());

    const QVector<QRect> dirtyRects = m_finalPainter->takeDirtyRegion();
    m_precisePainterWrapper.writeRects(dirtyRects);
    painter()->addDirtyRects(dirtyRects);

}

KisSpacingInformation KisColorSmudgeOp::paintAt(const KisPaintInformation& info)
{
    KisBrushSP brush = m_brush;
    const bool useDullingMode = m_smudgeRateOption.getMode() == KisSmudgeOption::DULLING_MODE;

    /* This is a fix for dulling + overlay + paint,
     * this should allow the image to composite paint addition effects correctly
     * while also respecting overlay mode. */
    bool useAlternatePrecisionSource = (m_overlayModeOption.isChecked() &&
                                        useDullingMode &&
                                        m_preciseImageDeviceWrapper!= nullptr);

    KisPrecisePaintDeviceWrapper &activeWrapper = useAlternatePrecisionSource ? *m_preciseImageDeviceWrapper :
                                                                                 m_precisePainterWrapper;

    // Simple error catching
    if (!painter()->device() || !brush || !brush->canPaintFor(info)) {
        return KisSpacingInformation(1.0);
    }

    if (m_smudgeRateOption.getMode() == KisSmudgeOption::SMEARING_MODE) {
        /**
        * Disable handling of the subpixel precision. In the smudge op we
        * should read from the aligned areas of the image, so having
        * additional internal offsets, created by the subpixel precision,
        * will worsen the quality (at least because
        * QRectF(m_dstDabRect).center() will not point to the real center
        * of the brush anymore).
        * Of course, this only really matters with smearing_mode (bug:327235),
        * and you only notice the lack of subpixel precision in the dulling methods.
        */
        m_dabCache->disableSubpixelPrecision();
    }

#if 0
    //if precision
    KoColor colorSpaceChanger = painter()->paintColor();
    const KoColorSpace* preciseColorSpace = colorSpaceChanger.colorSpace();
    if (colorSpaceChanger.colorSpace()->colorDepthId().id() == "U8") {
    preciseColorSpace = KoColorSpaceRegistry::instance()->colorSpace(colorSpaceChanger.colorSpace()->colorModelId().id(), "U16", colorSpaceChanger.profile() );
        colorSpaceChanger.convertTo(preciseColorSpace);
    }
    painter()->setPaintColor(colorSpaceChanger);
#endif

    // get the scaling factor calculated by the size option
    qreal scale = m_sizeOption.apply(info);
    scale *= KisLodTransform::lodToScale(painter()->device());
    qreal rotation = m_rotationOption.apply(info);
    if (checkSizeTooSmall(scale)) return KisSpacingInformation();

    qreal ratio = m_ratioOption.apply(info);

    KisDabShape shape(scale, ratio, rotation);

    QPointF scatteredPos =
        m_scatterOption.apply(info,
                              brush->maskWidth(shape, 0, 0, info),
                              brush->maskHeight(shape, 0, 0, info));

    QPointF hotSpot = brush->hotSpot(shape, info);

    {
        m_strategy->updateMask(m_dabCache, info, shape, scatteredPos, &m_dstDabRect);

        QPointF newCenterPos = QRectF(m_dstDabRect).center();
        /**
         * Save the center of the current dab to know where to read the
         * data during the next pass. We do not save scatteredPos here,
         * because it may differ slightly from the real center of the
         * brush (due to rounding effects), which will result in a
         * really weird quality.
         */
        QRect srcDabRect = m_dstDabRect.translated((m_lastPaintPos - newCenterPos).toPoint());

        m_lastPaintPos = newCenterPos;


        KisSpacingInformation spacingInfo =
            effectiveSpacing(scale, rotation,
                             &m_airbrushOption, &m_spacingOption, info);

        if (m_firstRun) {
            m_firstRun = false;
            return spacingInfo;
        }

        QPoint canvasLocalSamplePoint = (srcDabRect.topLeft() + hotSpot).toPoint();

        const qreal colorRate = m_colorRateOption.isChecked() ? m_colorRateOption.computeSizeLikeValue(info) : 0.0;
        const qreal smudgeLength = m_smudgeRateOption.isChecked() ? m_smudgeRateOption.computeSizeLikeValue(info) : 1.0;
        const qreal fpOpacity = m_opacityOption.getOpacityf(info);
        const qreal lightnessStrength = m_lightnessStrengthOption.apply(info);

        m_strategy->paintDab(srcDabRect, m_dstDabRect,
                            canvasLocalSamplePoint,
                            fpOpacity, colorRate, smudgeLength, lightnessStrength);

        const QVector<QRect> dirtyRects = m_finalPainter->takeDirtyRegion();
        m_precisePainterWrapper.writeRects(dirtyRects);
        painter()->addDirtyRects(dirtyRects);

        return spacingInfo;
    }

    {
        m_strategy->updateMask(m_dabCache, info, shape, scatteredPos, &m_dstDabRect);

        QPointF newCenterPos = QRectF(m_dstDabRect).center();
        /**
         * Save the center of the current dab to know where to read the
         * data during the next pass. We do not save scatteredPos here,
         * because it may differ slightly from the real center of the
         * brush (due to rounding effects), which will result in a
         * really weird quality.
         */
        QRect srcDabRect = m_dstDabRect.translated((m_lastPaintPos - newCenterPos).toPoint());

        m_lastPaintPos = newCenterPos;


        KisSpacingInformation spacingInfo =
            effectiveSpacing(scale, rotation,
                             &m_airbrushOption, &m_spacingOption, info);

        if (m_firstRun) {
            m_firstRun = false;
            return spacingInfo;
        }

        QPoint canvasLocalSamplePoint = (srcDabRect.topLeft() + hotSpot).toPoint();

        const qreal colorRate = m_colorRateOption.isChecked() ? m_colorRateOption.computeSizeLikeValue(info) : 0.0;
        const qreal smudgeLength = m_smudgeRateOption.isChecked() ? m_smudgeRateOption.computeSizeLikeValue(info) : 1.0;
        const qreal fpOpacity = m_opacityOption.getOpacityf(info);
        const qreal lightnessStrength = m_lightnessStrengthOption.apply(info);

        m_precisePainterWrapper.readRects(m_finalPainter->calculateAllMirroredRects(m_dstDabRect));

        const QRect dstRectAtTempDev(QPoint(), m_dstDabRect.size());

        if (m_strategy->needsDstInitialized()) {
            KisPainter::copyAreaOptimized(dstRectAtTempDev.topLeft(),
                                          m_precisePainterWrapper.preciseDevice(),
                                          m_tempDev,
                                          m_dstDabRect);
        }


        m_strategy->paintDab(srcDabRect, dstRectAtTempDev,
                            canvasLocalSamplePoint,
                            fpOpacity, colorRate, smudgeLength, lightnessStrength);

        KisFixedPaintDeviceSP finalSelection = m_strategy->finalSelectionDevice();

        m_finalPainter->bitBltWithFixedSelection(m_dstDabRect.x(), m_dstDabRect.y(), m_tempDev, finalSelection, m_dstDabRect.width(), m_dstDabRect.height());
        m_finalPainter->renderMirrorMaskSafe(m_dstDabRect, m_tempDev, 0, 0, m_maskDab, !m_dabCache->needSeparateOriginal());

        const QVector<QRect> dirtyRects = m_finalPainter->takeDirtyRegion();
        m_precisePainterWrapper.writeRects(dirtyRects);
        painter()->addDirtyRects(dirtyRects);

        return spacingInfo;
    }











    /**
     * Update the brush mask.
     *
     * Upon leaving the function:
     *   o m_maskDab stores the new mask
     *   o m_maskBounds stores the extents of the mask paint device
     *   o m_dstDabRect stores the destination rect where the mask is going
     *     to be written to
     */
    updateMask(info, shape, scatteredPos);

    QPointF newCenterPos = QRectF(m_dstDabRect).center();
    /**
     * Save the center of the current dab to know where to read the
     * data during the next pass. We do not save scatteredPos here,
     * because it may differ slightly from the real center of the
     * brush (due to rounding effects), which will result in a
     * really weird quality.
     */
    QRect srcDabRect = m_dstDabRect.translated((m_lastPaintPos - newCenterPos).toPoint());

    m_lastPaintPos = newCenterPos;

    KisSpacingInformation spacingInfo =
        effectiveSpacing(scale, rotation,
                         &m_airbrushOption, &m_spacingOption, info);

    if (m_firstRun) {
        m_firstRun = false;
        return spacingInfo;
    }

    QPoint canvasLocalSamplePoint = (srcDabRect.topLeft() + hotSpot).toPoint();

    if (m_useNewEngine) {
        mixSmudgePaintAt(info, activeWrapper, srcDabRect, canvasLocalSamplePoint, useDullingMode);
        return spacingInfo;
    }

    const qreal fpOpacity = (qreal(painter()->opacity()) / 255.0) * m_opacityOption.getOpacityf(info);

    if (m_image && m_overlayModeOption.isChecked()) {
        m_image->blockUpdates();
        m_backgroundPainter->bitBlt(QPoint(), m_image->projection(), srcDabRect);
        m_image->unblockUpdates();
    }
    else {
        // IMPORTANT: Clear the temporary painting device to transparent black.
        //            It will only clear the extents of the brush.
        m_tempDev->clear(QRect(QPoint(), m_dstDabRect.size()));
    }

    // stored in the color space of the paintColor
    KoColor dullingFillColor = m_paintColor;

    if (!useDullingMode) {
        activeWrapper.readRect(srcDabRect);
        m_smudgePainter->bitBlt(QPoint(), activeWrapper.preciseDevice(), srcDabRect);
    } else {
        dullingFillColor = getDullingFillColor(info, activeWrapper, canvasLocalSamplePoint);
    }

    // if the user selected the color smudge option,
    // we will mix some color into the temporary painting device (m_tempDev)
    if (m_colorRateOption.isChecked()) {
        // this will apply the opacity (selected by the user) to copyPainter
        // (but fit the rate inbetween the range 0.0 to (1.0-SmudgeRate))
        qreal maxColorRate = qMax<qreal>(1.0 - m_smudgeRateOption.getRate(), 0.2);
        m_colorRateOption.apply(*m_colorRatePainter, info, 0.0, maxColorRate, fpOpacity);

        // paint a rectangle with the current color (foreground color)
        // or a gradient color (if enabled)
        // into the temporary painting device and use the user selected
        // composite mode
        KoColor color = m_paintColor;
        m_gradientOption.apply(color, m_gradient, info);
        if (m_hsvTransform) {
            Q_FOREACH (KisPressureHSVOption * option, m_hsvOptions) {
                option->apply(m_hsvTransform, info);
            }
            m_hsvTransform->transform(color.data(), color.data(), 1);
        }

        if (!useDullingMode) {
            KIS_SAFE_ASSERT_RECOVER(*m_colorRatePainter->device()->colorSpace() == *color.colorSpace()) {
                color.convertTo(m_colorRatePainter->device()->colorSpace());
            }

            m_colorRatePainter->fill(0, 0, m_dstDabRect.width(), m_dstDabRect.height(), color);
        } else {
            KIS_SAFE_ASSERT_RECOVER(*dullingFillColor.colorSpace() == *color.colorSpace()) {
                color.convertTo(dullingFillColor.colorSpace());
            }
            KIS_SAFE_ASSERT_RECOVER_NOOP(*dullingFillColor.colorSpace() == *m_tempDev->colorSpace());
            m_preciseColorRateCompositeOp->composite(dullingFillColor.data(), 0,
                                                     color.data(), 0,
                                                     0, 0,
                                                     1, 1,
                                                     m_colorRatePainter->opacity());
        }
    }

    if (useDullingMode) {
        KIS_SAFE_ASSERT_RECOVER_NOOP(*dullingFillColor.colorSpace() == *m_tempDev->colorSpace());
        m_tempDev->fill(QRect(0, 0, m_dstDabRect.width(), m_dstDabRect.height()), dullingFillColor);
    }

    m_precisePainterWrapper.readRects(m_finalPainter->calculateAllMirroredRects(m_dstDabRect));

    // if color is disabled (only smudge) and "overlay mode" is enabled
    // then first blit the region under the brush from the image projection
    // to the painting device to prevent a rapid build up of alpha value
    // if the color to be smudged is semi transparent.
    if (m_image && m_overlayModeOption.isChecked() && !m_colorRateOption.isChecked()) {
        m_finalPainter->setOpacity(OPACITY_OPAQUE_U8);
        m_image->blockUpdates();
        // TODO: check if this code is correct in mirrored mode! Technically, the
        //       painter renders the mirrored dab only, so we should also prepare
        //       the overlay for it in all the places.
        m_finalPainter->bitBlt(m_dstDabRect.topLeft(), m_image->projection(), m_dstDabRect);
        m_image->unblockUpdates();
    }

    // set opacity calculated by the rate option
    m_smudgeRateOption.apply(*m_finalPainter, info, 0.0, 1.0, fpOpacity);

    // then blit the temporary painting device on the canvas at the current brush position
    // the alpha mask (maskDab) will be used here to only blit the pixels that are in the area (shape) of the brush
    m_finalPainter->bitBltWithFixedSelection(m_dstDabRect.x(), m_dstDabRect.y(), m_tempDev, m_maskDab, m_dstDabRect.width(), m_dstDabRect.height());
    m_finalPainter->renderMirrorMaskSafe(m_dstDabRect, m_tempDev, 0, 0, m_maskDab, !m_dabCache->needSeparateOriginal());

    const QVector<QRect> dirtyRects = m_finalPainter->takeDirtyRegion();
    m_precisePainterWrapper.writeRects(dirtyRects);
    painter()->addDirtyRects(dirtyRects);

    return spacingInfo;

}

KisSpacingInformation KisColorSmudgeOp::updateSpacingImpl(const KisPaintInformation &info) const
{
    const qreal scale = m_sizeOption.apply(info) * KisLodTransform::lodToScale(painter()->device());
    const qreal rotation = m_rotationOption.apply(info);
    return effectiveSpacing(scale, rotation, &m_airbrushOption, &m_spacingOption, info);
}


KisTimingInformation KisColorSmudgeOp::updateTimingImpl(const KisPaintInformation &info) const
{
    return KisPaintOpPluginUtils::effectiveTiming(&m_airbrushOption, &m_rateOption, info);
}
