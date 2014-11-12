/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothGatt.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"
#include "BluetoothUtils.h"
#include "nsIUUIDGenerator.h"
#include "nsServiceManagerUtils.h"

#include "mozilla/dom/bluetooth/BluetoothTypes.h"
#include "mozilla/dom/BluetoothGattBinding.h"
#include "mozilla/dom/Promise.h"

using namespace mozilla;
using namespace mozilla::dom;

USING_BLUETOOTH_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_INHERITED(BluetoothGatt,
                                   DOMEventTargetHelper,
                                   mConnectGattRunnable)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BluetoothGatt)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BluetoothGatt, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BluetoothGatt, DOMEventTargetHelper)

BluetoothGatt::BluetoothGatt(nsPIDOMWindow* aWindow,
                             const nsAString& aAppUuid,
                             bool aAutoConnect,
                             const nsAString& aDeviceAddr,
                             BluetoothReplyRunnable* aRunnable,
                             BluetoothService* aService)
  : DOMEventTargetHelper(aWindow)
  , mAppUuid(aAppUuid)
  , mAutoConnect(aAutoConnect)
  , mConnectionState(BluetoothConnectionState::Connecting)
  , mConnectGattRunnable(aRunnable)
  , mDeviceAddr(aDeviceAddr)
{
  BT_API2_LOGR();

  MOZ_ASSERT(aWindow);
  MOZ_ASSERT(!mAppUuid.IsEmpty());
  MOZ_ASSERT(!mDeviceAddr.IsEmpty());
  MOZ_ASSERT(aRunnable);
  MOZ_ASSERT(aService);

  aService->RegisterBluetoothSignalHandler(mAppUuid, this);

  /**
   * Register the app client to bluedroid.
   * A ClientRegistered signal indicating the register result will be
   * received later.
   */
  aService->RegisterGattClientInternal(mAppUuid, aRunnable);
}

BluetoothGatt::~BluetoothGatt()
{
  UnregisterClient();
  BluetoothService* bs = BluetoothService::Get();
  // bs can be null on shutdown, where destruction might happen.
  NS_ENSURE_TRUE_VOID(bs);
  bs->UnregisterBluetoothSignalHandler(mAppUuid, this);
}

void
BluetoothGatt::UnregisterClient()
{
  NS_ENSURE_TRUE_VOID(mClientIf > 0);
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  nsRefPtr<BluetoothVoidReplyRunnable> results =
    new BluetoothVoidReplyRunnable(nullptr);
  bs->UnregisterGattClientInternal(mClientIf, results);
}

void
BluetoothGatt::DisconnectFromOwner()
{
  BT_API2_LOGR();
  UnregisterClient();
  DOMEventTargetHelper::DisconnectFromOwner();

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);
  bs->UnregisterBluetoothSignalHandler(mAppUuid, this);
}

// static
already_AddRefed<BluetoothGatt>
BluetoothGatt::Create(nsPIDOMWindow* aWindow,
                      bool aAutoConnect,
                      const nsAString& aDeviceAddr,
                      BluetoothReplyRunnable* aRunnable)
{
  BT_API2_LOGR("BluetoothGatt::Create");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aWindow);

  NS_ENSURE_TRUE(aRunnable, nullptr);

  // Generate a random UUID for this gatt client
  nsresult rv;
  nsCOMPtr<nsIUUIDGenerator> uuidGenerator =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, nullptr);

  nsID uuid;
  rv = uuidGenerator->GenerateUUIDInPlace(&uuid);
  NS_ENSURE_SUCCESS(rv, nullptr);

  // Get the UUID string from generated id
  char uuidBuffer[NSID_LENGTH];
  uuid.ToProvidedString(uuidBuffer);
  NS_ConvertASCIItoUTF16 uuidString(uuidBuffer);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE(bs, nullptr);

  nsRefPtr<BluetoothGatt> gatt =
    new BluetoothGatt(aWindow, Substring(uuidString, 1, NSID_LENGTH - 3),
                      aAutoConnect, aDeviceAddr, aRunnable, bs);

  return gatt.forget();
}

already_AddRefed<Promise>
BluetoothGatt::Connect(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  BT_ENSURE_TRUE_REJECT(
    mConnectionState == BluetoothConnectionState::Disconnected,
    NS_ERROR_DOM_INVALID_STATE_ERR);

  BluetoothService* bs = BluetoothService::Get();
  BT_ENSURE_TRUE_REJECT(bs, NS_ERROR_NOT_AVAILABLE);

  UpdateConnectionState(BluetoothConnectionState::Connecting);
  nsRefPtr<BluetoothReplyRunnable> result =
    new BluetoothVoidReplyRunnable(nullptr /* DOMRequest */,
                                   promise,
                                   NS_LITERAL_STRING("ConnectGattClient"));
  bs->ConnectGattClientInternal(mClientIf,
                                mDeviceAddr,
                                false, // aIsDirect
                                result);

  return promise.forget();
}

already_AddRefed<Promise>
BluetoothGatt::Disconnect(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  BT_ENSURE_TRUE_REJECT(
    mConnectionState == BluetoothConnectionState::Connected,
    NS_ERROR_DOM_INVALID_STATE_ERR);

  BluetoothService* bs = BluetoothService::Get();
  BT_ENSURE_TRUE_REJECT(bs, NS_ERROR_NOT_AVAILABLE);

  UpdateConnectionState(BluetoothConnectionState::Disconnecting);
  nsRefPtr<BluetoothReplyRunnable> result =
    new BluetoothVoidReplyRunnable(nullptr /* DOMRequest */,
                                   promise,
                                   NS_LITERAL_STRING("DisconnectGattClient"));
  bs->DisconnectGattClientInternal(mClientIf, mDeviceAddr, result);

  return promise.forget();
}

void
BluetoothGatt::UpdateConnectionState(BluetoothConnectionState aState)
{
  BT_API2_LOGR("new state: %d", int(aState));
  mConnectionState = aState;

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  NS_ENSURE_SUCCESS_VOID(rv);

  rv = event->InitEvent(NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
                        false,
                        false);
  NS_ENSURE_SUCCESS_VOID(rv);

  DispatchTrustedEvent(event);
}

void
BluetoothGatt::HandleClientRegistered(const BluetoothValue& aValue)
{
  MOZ_ASSERT(aValue.type() == BluetoothValue::Tuint32_t);
  mClientIf = aValue.get_uint32_t();
  BT_API2_LOGR("clientIf = %d", mClientIf);

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  bs->ConnectGattClientInternal(mClientIf,
                                mDeviceAddr,
                                !mAutoConnect,
                                mConnectGattRunnable);
}

void
BluetoothGatt::Notify(const BluetoothSignal& aData)
{
  BT_LOGD("[D] %s", NS_ConvertUTF16toUTF8(aData.name()).get());

  BluetoothValue v = aData.value();
  if (aData.name().EqualsLiteral("ClientRegistered")) {
    HandleClientRegistered(v);
  } else if (aData.name().EqualsLiteral(GATT_CONNECTION_STATUS_CHANGED_ID)) {
    MOZ_ASSERT(v.type() == BluetoothValue::Tbool);

    BluetoothConnectionState state =
      v.get_bool() ? BluetoothConnectionState::Connected
                   : BluetoothConnectionState::Disconnected;
    UpdateConnectionState(state);
  } else {
    BT_WARNING("Not handling device signal: %s",
               NS_ConvertUTF16toUTF8(aData.name()).get());
  }
}

JSObject*
BluetoothGatt::WrapObject(JSContext* aContext)
{
  return BluetoothGattBinding::Wrap(aContext, this);
}
