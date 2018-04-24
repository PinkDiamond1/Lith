import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0

import "."
import "Default"

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: "Lith"

    statusBar: StatusBar {
        RowLayout {
            anchors.fill: parent
            Text {
                text: qsTr(stuff.bufferCount + " buffers")
            }
            Item {
                height: 1
                Layout.fillWidth: true
            }
            Text {
                visible: weechat.fetchTo !== 0
                text: "Syncing"
            }
            ProgressBar {
                visible: weechat.fetchTo !== 0
                minimumValue: 0
                value: weechat.fetchFrom.toFixed(1) / weechat.fetchTo.toFixed(1)
            }
        }
    }
    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal
        ColumnLayout {
            spacing: 0
            Button {
                implicitWidth: 200
                Layout.fillWidth: true
                text: "Lith"
                onClicked: settings.open()
            }
            BufferList {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }
        Item {
            clip: true
            Layout.fillWidth: true

            Item {
                id: chatTitle
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                }
                height: childrenRect.height
                RowLayout {
                    width: parent.width
                    Button {
                        text: "Gimme backlog"
                        onClicked: stuff.selected.fetchMoreLines()
                    }
                    Text {
                        font.family: "Consolas"
                        text: stuff.selected ? stuff.selected.name + ": " + stuff.selected.title : "(No buffer selected)"
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Button {
                        text: nickList.open ? ">>" : "<< Nicklist"
                        onClicked: nickList.open = !nickList.open
                    }
                }
            }

            Rectangle {
                anchors.fill: messageScrollView
                color: "white"
            }

            ScrollView {
                id: messageScrollView
                anchors {
                    top: chatTitle.bottom
                    bottom: chatInput.top
                    left: parent.left
                    right: parent.right
                }
                horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

                ListView {
                    id: messageListView
                    width: parent.width
                    interactive: false

                    clip: true
                    spacing: 3

                    onCountChanged: {
                        currentIndex = count - 1
                    }
                    onWidthChanged: {
                        positionViewAtEnd()
                    }
                    onHeightChanged: {
                        positionViewAtEnd()
                    }

                    snapMode: ListView.SnapToItem
                    highlightRangeMode: ListView.ApplyRange
                    highlightMoveDuration: 100
                    highlightFollowsCurrentItem: true
                    highlightResizeDuration: 100
                    preferredHighlightBegin: height - (highlightItem ? highlightItem.height : 0)
                    preferredHighlightEnd: height - (highlightItem ? highlightItem.height : 0)
                    highlight: Rectangle {
                        color: "#88ff0000"
                    }

                    addDisplaced: Transition {
                        NumberAnimation {
                            property: "y"
                        }
                    }

                    add: Transition {
                        NumberAnimation {
                            property: "y"
                            from: height
                        }
                    }

                    model: stuff.selected.lines

                    delegate: Line {}
                }
            }

            TextField {
                id: chatInput
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
                onAccepted: {
                    stuff.selected.input(chatInput.text)
                    chatInput.text = ""
                }
            }
        }
        NickList {
            id: nickList
            clip: true
            model: stuff.selected.nicks
            implicitWidth: 200
        }
    }

    Settings {
        id: settings
    }
}
