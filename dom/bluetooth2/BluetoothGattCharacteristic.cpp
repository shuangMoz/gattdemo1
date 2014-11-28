/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothGattCharacteristic.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothGattCharacteristicBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/HoldDropJSObjects.h"

#include "nsIGlobalObject.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothGattCharacteristic)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(BluetoothGattCharacteristic)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mValue)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(BluetoothGattCharacteristic)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDescriptors)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(BluetoothGattCharacteristic)
  tmp->Unroot();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattCharacteristic)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattCharacteristic)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattCharacteristic)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothGattCharacteristic::BluetoothGattCharacteristic(
  nsPIDOMWindow* aOwner, const nsAString& aUuid,
  int aInstanceId, int aClientIf, const nsAString& aServiceUuid,
  int aServiceInstanceId, bool aIsPrimary, const nsAString& aDeviceAddr)
  : mOwner(aOwner)
  , mUuid(aUuid)
  , mInstanceId(aInstanceId)
  , mClientIf(aClientIf)
  , mServiceUuid(aServiceUuid)
  , mServiceInstanceId(aServiceInstanceId)
  , mIsPrimary(aIsPrimary)
  , mDeviceAddr(aDeviceAddr)
{
  BT_API2_LOGR();

  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(!mUuid.IsEmpty());
  MOZ_ASSERT(!mServiceUuid.IsEmpty());
  MOZ_ASSERT(!mDeviceAddr.IsEmpty());
}

BluetoothGattCharacteristic::~BluetoothGattCharacteristic()
{
}

// static
already_AddRefed<BluetoothGattCharacteristic>
BluetoothGattCharacteristic::Create(nsPIDOMWindow* aWindow,
                                    const nsAString& aUuid,
                                    int aInstanceId,
                                    int aClientIf,
                                    const nsAString& aServiceUuid,
                                    int aServiceInstanceId,
                                    bool aIsPrimary,
                                    const nsAString& aDeviceAddr)
{
  BT_API2_LOGR("BluetoothGattCharacteristic::Create");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothGattCharacteristic> gattCharacteristic =
    new BluetoothGattCharacteristic(aWindow, aUuid, aInstanceId,
                                    aClientIf, aServiceUuid,
                                    aServiceInstanceId, aIsPrimary,
                                    aDeviceAddr);

  return gattCharacteristic.forget();
}

void
BluetoothGattCharacteristic::Root()
{
  if (mIsRooted) {
    return;
  }
  mozilla::HoldJSObjects(this);
  mIsRooted = true;
}

void
BluetoothGattCharacteristic::Unroot()
{
  if (!mIsRooted) {
    return;
  }
  mValue = nullptr;
  mozilla::DropJSObjects(this);
  mIsRooted = false;
}

JSObject*
BluetoothGattCharacteristic::WrapObject(JSContext* aContext)
{
  return BluetoothGattCharacteristicBinding::Wrap(aContext, this);
}

already_AddRefed<Promise>
BluetoothGattCharacteristic::StartNotifications(ErrorResult& aRv)
{
  BT_API2_LOGR();

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  BluetoothService* bs = BluetoothService::Get();
  BT_ENSURE_TRUE_REJECT(bs, NS_ERROR_NOT_AVAILABLE);

  nsRefPtr<BluetoothReplyRunnable> result =
    new BluetoothVoidReplyRunnable(nullptr /* DOMRequest */,
                                   promise,
                                   NS_LITERAL_STRING("StartNotification"));

  bs->StartNotificationsInternal(mClientIf, mDeviceAddr,
                                 mServiceUuid, mServiceInstanceId, mIsPrimary,
                                 mUuid, mInstanceId, result);

  return promise.forget();
}

void
BluetoothGattCharacteristic::AppendDescriptor(const nsAString& aUuid,
                                              int aInstanceId,
                                              int aConnId)
{
  nsRefPtr<BluetoothGattDescriptor> descriptor =
    BluetoothGattDescriptor::Create(
      GetParentObject(), aUuid, aInstanceId, aConnId,
      mServiceUuid, mServiceInstanceId, mIsPrimary,
      mUuid, mInstanceId // characteristic id
      );
  NS_ENSURE_TRUE_VOID(descriptor);

  mDescriptors.AppendElement(descriptor);
}
