/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtBluetooth module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qlowenergycontrollerbase_p.h"

#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtBluetooth/QLowEnergyServiceData>

QT_BEGIN_NAMESPACE

QLowEnergyControllerPrivate::QLowEnergyControllerPrivate()
    : QObject()
{
}

QLowEnergyControllerPrivate::~QLowEnergyControllerPrivate()
{
}

bool QLowEnergyControllerPrivate::isValidLocalAdapter()
{
#ifdef QT_WINRT_BLUETOOTH
    return true;
#endif
    if (localAdapter.isNull())
        return false;

    const QList<QBluetoothHostInfo> foundAdapters = QBluetoothLocalDevice::allDevices();
    bool adapterFound = false;

    foreach (const QBluetoothHostInfo &info, foundAdapters) {
        if (info.address() == localAdapter) {
            adapterFound = true;
            break;
        }
    }

    return adapterFound;
}


void QLowEnergyControllerPrivate::setError(
        QLowEnergyController::Error newError)
{
    Q_Q(QLowEnergyController);
    error = newError;

    switch (newError) {
    case QLowEnergyController::UnknownRemoteDeviceError:
        errorString = QLowEnergyController::tr("Remote device cannot be found");
        break;
    case QLowEnergyController::InvalidBluetoothAdapterError:
        errorString = QLowEnergyController::tr("Cannot find local adapter");
        break;
    case QLowEnergyController::NetworkError:
        errorString = QLowEnergyController::tr("Error occurred during connection I/O");
        break;
    case QLowEnergyController::ConnectionError:
        errorString = QLowEnergyController::tr("Error occurred trying to connect to remote device.");
        break;
    case QLowEnergyController::AdvertisingError:
        errorString = QLowEnergyController::tr("Error occurred trying to start advertising");
        break;
    case QLowEnergyController::RemoteHostClosedError:
        errorString = QLowEnergyController::tr("Remote device closed the connection");
        break;
    case QLowEnergyController::NoError:
        return;
    default:
    case QLowEnergyController::UnknownError:
        errorString = QLowEnergyController::tr("Unknown Error");
        break;
    }

    emit q->error(newError);
}

void QLowEnergyControllerPrivate::setState(
        QLowEnergyController::ControllerState newState)
{
    Q_Q(QLowEnergyController);
    if (state == newState)
        return;

    state = newState;
    if (state == QLowEnergyController::UnconnectedState
            && role == QLowEnergyController::PeripheralRole) {
        remoteDevice.clear();
    }
    emit q->stateChanged(state);
}

QSharedPointer<QLowEnergyServicePrivate> QLowEnergyControllerPrivate::serviceForHandle(
        QLowEnergyHandle handle)
{
    ServiceDataMap &currentList = serviceList;
    if (role == QLowEnergyController::PeripheralRole)
        currentList = localServices;

    const QList<QSharedPointer<QLowEnergyServicePrivate>> values = currentList.values();
    for (auto service: values)
        if (service->startHandle <= handle && handle <= service->endHandle)
            return service;

    return QSharedPointer<QLowEnergyServicePrivate>();
}

/*!
    Returns a valid characteristic if the given handle is the
    handle of the characteristic itself or one of its descriptors
 */
QLowEnergyCharacteristic QLowEnergyControllerPrivate::characteristicForHandle(
        QLowEnergyHandle handle)
{
    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(handle);
    if (service.isNull())
        return QLowEnergyCharacteristic();

    if (service->characteristicList.isEmpty())
        return QLowEnergyCharacteristic();

    // check whether it is the handle of a characteristic header
    if (service->characteristicList.contains(handle))
        return QLowEnergyCharacteristic(service, handle);

    // check whether it is the handle of the characteristic value or its descriptors
    QList<QLowEnergyHandle> charHandles = service->characteristicList.keys();
    std::sort(charHandles.begin(), charHandles.end());
    for (int i = charHandles.size() - 1; i >= 0; i--) {
        if (charHandles.at(i) > handle)
            continue;

        return QLowEnergyCharacteristic(service, charHandles.at(i));
    }

    return QLowEnergyCharacteristic();
}

/*!
    Returns a valid descriptor if \a handle belongs to a descriptor;
    otherwise an invalid one.
 */
QLowEnergyDescriptor QLowEnergyControllerPrivate::descriptorForHandle(
        QLowEnergyHandle handle)
{
    const QLowEnergyCharacteristic matchingChar = characteristicForHandle(handle);
    if (!matchingChar.isValid())
        return QLowEnergyDescriptor();

    const QLowEnergyServicePrivate::CharData charData = matchingChar.
            d_ptr->characteristicList[matchingChar.attributeHandle()];

    if (charData.descriptorList.contains(handle))
        return QLowEnergyDescriptor(matchingChar.d_ptr, matchingChar.attributeHandle(),
                                    handle);

    return QLowEnergyDescriptor();
}

/*!
    Returns the length of the updated characteristic value.
 */
quint16 QLowEnergyControllerPrivate::updateValueOfCharacteristic(
        QLowEnergyHandle charHandle,const QByteArray &value, bool appendValue)
{
    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(charHandle);
    if (!service.isNull()) {
        CharacteristicDataMap::iterator charIt = service->characteristicList.find(charHandle);
        if (charIt != service->characteristicList.end()) {
            QLowEnergyServicePrivate::CharData &charDetails = charIt.value();

            if (appendValue)
                charDetails.value += value;
            else
                charDetails.value = value;

            return charDetails.value.size();
        }
    }

    return 0;
}

/*!
    Returns the length of the updated descriptor value.
 */
quint16 QLowEnergyControllerPrivate::updateValueOfDescriptor(
        QLowEnergyHandle charHandle, QLowEnergyHandle descriptorHandle,
        const QByteArray &value, bool appendValue)
{
    QSharedPointer<QLowEnergyServicePrivate> service = serviceForHandle(charHandle);
    if (!service.isNull()) {
        CharacteristicDataMap::iterator charIt = service->characteristicList.find(charHandle);
        if (charIt != service->characteristicList.end()) {
            QLowEnergyServicePrivate::CharData &charDetails = charIt.value();

            DescriptorDataMap::iterator descIt = charDetails.descriptorList.find(descriptorHandle);
            if (descIt != charDetails.descriptorList.end()) {
                QLowEnergyServicePrivate::DescData &descDetails = descIt.value();

                if (appendValue)
                    descDetails.value += value;
                else
                    descDetails.value = value;

                return descDetails.value.size();
            }
        }
    }

    return 0;
}

void QLowEnergyControllerPrivate::invalidateServices()
{
    foreach (const QSharedPointer<QLowEnergyServicePrivate> service, serviceList.values()) {
        service->setController(0);
        service->setState(QLowEnergyService::InvalidService);
    }

    serviceList.clear();
}

QT_END_NAMESPACE
