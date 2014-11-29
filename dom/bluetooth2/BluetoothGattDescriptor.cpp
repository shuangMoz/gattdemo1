/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothGattDescriptor.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothGattDescriptorBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/HoldDropJSObjects.h"

#include "nsIGlobalObject.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_CLASS(BluetoothGattDescriptor)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(BluetoothGattDescriptor)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mValue)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(BluetoothGattDescriptor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(BluetoothGattDescriptor)
  tmp->Unroot();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(BluetoothGattDescriptor)
NS_IMPL_CYCLE_COLLECTING_RELEASE(BluetoothGattDescriptor)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(BluetoothGattDescriptor)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

BluetoothGattDescriptor::BluetoothGattDescriptor(
  nsPIDOMWindow* aOwner, const nsAString& aUuid,
  int aInstanceId, int aConnId, const nsAString& aServiceUuid,
  int aServiceInstanceId, bool aIsPrimary,
  const nsAString& aCharUuid, int aCharInstanceId)
  : mOwner(aOwner)
  , mUuid(aUuid)
  , mInstanceId(aInstanceId)
  , mConnId(aConnId)
  , mServiceUuid(aServiceUuid)
  , mServiceInstanceId(aServiceInstanceId)
  , mIsPrimary(aIsPrimary)
  , mCharUuid(aCharUuid)
  , mCharInstanceId(aCharInstanceId)
{
  BT_API2_LOGR();

  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(!mUuid.IsEmpty());
  MOZ_ASSERT(!mServiceUuid.IsEmpty());
  MOZ_ASSERT(!mCharUuid.IsEmpty());
}

BluetoothGattDescriptor::~BluetoothGattDescriptor()
{
}

// static
already_AddRefed<BluetoothGattDescriptor>
BluetoothGattDescriptor::Create(nsPIDOMWindow* aWindow,
                                    const nsAString& aUuid,
                                    int aInstanceId,
                                    int aConnId,
                                    const nsAString& aServiceUuid,
                                    int aServiceInstanceId,
                                    bool aIsPrimary,
                                    const nsAString& aCharUuid,
                                    int aCharInstanceId)
{
  BT_API2_LOGR("BluetoothGattDescriptor::Create");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  nsRefPtr<BluetoothGattDescriptor> gattDescriptor =
    new BluetoothGattDescriptor(aWindow, aUuid, aInstanceId,
                                aConnId, aServiceUuid,
                                aServiceInstanceId, aIsPrimary,
                                aCharUuid, aCharInstanceId);

  return gattDescriptor.forget();
}

void
BluetoothGattDescriptor::Root()
{
  if (mIsRooted) {
    return;
  }
  mozilla::HoldJSObjects(this);
  mIsRooted = true;
}

void
BluetoothGattDescriptor::Unroot()
{
  if (!mIsRooted) {
    return;
  }
  mValue = nullptr;
  mozilla::DropJSObjects(this);
  mIsRooted = false;
}

JSObject*
BluetoothGattDescriptor::WrapObject(JSContext* aContext)
{
  return BluetoothGattDescriptorBinding::Wrap(aContext, this);
}

already_AddRefed<Promise>
BluetoothGattDescriptor::WriteValue(const ArrayBuffer& aValue, ErrorResult& aRv)
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
                                   NS_LITERAL_STRING("WriteDescriptor"));

  int authType = 0; // AUTHENTICATION_NONE
  int writeType = 0x02; // WRITE_TYPE_DEFAULT
  int len = aValue.Length();
  nsCString a = nsCString(reinterpret_cast<const char*>(aValue.Data()));
  BT_API2_LOGR("array length: %d", len);
  /*TODO: Handle WriteType */
  bs->WriteDescriptorInternal(mConnId,
                              mServiceUuid, mServiceInstanceId, mIsPrimary,
                              mCharUuid, mCharInstanceId,
                              mUuid, mInstanceId, writeType, len, authType,
                              a,
                              result);

  return promise.forget();
}
