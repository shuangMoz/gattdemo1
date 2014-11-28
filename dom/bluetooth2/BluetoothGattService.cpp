/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothGattService.h"
#include "BluetoothGattCharacteristic.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothGattServiceBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(
  BluetoothGattService, mOwner, mCharacteristics)
NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattService)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattService)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattService)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothGattService::BluetoothGattService(nsPIDOMWindow* aOwner,
                             bool aIsPrimary,
                             const nsAString& aUuid,
                             int aInstanceId,
                             int aConnId)
  : mOwner(aOwner)
  , mIsPrimary(aIsPrimary)
  , mUuid(aUuid)
  , mInstanceId(aInstanceId)
  , mConnId(aConnId)
{
  BT_API2_LOGR();

  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(!mUuid.IsEmpty());
}

BluetoothGattService::~BluetoothGattService()
{
}

// static
already_AddRefed<BluetoothGattService>
BluetoothGattService::Create(nsPIDOMWindow* aWindow,
                             const BluetoothValue& aValue)
{
  BT_API2_LOGR("BluetoothGattService::Create");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  MOZ_ASSERT(aValue.type() == BluetoothValue::TArrayOfBluetoothNamedValue);
  const InfallibleTArray<BluetoothNamedValue>& values =
    aValue.get_ArrayOfBluetoothNamedValue();

  MOZ_ASSERT(values.Length() == 4 && // uuid, instanceId, isPrimary, connId
             values[0].value().type() == BluetoothValue::TnsString &&
             values[1].value().type() == BluetoothValue::Tuint32_t &&
             values[2].value().type() == BluetoothValue::Tuint32_t &&
             values[3].value().type() == BluetoothValue::Tuint32_t);

  nsString uuid = values[0].value().get_nsString();
  int instanceId = values[1].value().get_uint32_t();
  bool isPrimary = (bool)values[2].value().get_uint32_t();
  int connId = values[3].value().get_uint32_t();

  BT_API2_LOGR("create service with uuid %s, instanceid %d, isPrimary %d",
               NS_ConvertUTF16toUTF8(uuid).get(), instanceId, isPrimary);

  nsRefPtr<BluetoothGattService> gattService =
    new BluetoothGattService(aWindow, isPrimary, uuid, instanceId, connId);

  return gattService.forget();
}

JSObject*
BluetoothGattService::WrapObject(JSContext* aContext)
{
  return BluetoothGattServiceBinding::Wrap(aContext, this);
}

void
BluetoothGattService::AppendCharacteristic(nsAString& aUuid,
                                           int aInstanceId,
                                           int aClientIf,
                                           nsAString& aDeviceAddr)
{
  nsRefPtr<BluetoothGattCharacteristic> characteristic =
    BluetoothGattCharacteristic::Create(
      GetParentObject(), aUuid, aInstanceId, aClientIf,
      mUuid, mInstanceId, mIsPrimary, aDeviceAddr);
  NS_ENSURE_TRUE_VOID(characteristic);

  mCharacteristics.AppendElement(characteristic);
}

already_AddRefed<BluetoothGattCharacteristic>
BluetoothGattService::FindCharacteristic(const nsAString& aUuid,
                                         int aInstanceId)
{
  for (uint32_t i = 0; i < mCharacteristics.Length(); i++) {
    nsString uuid;
    mCharacteristics[i]->GetUuid(uuid);

    if (uuid.Equals(aUuid) &&
        mCharacteristics[i]->InstanceId() == aInstanceId) {
      nsRefPtr<BluetoothGattCharacteristic> characteristic =
        mCharacteristics[i];
      return characteristic.forget();
    }
  }
  return nullptr;
}
