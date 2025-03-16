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
#include <qpoint.h>
#include <qpolygon.h>
#include <qvector2d.h>
#include <qwindowdefs.h>
#include "kis_global.h"
#include "kis_canvas2.h"
#include "KoSnapProxy.h"
#include "kis_group_layer.h"
#include "kis_node.h"
#include "kis_base_node.h"
#include "kis_selection.h"
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
    // Q_UNUSED(maxSnapDistance);
    qreal minDistance = std::numeric_limits<qreal>::max();
    qWarning() << "Snap called";
    KisCanvas2 *canvas2 = dynamic_cast<KisCanvas2*>(proxy->canvas());
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(canvas2, false);
    KisImageWSP currentImage = canvas2->currentImage();
    KisNodeListSP currentlyProcessingNodes = proxy->currentlyProcessingNodes();
    QRectF closestNodeBounds;
    QPointF snappedPoint = mousePosition;
    qWarning() << "Before nullcheck";
    if (currentlyProcessingNodes) {
        qWarning() << "It's true";
        auto count = currentlyProcessingNodes->count(); // Crash here
        qWarning() << "Made it past count";
        if (count) {
            QRectF processingNodeBounds;
            QPointF processingNodeCenter;
            QPointF pointToSnap;
            qWarning() << count;
            qWarning() << "Currently processing nodes count: " << currentlyProcessingNodes->count();
            
            if (count > 1) {
                int bottom;
                int top;
                int left;
                int right;
                for (KisNodeSP currentNode : *currentlyProcessingNodes) {
                    QRect currentBounds = currentNode->exactBounds();
                    bottom = std::max(currentBounds.bottom(), bottom);
                    top = std::max(currentBounds.top(), top);
                    left = std::max(currentBounds.left(), left);
                    right = std::max(currentBounds.right(), right);
                }
                processingNodeBounds = QRectF(left, top, right-left, top-bottom);
                processingNodeCenter = processingNodeBounds.center();
            } else {
                qWarning() << "Processing node: " << currentlyProcessingNodes->first()->name();
                processingNodeBounds = currentlyProcessingNodes->first()->exactBounds();
                processingNodeCenter = processingNodeBounds.center();
            }
            
            
        
            QList<KisNodeSP> layers;
            const bool hasGroupLayers =
                    KisLayerUtils::recursiveFindNode(currentImage->root(),
                        [&layers] (KisNodeSP node) {
                            if (node->visible() && node->name() != "root" && node->name() != "decorations-wrapper-layer") {
                                qWarning() << "Visible node: " << node->name();
                                // layers.append(node);
                                auto name = node->name();
                                QRect outerNodeBounds = node->exactBounds();
                                QPoint outerNodeCenter = outerNodeBounds.center();
                                layers.append(node);
                            }
                            
                            return false;
                        });
                        
            for (KisNodeSP node : layers) {
                auto name = node->name();
                QRect outerNodeBounds = node->exactBounds();
                QPoint outerNodeCenter = outerNodeBounds.center();
                for (KisNodeSP currentlyNode : *currentlyProcessingNodes) {
                    if (currentlyNode != node) {
                        qWarning() << "NOT same as currently moved: " << node->name();
                        QRect currentNodeBounds = currentlyNode->exactBounds();
                        QPoint currentNodeCenter = currentNodeBounds.center();
                        qWarning() << "Outie coords: " << name << "x: " << outerNodeCenter.x() << "y: " << outerNodeCenter.y();
                        qWarning() << "Innie coords: " << name << "x: " << currentNodeCenter.x() << "y: " << currentNodeCenter.y();
    
                        auto distX = qAbs(currentNodeCenter.x() - outerNodeCenter.x());
                        distX = canvas2->viewConverter()->viewToDocument(QSizeF(distX, distX)).width();
                        qWarning() << "distX: " << distX;
                        qWarning() << "minDistance: " << minDistance;
                        qWarning() << "maxSnapDistance: " << maxSnapDistance;
    
                        if (distX < maxSnapDistance && distX < minDistance) {
                            qWarning() << "Within SNAPPING distance!!!";
                            minDistance = distX;
                            pointToSnap = outerNodeCenter;
                            closestNodeBounds = currentNodeBounds;
                        }
                    }
                }
                
            }

            QPointF differenceInnieToMouse = QPointF(mousePosition - canvas2->viewToDocument(closestNodeBounds.center()));
            snappedPoint = QPointF((canvas2->viewConverter()->viewToDocument(pointToSnap) + differenceInnieToMouse).x(), mousePosition.y());
        }
        
    }

    // const QPointF imagePos = canvas2->coordinatesConverter()->documentToImage(mousePosition);
    // const QPointF alignedDocPoint = canvas2->coordinatesConverter()->imageToDocument(imagePos.toPoint());
    setSnappedPosition(snappedPoint, ToPoint);

    return minDistance < std::numeric_limits<qreal>::max();
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
