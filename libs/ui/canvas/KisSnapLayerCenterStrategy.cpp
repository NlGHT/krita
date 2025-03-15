/*
 *  SPDX-FileCopyrightText: 2019 Kuntal Majumder <hellozee@disroot.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSnapLayerCenterStrategy.h"

#include <QPainterPath>
#include <qglobal.h>
#include "KisViewManager.h"
#include <kis_layer_utils.h>
#include "kis_global.h"
#include "kis_canvas2.h"
#include "KoSnapProxy.h"
#include "kis_group_layer.h"
#include "kis_node.h"
#include "kis_base_node.h"
#include "kis_types.h"
#include "libs/k_types.h"

KisSnapLayerCenterStrategy::KisSnapLayerCenterStrategy(KoSnapGuide::Strategy type):
    KoSnapStrategy(type)
{
}

KisSnapLayerCenterStrategy::~KisSnapLayerCenterStrategy()
{
}

bool KisSnapLayerCenterStrategy::snap(const QPointF &mousePosition, KoSnapProxy *proxy, qreal maxSnapDistance)
{
    Q_UNUSED(maxSnapDistance);
    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2*>(proxy->canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(canvas2, false);
    KisImageWSP currentImage = canvas2->currentImage();
    KisNodeList *currentlyProcessingNodes = proxy->currentlyProcessingNodes();
    if (currentlyProcessingNodes && currentlyProcessingNodes->count()) {
        qWarning() << "Currently processing nodes count: " << currentlyProcessingNodes->count();
    }

    // root.data()->firstChild
    // auto root = currentImage->rootLayer();
    QList<KisNodeSP> layers;
    const bool hasGroupLayers =
            KisLayerUtils::recursiveFindNode(currentImage->root(),
                [&layers] (KisNodeSP node) {
                    // layers.append(node);
                    auto name = node->name();
                    QRect bounds = node->exactBounds();
                    QPoint center = bounds.center();
                    layers.append(node);
                    qWarning() << "Name: " << name << "x: " << center.x() << "y: " << center.y();
                    return false;
                });

    

    // qWarning() << "YO YO YOOOOOOO";

    const QPointF imagePos = canvas2->coordinatesConverter()->documentToImage(mousePosition);
    const QPointF alignedDocPoint = canvas2->coordinatesConverter()->imageToDocument(imagePos.toPoint());
    setSnappedPosition(alignedDocPoint, ToPoint);

    return true;
}

QPainterPath KisSnapLayerCenterStrategy::decoration(const KoViewConverter &converter) const
{
    QSizeF unzoomedSize = converter.viewToDocument(QSizeF(5, 5));
    QPainterPath decoration;
    decoration.moveTo(snappedPosition() - QPointF(unzoomedSize.width(), 0));
    decoration.lineTo(snappedPosition() + QPointF(unzoomedSize.width(), 0));
    decoration.moveTo(snappedPosition() - QPointF(0, unzoomedSize.height()));
    decoration.lineTo(snappedPosition() + QPointF(0, unzoomedSize.height()));
    return decoration;
}
