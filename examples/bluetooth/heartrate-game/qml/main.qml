// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.7
import QtQuick.Window 2.2
import "."

Window {
    id: wroot
    visible: true
    width: 720 * .7
    height: 1240 * .7
    title: qsTr("HeartRateGame")
    color: GameSettings.backgroundColor

    Component.onCompleted: {
        GameSettings.wWidth = Qt.binding(function() {return width})
        GameSettings.wHeight = Qt.binding(function() {return height})
    }

    Loader {
        id: splashLoader
        anchors.fill: parent
        source: "SplashScreen.qml"
        asynchronous: false
        visible: true

        onStatusChanged: {
            if (status === Loader.Ready) {
                appLoader.setSource("App.qml");
            }
        }
    }

    Connections {
        target: splashLoader.item
        function onReadyToGo() {
            appLoader.visible = true
            appLoader.item.init()
            splashLoader.visible = false
            splashLoader.setSource("")
            appLoader.item.forceActiveFocus();
        }
    }

    Loader {
        id: appLoader
        anchors.fill: parent
        visible: false
        asynchronous: true
        onStatusChanged: {
            if (status === Loader.Ready)
                splashLoader.item.appReady()
            if (status === Loader.Error)
                splashLoader.item.errorInLoadingApp();
        }
    }
}
