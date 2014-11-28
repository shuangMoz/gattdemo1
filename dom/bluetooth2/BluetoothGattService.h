/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluetoothgattservice_h__
#define mozilla_dom_bluetooth_bluetoothgattservice_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/BluetoothGattServiceBinding.h"
#include "mozilla/dom/bluetooth/BluetoothGattCharacteristic.h"
#include "BluetoothCommon.h"
#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "nsPIDOMWindow.h"

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothSignal;
class BluetoothValue;

class BluetoothGattService MOZ_FINAL : public nsISupports,
                                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattService)

  /****************************************************************************
   * Attribute Getters
   ***************************************************************************/
  void GetCharacteristics(
    nsTArray<nsRefPtr<BluetoothGattCharacteristic>>& aCharacteristics) const
  {
    aCharacteristics = mCharacteristics;
  }

  bool IsPrimary() const
  {
    return mIsPrimary;
  }

  void GetUuid(nsString& aUuid) const
  {
    aUuid = mUuid;
  }

  int InstanceId() const
  {
    return mInstanceId;
  }

  /****************************************************************************
   * Others
   ***************************************************************************/
  static already_AddRefed<BluetoothGattService> Create(
    nsPIDOMWindow* aOwner,
    const BluetoothValue& aValue);

  void Notify(const BluetoothSignal& aParam); // BluetoothSignalObserver

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  int ConnId() const
  {
    return mConnId;
  }

  void AppendCharacteristic(nsAString& aUuid,
                            int aInstanceId,
                            int aClientIf,
                            nsAString& aDeviceAddr);

  /**
   * A helper function to find existing characteristics.
   */
  already_AddRefed<BluetoothGattCharacteristic>
  FindCharacteristic(const nsAString& aUuid, int aInstanceId);

private:
  BluetoothGattService(nsPIDOMWindow* aOwner,
                       bool aIsPrimary,
                       const nsAString& aUuid,
                       int aInstanceId,
                       int aConnId);

  ~BluetoothGattService();

  nsCOMPtr<nsPIDOMWindow> mOwner;

  /****************************************************************************
   * Variables
   ***************************************************************************/

  /**
   * Characteristics provided by this gatt service.
   */
  nsTArray<nsRefPtr<BluetoothGattCharacteristic>> mCharacteristics;
  /**
   * UUID of this gatt service.
   */
  nsString mUuid;

  /**
   * Indicate whether this is a primary service or not.
   */
  bool mIsPrimary;

  /**
   * Instance id of the gatt service.
   */
  int mInstanceId;

  int mConnId;
};

END_BLUETOOTH_NAMESPACE

#endif
