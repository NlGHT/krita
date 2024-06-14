/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SvgTextLabel.h"
#include <KoSvgTextShape.h>
#include <KoSvgTextProperties.h>
#include <KoColorBackground.h>
#include <QPainter>
#include <QImage>
#include <QQuickWindow>

struct Q_DECL_HIDDEN SvgTextLabel::Private {
    KoSvgTextShape* shape{nullptr};
    int padding = 2;
};

SvgTextLabel::SvgTextLabel(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , d(new Private)
{
    d->shape = new KoSvgTextShape();
    d->shape->setResolution(72, 72);
    setImplicitSize(10, 10);
    setOpaquePainting(true);
    setAntialiasing(true);
}

SvgTextLabel::~SvgTextLabel()
{
}

void SvgTextLabel::paint(QPainter *painter)
{
    if (!painter->isActive()) return;
    painter->save();
    painter->fillRect(0, 0, width(), height(), fillColor());
    if (d->shape->boundingRect().isEmpty()) {
        painter->restore();
        return;
    }

    QRectF bbox = d->shape->selectionBoxes(0, d->shape->posForIndex(d->shape->plainText().size())).boundingRect();
    qreal boxHeight = bbox.isEmpty()? fontSize(): bbox.height();
    bbox = bbox.isEmpty()? d->shape->boundingRect(): bbox;
    qreal scale = (height()- d->padding*2) / boxHeight;

    painter->translate(boundingRect().center());
    painter->scale(scale, scale);
    painter->translate(-bbox.center());

    qreal pixelRatio = this->window()->effectiveDevicePixelRatio();
    QRectF scaledBounds = QRectF(0, 0, this->boundingRect().width()*pixelRatio, this->boundingRect().height()*pixelRatio);

    painter->setClipRect(painter->transform().inverted().mapRect(scaledBounds));
    painter->setPen(Qt::transparent);
    d->shape->paint(*painter);

    painter->restore();
}

QStringList SvgTextLabel::fontFamilies() const
{
    return d->shape->textProperties().property(KoSvgTextProperties::FontFamiliesId).value<QStringList>();
}

int SvgTextLabel::fontWeight() const
{
    return d->shape->textProperties().property(KoSvgTextProperties::FontWeightId).toInt();
}

QFont::Style SvgTextLabel::fontStyle() const
{
    return d->shape->textProperties().property(KoSvgTextProperties::FontStyleId).value<QFont::Style>();
}

qreal SvgTextLabel::fontSize() const
{
    return d->shape->textProperties().property(KoSvgTextProperties::FontSizeId).toReal();
}

QColor SvgTextLabel::textColor() const
{
    KoColorBackground *bg = dynamic_cast<KoColorBackground*>(d->shape->background().data());
    if (bg) {
        return bg->color();
    }
    return Qt::black;
}

QStringList SvgTextLabel::openTypeFeatures() const
{
    return d->shape->textProperties().property(KoSvgTextProperties::FontFeatureSettingsId).value<QStringList>();
}

QString SvgTextLabel::text() const
{
    return d->shape->plainText();
}

int SvgTextLabel::padding() const
{
    return d->padding;
}

void SvgTextLabel::setFontFamilies(QStringList fontFamilies)
{
    KoSvgTextProperties props = d->shape->textProperties();
    QStringList current = props.property(KoSvgTextProperties::FontFamiliesId).value<QStringList>();
    if (current == fontFamilies)
        return;

    props.setProperty(KoSvgTextProperties::FontFamiliesId, fontFamilies);
    d->shape->setPropertiesAtPos(-1, props);
    emit fontFamiliesChanged(fontFamilies);
}

void SvgTextLabel::setFontWeight(int fontWeight)
{
    KoSvgTextProperties props = d->shape->textProperties();
    int current = props.property(KoSvgTextProperties::FontWeightId).toInt();
    if (current == fontWeight)
        return;

    props.setProperty(KoSvgTextProperties::FontWeightId, fontWeight);
    d->shape->setPropertiesAtPos(-1, props);
    emit fontWeightChanged(fontWeight);
}

void SvgTextLabel::setFontStyle(QFont::Style fontStyle)
{
    KoSvgTextProperties props = d->shape->textProperties();
    QFont::Style current = props.property(KoSvgTextProperties::FontStyleId).value<QFont::Style>();
    if (current == fontStyle)
        return;

    props.setProperty(KoSvgTextProperties::FontStyleId, fontStyle);
    d->shape->setPropertiesAtPos(-1, props);
    updateShape();
    emit fontStyleChanged(fontStyle);
}

void SvgTextLabel::setFontSize(qreal fontSize)
{
    KoSvgTextProperties props = d->shape->textProperties();
    qreal current = props.property(KoSvgTextProperties::FontSizeId).toReal();
    if (qFuzzyCompare(current, fontSize))
        return;

    props.setProperty(KoSvgTextProperties::FontSizeId, fontSize);
    d->shape->setPropertiesAtPos(-1, props);
    emit fontSizeChanged(fontSize);
}

void SvgTextLabel::setTextColor(QColor textColor)
{
    KoSvgTextProperties props = d->shape->textProperties();
    if (props.hasProperty(KoSvgTextProperties::FillId)) {
        KoColorBackground *bg = dynamic_cast<KoColorBackground*>(d->shape->background().data());
        if (bg) {
            if (bg->color() == textColor)
                return;
        }
    }

    d->shape->setBackground(QSharedPointer<KoColorBackground>(new KoColorBackground(textColor)));
    emit textColorChanged(textColor);
}

void SvgTextLabel::setOpenTypeFeatures(QStringList openTypeFeatures)
{
    KoSvgTextProperties props = d->shape->textProperties();
    QStringList otf = props.property(KoSvgTextProperties::FontFeatureSettingsId).value<QStringList>();
    if (otf == openTypeFeatures)
        return;

    props.setProperty(KoSvgTextProperties::FontFeatureSettingsId, openTypeFeatures);
    d->shape->setPropertiesAtPos(-1, props);
    emit openTypeFeaturesChanged(openTypeFeatures);
}

void SvgTextLabel::setText(QString text)
{
    if (d->shape->plainText() == text)
        return;

    int length = d->shape->plainText().size();
    int start = 0;
    d->shape->removeText(start, length);
    d->shape->insertText(0, text);
    updateShape();
    emit textChanged(d->shape->plainText());
}

void SvgTextLabel::setPadding(int padding)
{
    if (d->padding == padding)
        return;

    d->padding = padding;
    emit paddingChanged(d->padding);
}

void SvgTextLabel::updateShape()
{
    d->shape->relayout();
    setImplicitSize(d->shape->boundingRect().width(), d->shape->boundingRect().height());
    update();
}
