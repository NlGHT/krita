/*
 *  SPDX-FileCopyrightText: 2019 Kuntal Majumder <hellozee@disroot.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __KIS_SNAP_LAYER_CENTER_STRATEGY_H
#define __KIS_SNAP_LAYER_CENTER_STRATEGY_H

#include <QScopedPointer>

#include "KoSnapStrategy.h"


class KisSnapLayerCenterStrategy : public KoSnapStrategy
{
public:
    KisSnapLayerCenterStrategy(KoSnapGuide::Strategy type = KoSnapGuide::LayerCenterSnapping);
    ~KisSnapLayerCenterStrategy() override;

    bool snap(const QPointF &mousePosition, KoSnapProxy * proxy, qreal maxSnapDistance) override;
    QPainterPath decoration(const KoViewConverter &converter) const override;
};

#endif /* __KIS_SNAP_LAYER_CENTER_STRATEGY_H */
