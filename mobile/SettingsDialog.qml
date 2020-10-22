import QtQuick 2.0
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.3

Dialog {
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    standardButtons: Dialog.Ok | Dialog.Cancel

    SystemPalette {
        id: palette
    }

    header: TabBar {
        id: tabBar
        width: parent.width
        TabButton {
            text: "Connection"
        }
        TabButton {
            text: "Interface"
        }
    }

    background: Rectangle {
        color: palette.base
    }

    onAccepted: {
        var newHost = hostField.text
        var newPort = portField.text
        var newEncrypted = encryptedCheckbox.checked
        var newPassphrase = passphraseField.text
        weechat.host = newHost
        weechat.port = newPort
        weechat.encrypted = newEncrypted
        if (newPassphrase.length > 0)
            weechat.setPassphrase(newPassphrase)
        newPassphrase = ""
    }
    onRejected: {
        hostField.text = weechat.host
        portField.text = weechat.port
        encryptedCheckbox.checked = weechat.encrypted
        passphraseField.text = ""
    }

    width: parent.width - 12
    height: parent.height - 12

    StackLayout {
        anchors.fill: parent
        currentIndex: tabBar.currentIndex
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            GridLayout {
                columns: 2
                Text {
                    text: "Hostname"
                    color: palette.text
                }
                TextField {
                    id: hostField
                    text: weechat.host
                    inputMethodHints: Qt.ImhNoPredictiveText
                }
                Text {
                    text: "Port"
                    color: palette.text
                }
                TextField {
                    id: portField
                    text: weechat.port
                    inputMethodHints: Qt.ImhPreferNumbers
                    validator: IntValidator {
                        bottom: 0
                        top: 65535
                    }
                }
                Text {
                    text: "SSL"
                    color: palette.text
                }
                CheckBox {
                    id: encryptedCheckbox
                    checked: weechat.encrypted
                    Layout.alignment: Qt.AlignLeft
                }
                Text {
                    text: "Password"
                    color: palette.text
                }
                TextField {
                    id: passphraseField
                    placeholderText: weechat.hasPassphrase ? "*****" : ""
                    echoMode: TextInput.Password
                    passwordCharacter: "*"
                }
            }
            Item {
                Layout.fillHeight: true
            }
        }
        Text {
            text: "AHOJ"
        }
    }
}
