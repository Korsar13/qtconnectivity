// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QBluetoothLocalDevice>

class ConnectionHandler : public QObject
{
    Q_PROPERTY(bool alive READ alive NOTIFY deviceChanged)
    Q_PROPERTY(QString name READ name NOTIFY deviceChanged)
    Q_PROPERTY(QString address READ address NOTIFY deviceChanged)
    Q_PROPERTY(bool requiresAddressType READ requiresAddressType CONSTANT)

    Q_OBJECT
public:
    explicit ConnectionHandler(QObject *parent = nullptr);

    bool alive() const;
    bool requiresAddressType() const;
    QString name() const;
    QString address() const;

signals:
    void deviceChanged();

private slots:
    void hostModeChanged(QBluetoothLocalDevice::HostMode mode);

private:
    QBluetoothLocalDevice m_localDevice;
};

#endif // CONNECTIONHANDLER_H
