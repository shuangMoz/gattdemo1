/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__
#define mozilla_dom_bluetooth_bluetoothgattcharacteristic_h__

#include "mozilla/Attributes.h"
#include "BluetoothCommon.h"
#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsWrapperCache.h"

namespace mozilla {
class ErrorResult;
namespace dom {
class Promise;
}
}

BEGIN_BLUETOOTH_NAMESPACE

class BluetoothGattCharacteristic MOZ_FINAL : public nsISupports,
                                              public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattCharacteristic)

  /****************************************************************************
   * Attribute Getters
   ***************************************************************************/

  void GetUuid(nsString& aUuid) const
  {
    aUuid = mUuid;
  }

  int InstanceId() const
  {
    return mInstanceId;
  }

  void GetValue(JSContext* cx,
                JS::MutableHandle<JSObject*> aValue,
                ErrorResult& aRv) { }

  already_AddRefed<Promise>
  StartNotifications(ErrorResult& aRv);

  /****************************************************************************
   * Others
   ***************************************************************************/
  static already_AddRefed<BluetoothGattCharacteristic> Create(
    nsPIDOMWindow* aOwner,
    const nsAString& aUuid,
    int aInstanceId,
    int aClientIf,
    const nsAString& aServiceUuid,
    int aServiceInstanceId,
    bool aIsPrimary,
    const nsAString& aDeviceAddr);

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  BluetoothGattCharacteristic(nsPIDOMWindow* aOwner,
                              const nsAString& aUuid,
                              int aInstanceId,
                              int aClientIf,
                              const nsAString& aServiceUuid,
                              int aServiceInstanceId,
                              bool aIsPrimary,
                              const nsAString& aDeviceAddr);

  ~BluetoothGattCharacteristic();

  void Root();
  void Unroot();
  /****************************************************************************
   * Variables
   ***************************************************************************/

  nsCOMPtr<nsPIDOMWindow> mOwner;

  /**
   * UUID of this gatt characteristic.
   */
  nsString mUuid;

  /**
   * Instance id of the gatt characteristic.
   */
  int mInstanceId;

  /**
   * Raw value of this characteristic.
   */
  nsTArray<uint8_t> mRawValue;

  JS::Heap<JSObject*> mValue;
  bool mIsRooted;

  int mClientIf;
  nsString mDeviceAddr;
  nsString mServiceUuid;
  int mServiceInstanceId;
  bool mIsPrimary;
};

END_BLUETOOTH_NAMESPACE

#endif
