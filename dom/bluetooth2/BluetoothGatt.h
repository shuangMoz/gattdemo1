/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluetoothgatt_h__
#define mozilla_dom_bluetooth_bluetoothgatt_h__

#include "mozilla/Attributes.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/BluetoothGattBinding.h"
#include "mozilla/dom/bluetooth/BluetoothGattService.h"
#include "BluetoothCommon.h"
#include "nsCOMPtr.h"

namespace mozilla {
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothReplyRunnable;
class BluetoothService;
class BluetoothSignal;
class BluetoothValue;

class BluetoothGatt MOZ_FINAL : public DOMEventTargetHelper
                              , public BluetoothSignalObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(BluetoothGatt,
                                           DOMEventTargetHelper)

  /****************************************************************************
   * Attribute Getters
   ***************************************************************************/
  BluetoothConnectionState ConnectionState() const
  {
    return mConnectionState;
  }

  void GetServices(nsTArray<nsRefPtr<BluetoothGattService>>& aServices) const
  {
    aServices = mServices;
  }

  /****************************************************************************
   * Event Handlers
   ***************************************************************************/
  IMPL_EVENT_HANDLER(connectionstatechanged);
  IMPL_EVENT_HANDLER(characteristicchanged);

  /****************************************************************************
   * Methods (Web API Implementation)
   ***************************************************************************/
  already_AddRefed<Promise> Connect(ErrorResult& aRv);
  already_AddRefed<Promise> Disconnect(ErrorResult& aRv);

  /****************************************************************************
   * Others
   ***************************************************************************/
  static already_AddRefed<BluetoothGatt> Create(
    nsPIDOMWindow* aOwner,
    bool aAutoConnect,
    const nsAString& aDeviceAddr,
    BluetoothReplyRunnable* aRunnable);

  void Notify(const BluetoothSignal& aParam); // BluetoothSignalObserver

  nsPIDOMWindow* GetParentObject() const
  {
     return GetOwner();
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;
  virtual void DisconnectFromOwner() MOZ_OVERRIDE;

private:
  BluetoothGatt(nsPIDOMWindow* aOwner,
                const nsAString& aAppUuid,
                bool aAutoConnect,
                const nsAString& aDeviceAddr,
                BluetoothReplyRunnable* aRunnable,
                BluetoothService* aService);
  ~BluetoothGatt();

  /**
   * Update mConnectionState to aState and fire
   * connectionstatechanged event to the application.
   *
   * @param aState [in] New connection state
   */
  void UpdateConnectionState(BluetoothConnectionState aState);

  /**
   * fire characteristicchanged event to the application.
   *
   * @param
   */
  void UpdateCharacteristChanged(BluetoothGattCharacteristic* aChar);

  /**
   * Unregister the gatt client from bluetooth stack.
   */
  void UnregisterClient();

  /**
   * Trigger connect operation after receiving "ClientRegistered" signal.
   * This signal will be received when application sucessfully registerd itself
   * as a gatt client from bluetooth stack.
   *
   * @param aValue [in] BluetoothValue
   */
  void HandleClientRegistered(const BluetoothValue& aValue);

  void HandleServiceDiscovered(const BluetoothValue& aValue);
  void HandleSearchCompleted();
  void HandleGetCharacteristic(const BluetoothValue& aValue);
  void HandleGetDescriptor(const BluetoothValue& aValue);
  void HandleCharacteristicChanged(const BluetoothValue& aValue);

  /****************************************************************************
   * Variables
   ***************************************************************************/
  /**
   * Random generated UUID of this gatt client.
   */
  nsString mAppUuid;

  /**
   * Whether to directly connect to the remote device (false) or
   * to automatically connect as soon as the remote device becomes
   * available (true).
   */
  bool mAutoConnect;

  /**
   * Id of the gatt client interface given by bluetooth stack.
   */
  int mClientIf;

  /**
   * Connection state of this remote device.
   */
  BluetoothConnectionState mConnectionState;

  /**
   * Address of the remote device.
   */
  nsString mDeviceAddr;

  nsRefPtr<BluetoothReplyRunnable> mConnectGattRunnable;

  nsTArray<nsRefPtr<BluetoothGattService>> mServices;
};

END_BLUETOOTH_NAMESPACE

#endif
