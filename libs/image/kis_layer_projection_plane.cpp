/*
 *  SPDX-FileCopyrightText: 2015 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_layer_projection_plane.h"

#include <QBitArray>
#include <KoColorSpace.h>
#include <KoChannelInfo.h>
#include <KoCompositeOpRegistry.h>
#include "kis_painter.h"
#include "kis_projection_leaf.h"
#include "kis_cached_paint_device.h"
#include "kis_sequential_iterator.h"


struct KisLayerProjectionPlane::Private
{
    KisLayer *layer;
    KisCachedPaintDevice cachedDevice;
};


KisLayerProjectionPlane::KisLayerProjectionPlane(KisLayer *layer)
    : m_d(new Private)
{
    m_d->layer = layer;
}

KisLayerProjectionPlane::~KisLayerProjectionPlane()
{
}

QRect KisLayerProjectionPlane::recalculate(const QRect& rect, KisNodeSP filthyNode, KisRenderPassFlags flags)
{
    return m_d->layer->updateProjection(rect, filthyNode, flags);
}

void KisLayerProjectionPlane::applyImpl(KisPainter *painter, const QRect &rect, KritaUtils::ThresholdMode thresholdMode)
{
    KisPaintDeviceSP device = m_d->layer->projection();
    if (!device) return;

    QRect needRect = rect;

    if (m_d->layer->compositeOpId() != COMPOSITE_COPY &&
        m_d->layer->compositeOpId() != COMPOSITE_DESTINATION_IN  &&
        m_d->layer->compositeOpId() != COMPOSITE_DESTINATION_ATOP) {

        needRect &= device->extent();
    }

    if(needRect.isEmpty()) return;

    const QBitArray channelFlags = m_d->layer->projectionLeaf()->channelFlags();

    QScopedPointer<KisCachedPaintDevice::Guard> d1;

    if (thresholdMode != KritaUtils::ThresholdNone) {
        d1.reset(new KisCachedPaintDevice::Guard(device, m_d->cachedDevice));
        KisPaintDeviceSP tmp = d1->device();
        tmp->makeCloneFromRough(device, needRect);

        KritaUtils::thresholdOpacity(tmp, needRect, thresholdMode);

        device = tmp;
    }

    painter->setChannelFlags(channelFlags);
    painter->setCompositeOpId(m_d->layer->compositeOpId());
    painter->setOpacityU8(m_d->layer->projectionLeaf()->opacity());
    painter->bitBlt(needRect.topLeft(), device, needRect);
}

void KisLayerProjectionPlane::apply(KisPainter *painter, const QRect &rect)
{
    applyImpl(painter, rect, KritaUtils::ThresholdNone);
}

void KisLayerProjectionPlane::applyMaxOutAlpha(KisPainter *painter, const QRect &rect, KritaUtils::ThresholdMode thresholdMode)
{
    applyImpl(painter, rect, thresholdMode);
}

KisPaintDeviceList KisLayerProjectionPlane::getLodCapableDevices() const
{
    return KisPaintDeviceList() << m_d->layer->projection();
}

QRect KisLayerProjectionPlane::needRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->needRect(rect, pos);
}

QRect KisLayerProjectionPlane::changeRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->changeRect(rect, pos);
}

QRect KisLayerProjectionPlane::accessRect(const QRect &rect, KisLayer::PositionToFilthy pos) const
{
    return m_d->layer->accessRect(rect, pos);
}

QRect KisLayerProjectionPlane::needRectForOriginal(const QRect &rect) const
{
    return m_d->layer->needRectForOriginal(rect);
}

QRect KisLayerProjectionPlane::tightUserVisibleBounds() const
{
    return m_d->layer->tightUserVisibleBounds();
}

QRect KisLayerProjectionPlane::looseUserVisibleBounds() const
{
    return m_d->layer->looseUserVisibleBounds();
}

