/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12
import org.krita.flake.text 1.0

CollapsibleGroupProperty {
    propertyName: i18nc("@label", "Font Style");
    propertyType: TextPropertyBase.Character;
    toolTip: i18nc("@info:tooltip",
                   "Font style allows setting the sub style of the given font family, such as italics and bold");
    searchTerms: i18nc("comma separated search terms for the fontsize property, matching is case-insensitive",
                       "weight, width, style, italics, oblique, font-style, font-stretch, font-weight, bold, optical-size, variation");

    property alias fontWeight: fontWeightSpn.value;
    property alias fontWidth: fontStretchSpn.value;
    property int fontSlant: KoSvgTextPropertiesModel.StyleNormal;

    onFontSlantChanged: {
        fontSlantCmb.currentIndex = fontSlantCmb.indexOfValue(fontSlant);
        if (!blockSignals) {
            properties.fontStyle = fontSlant;
        }
    }
    property alias fontOptical: opticalSizeCbx.checked;

    onPropertiesUpdated: {
        blockSignals = true;
        fontWeight = properties.fontWeight;
        fontWidth = properties.fontWidth;
        fontOptical = properties.fontOpticalSizeLink;
        fontSlant = properties.fontStyle;
        styleCmb.currentIndex = fontStylesModel.rowForStyle(properties.fontWeight, properties.fontWidth, properties.fontStyle);
        visible = properties.fontWeightState !== KoSvgTextPropertiesModel.PropertyUnset
                 || properties.fontStyleState !== KoSvgTextPropertiesModel.PropertyUnset
                 || properties.fontWidthState !== KoSvgTextPropertiesModel.PropertyUnset
                 || properties.fontOpticalSizeLinkState !== KoSvgTextPropertiesModel.PropertyUnset;
        blockSignals = false;
    }
    onFontWeightChanged: {
        if (!blockSignals) {
            properties.fontWeight = fontWeight;
        }
    }

    onFontWidthChanged: {
        if (!blockSignals) {
            properties.fontWidth = fontWidth;
        }
    }

    onFontOpticalChanged: {
        if (!blockSignals) {
            properties.fontOpticalSizeLink = fontOptical;
        }
    }

    onEnableProperty: {
        properties.fontWeightState = KoSvgTextPropertiesModel.PropertySet;
    }

    titleItem: ComboBox {
        id: styleCmb;
        model: fontStylesModel;
        textRole: "display";
        width: parent.width;
        onActivated: {
            if (!blockSignals) {
                // Because each change to propertiesModel causes signals to fire,
                // we need to first store the vars and then apply them.
                var weight = fontStylesModel.weightValue(currentIndex);
                var width = fontStylesModel.widthValue(currentIndex);
                var styleMode = fontStylesModel.styleModeValue(currentIndex);
                var axesValues = fontStylesModel.axesValues(currentIndex);
                properties.fontWeight = weight;
                properties.fontWidth = width;
                properties.fontStyle = styleMode;
                properties.axisValues = axesValues;
            }
        }
    }

    contentItem: GridLayout {
        id: mainLayout;
        columns: 3
        anchors.left: parent.left
        anchors.right: parent.right
        columnSpacing: 5;

        RevertPropertyButton {
            revertState: properties.fontWeightState;
            onClicked: properties.fontWeightState = KoSvgTextPropertiesModel.PropertyUnset;
        }

        Label {
            text: i18nc("@label:spinbox", "Weight:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight;
            Layout.preferredWidth: implicitWidth;
            elide: Text.ElideRight;
            font.italic: properties.fontWeightState === KoSvgTextPropertiesModel.PropertyTriState;
        }


        SpinBox {
            id: fontWeightSpn
            from: 0;
            to: 1000;
            editable: true;
            Layout.fillWidth: true;

            wheelEnabled: true;

        }


        RevertPropertyButton {
            revertState: properties.fontWidthState;
            onClicked: properties.fontWidthState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            id: widthLabel;
            text: i18nc("@label:spinbox", "Width:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight;
            Layout.preferredWidth: implicitWidth;
            elide: Text.ElideRight;
            font.italic: properties.fontWidthState === KoSvgTextPropertiesModel.PropertyTriState;
        }
        SpinBox {
            id: fontStretchSpn
            from: 0;
            to: 200;
            editable: true;
            Layout.fillWidth: true;
            wheelEnabled: true;
        }

        RevertPropertyButton {
            revertState: properties.fontStyleState;
            onClicked: properties.fontStyleState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Label {
            text: i18nc("@label:listbox", "Slant:")
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            horizontalAlignment: Text.AlignRight;
            Layout.preferredWidth: implicitWidth;
            elide: Text.ElideRight;
            font.italic: properties.fontStyleState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        ComboBox {
            id: fontSlantCmb
            model: [
                {text: i18nc("@label:inlistbox", "Normal"), value: KoSvgTextPropertiesModel.StyleNormal},
                {text: i18nc("@label:inlistbox", "Italic"), value: KoSvgTextPropertiesModel.StyleItalic},
                {text: i18nc("@label:inlistbox", "Oblique"), value: KoSvgTextPropertiesModel.StyleOblique}
            ]
            Layout.fillWidth: true
            textRole: "text";
            valueRole: "value";
            onActivated: fontSlant = currentValue;
            wheelEnabled: true;
        }

        RevertPropertyButton {
            revertState: properties.fontOpticalSizeLinkState;
            onClicked: properties.fontOpticalSizeLinkState = KoSvgTextPropertiesModel.PropertyUnset;
        }
        Item {
        width: 1;
        height: 1;}

        CheckBox {
            id: opticalSizeCbx
            text: i18nc("@option:check", "Optical Size")
            Layout.fillWidth: true;
            font.italic: properties.fontOpticalSizeLinkState === KoSvgTextPropertiesModel.PropertyTriState;
        }

        ColumnLayout {
            RevertPropertyButton {
                revertState: properties.axisValueState;
                onClicked: properties.axisValueState = KoSvgTextPropertiesModel.PropertyUnset;
            }
            Item {
                Layout.fillHeight: true;
            }
        }
        ListView {
            id: axesView;
            model: fontAxesModel;
            Layout.columnSpan: 2;
            Layout.fillWidth: true;
            Layout.preferredHeight: contentHeight;
            spacing: parent.columnSpacing;

            function updateAxes() {

            }

            delegate: RowLayout {
                spacing: axesView.spacing;
                width: axesView.width;
                height: implicitHeight;
                required property string display;
                required property double axismin;
                required property double axismax;
                required property bool axishidden;
                required property var model;

                Label {
                    text: parent.display;
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                    horizontalAlignment: Text.AlignRight;
                    height: parent.height;
                    elide: Text.ElideRight;
                    Layout.preferredWidth: widthLabel.width+spacing;
                }
                DoubleSpinBox {
                    id: axisSpn;
                    from: parent.axismin * multiplier;
                    to: parent.axismax * multiplier;
                    value: model.edit * multiplier;
                    onValueChanged: model.edit = (value / axisSpn.multiplier);
                    Layout.fillWidth: true;
                }
            }
        }

    }
}
