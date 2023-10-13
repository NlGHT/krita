/*
 * tool_transform.cc -- Part of Krita
 *
 * SPDX-FileCopyrightText: 2004 Boudewijn Rempt (boud@valdyas.org)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "tool_transform.h"

#include <klocalizedstring.h>

#include <kis_debug.h>
#include <kpluginfactory.h>

#include <kis_global.h>
#include <kis_types.h>
#include <KoToolRegistry.h>


#include "kis_tool_transform.h"
#include "kis_transform_mask_params_factory_registry.h"
#include "kis_transform_mask_adapter.h"
#include "kis_animated_transform_parameters.h"

K_PLUGIN_FACTORY_WITH_JSON(ToolTransformFactory, "kritatooltransform.json", registerPlugin<ToolTransform>();)

namespace {

KisAnimatedTransformParamsInterfaceSP createAnimatedParams(KisDefaultBoundsBaseSP defaultBounds)
{
    return toQShared(new KisAnimatedTransformMaskParameters(defaultBounds));
}

} // namespace



ToolTransform::ToolTransform(QObject *parent, const QVariantList &)
        : QObject(parent)
{
    KoToolRegistry::instance()->add(new KisToolTransformFactory());
    KisTransformMaskParamsFactoryRegistry::instance()->setAnimatedParamsFactory(&createAnimatedParams);
    KisTransformMaskParamsFactoryRegistry::instance()->addFactory("tooltransformparams", &KisTransformMaskAdapter::fromXML);
    qRegisterMetaType<TransformTransactionProperties>("TransformTransactionProperties");
    qRegisterMetaType<ToolTransformArgs>("ToolTransformArgs");
    qRegisterMetaType<QPainterPath>("QPainterPath");
}

ToolTransform::~ToolTransform()
{
}

#include "tool_transform.moc"
