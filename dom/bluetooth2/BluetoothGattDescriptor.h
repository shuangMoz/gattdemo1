/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_bluetooth_bluetoothgattdescriptor_h__
#define mozilla_dom_bluetooth_bluetoothgattdescriptor_h__

#include "mozilla/Attributes.h"
#include "mozilla/dom/TypedArray.h"
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

class BluetoothGattDescriptor MOZ_FINAL : public nsISupports,
                                          public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(BluetoothGattDescriptor)

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

  WriteValue(const ArrayBuffer& aValue, ErrorResult& aRv);

  /****************************************************************************
   * Others
   ***************************************************************************/
  static already_AddRefed<BluetoothGattDescriptor> Create(
    nsPIDOMWindow* aOwner,
    const nsAString& aUuid,
    int aInstanceId,
    int aConnId,
    const nsAString& aServiceUuid,
    int aServiceInstanceId,
    bool aIsPrimary,
    const nsAString& aCharUuid,
    int aCharInstanceId);

  nsPIDOMWindow* GetParentObject() const
  {
     return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  BluetoothGattDescriptor(nsPIDOMWindow* aOwner,
                          const nsAString& aUuid,
                          int aInstanceId,
                          int aConnId,
                          const nsAString& aServiceUuid,
                          int aServiceInstanceId,
                          bool aIsPrimary,
                          const nsAString& aCharUuid,
                          int aCharInstanceId);

  ~BluetoothGattDescriptor();

  void Root();
  void Unroot();
  /****************************************************************************
   * Variables
   ***************************************************************************/

  nsCOMPtr<nsPIDOMWindow> mOwner;

  /**
   * UUID of this gatt descriptor.
   */
  nsString mUuid;

  /**
   * Instance id of the gatt descriptor.
   */
  int mInstanceId;

  /**
   * Raw value of this descriptor.
   */
  nsTArray<uint8_t> mRawValue;

  JS::Heap<JSObject*> mValue;
  bool mIsRooted;

  int mConnId;
  nsString mServiceUuid;
  int mServiceInstanceId;
  bool mIsPrimary;
  nsString mCharUuid;
  int mCharInstanceId;
};

END_BLUETOOTH_NAMESPACE

#endif
