// Lith
// Copyright (C) 2020 Martin Bříza
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; If not, see <http://www.gnu.org/licenses/>.

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12

ListView {
    id: listView

    // ugh, this is an ugly hack to fix the button under the Drawer drag area
    // Qt doesn't seem to care about this https://bugreports.qt.io/browse/QTBUG-59141
    // note the whole widget is rotated by 180 degress so it has to be subtracted from the height
    property real scrollToBottomButtonPosition: scrollToBottomButton.visible ? height - scrollToBottomButton.y - scrollToBottomButton.height
                                                                             : height

    TextMetrics {
        id: timeMetrics
        text: Qt.formatTime(new Date(), Locale.LongFormat)
        font.pointSize: settings.baseFontSize
    }


    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        hoverEnabled: true
        active: hovered || pressed
        orientation: Qt.Vertical
        parent: listView.parent
        anchors.top: listView.top
        anchors.right: listView.right
        anchors.bottom: listView.bottom
    }

    verticalLayoutDirection: ListView.BottomToTop
    orientation: Qt.Vertical
    spacing: lith.settings.messageSpacing
    model: lith.selectedBuffer ? lith.selectedBuffer.lines_filtered : null
    delegate: ChannelMessage {
        messageModel: modelData
    }

    ChannelMessageActionMenu {
        id: channelMessageActionMenu
    }

    function fillTopOfList() {
        if (!lith.selectedBuffer)
            return
        if (yPosition - visibleArea.heightRatio < 0.25) {
            lith.selectedBuffer.fetchMoreLines()
        }
    }

    property real yPosition: visibleArea.yPosition
    onYPositionChanged: fillTopOfList()
    onContentHeightChanged: fillTopOfList()
    onModelChanged: fillTopOfList()

    property real absoluteYPosition: yPosition + visibleArea.heightRatio
    onAbsoluteYPositionChanged: {
        if (Qt.inputMethod.visible && absoluteYPosition < 1)
            Qt.inputMethod.hide()
    }

    Button {
        id: scrollToBottomButton
        anchors {
            bottom: parent.bottom
            right: parent.right
            margins: height / 2
        }
        width: height
        flat: false
        icon.source: "qrc:/navigation/"+currentTheme+"/down-arrow.png"
        opacity: listView.atYEnd ? 0.0 : 0.5
        visible: opacity > 0.0
        Behavior on opacity { NumberAnimation { duration: 100 } }
        onClicked: positionViewAtBeginning()
    }
}
