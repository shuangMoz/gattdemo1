/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "BluetoothGattManager.h"
#include "BluetoothCommon.h"
#include "BluetoothUtils.h"
#include "BluetoothInterface.h"
#include "BluetoothReplyRunnable.h"
#include "BluetoothService.h"

#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "MainThreadUtils.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"

#define ENSURE_GATT_CLIENT_IF_IS_READY_VOID(runnable)                         \
  do {                                                                        \
    if (!sBluetoothGattInterface) {                                           \
      NS_NAMED_LITERAL_STRING(errorStr,                                       \
                              "BluetoothGattClientInterface is not ready");   \
      DispatchBluetoothReply(runnable, BluetoothValue(), errorStr);           \
      return;                                                                 \
    }                                                                         \
  } while(0)

using namespace mozilla;
USING_BLUETOOTH_NAMESPACE

class BluetoothClient;

StaticRefPtr<BluetoothGattManager> sBluetoothGattManager;
static BluetoothGattInterface* sBluetoothGattInterface;
static BluetoothGattClientInterface* sBluetoothGattClientInterface;

bool BluetoothGattManager::mInShutdown = false;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sRegisterClientRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sConnectRunnableArray;
static nsTArray<nsRefPtr<BluetoothReplyRunnable> > sDisconnectRunnableArray;

static nsTArray<BluetoothClient> sClients;

class BluetoothClient
{
public:
  BluetoothClient(const nsAString& aAppUuid)
  : mAppUuid(aAppUuid)
  , mClientIf(-1)
  , mConnId(-1)
  { }

  nsString mAppUuid;
  int mClientIf;
  int mConnId;
};

/*
 * Static functions
 */
static int
GetClientIndex(const nsAString& aAppUuid)
{
  MOZ_ASSERT(!aAppUuid.IsEmpty());

  for (uint8_t i = 0; i < sClients.Length(); i++) {
    if (aAppUuid.Equals(sClients[i].mAppUuid)) {
      return i;
    }
  }

  BT_API2_LOGR("Cannot find the bluetooth client");
  return -1;
}

static int
GetClientIndex(int aClientIf)
{
  for (uint8_t i = 0; i < sClients.Length(); i++) {
    if (aClientIf == sClients[i].mClientIf) {
      return i;
    }
  }

  BT_API2_LOGR("Cannot find the bluetooth client");
  return -1;
}

static int
GetClientIndexByConnId(int aConnId)
{
  for (uint8_t i = 0; i < sClients.Length(); i++) {
    if (aConnId == sClients[i].mConnId) {
      return i;
    }
  }

  BT_API2_LOGR("Cannot find the bluetooth client");
  return -1;
}

BluetoothGattManager*
BluetoothGattManager::Get()
{
  MOZ_ASSERT(NS_IsMainThread());

  // If sBluetoothGattManager already exists, exit early
  if (sBluetoothGattManager) {
    return sBluetoothGattManager;
  }

  // If we're in shutdown, don't create a new instance
  NS_ENSURE_FALSE(mInShutdown, nullptr);

  // Create a new instance, register, and return
  BluetoothGattManager* manager = new BluetoothGattManager();
  sBluetoothGattManager = manager;
  return sBluetoothGattManager;
}

class InitGattResultHandler MOZ_FINAL : public BluetoothGattResultHandler
{
public:
  InitGattResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattInterface::Init failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Init() MOZ_OVERRIDE
  {
    if (mRes) {
      mRes->Init();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

// static
void
BluetoothGattManager::InitGattInterface(BluetoothProfileResultHandler* aRes)
{
  BluetoothInterface* btInf = BluetoothInterface::GetInstance();
  if (!btInf) {
    BT_LOGR("Error: Bluetooth interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  sBluetoothGattInterface = btInf->GetBluetoothGattInterface();
  if (!sBluetoothGattInterface) {
    BT_LOGR("Error: Bluetooth GATT interface not available");
    if (aRes) {
      aRes->OnError(NS_ERROR_FAILURE);
    }
    return;
  }

  sBluetoothGattClientInterface =
    sBluetoothGattInterface->GetBluetoothGattClientInterface();
  NS_ENSURE_TRUE_VOID(sBluetoothGattClientInterface);

  BluetoothGattManager* gattManager = BluetoothGattManager::Get();
  sBluetoothGattInterface->Init(gattManager,
                                new InitGattResultHandler(aRes));
}

class CleanupResultHandler MOZ_FINAL : public BluetoothGattResultHandler
{
public:
  CleanupResultHandler(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  { }

  void OnError(BluetoothStatus aStatus) MOZ_OVERRIDE
  {
    BT_WARNING("BluetoothGattInterface::Cleanup failed: %d",
               (int)aStatus);
    if (mRes) {
      mRes->OnError(NS_ERROR_FAILURE);
    }
  }

  void Cleanup() MOZ_OVERRIDE
  {
    sBluetoothGattClientInterface = nullptr;
    sBluetoothGattInterface = nullptr;

    sRegisterClientRunnableArray.Clear();
    sConnectRunnableArray.Clear();
    sDisconnectRunnableArray.Clear();

    if (mRes) {
      mRes->Deinit();
    }
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

class CleanupResultHandlerRunnable MOZ_FINAL : public nsRunnable
{
public:
  CleanupResultHandlerRunnable(BluetoothProfileResultHandler* aRes)
  : mRes(aRes)
  {
    MOZ_ASSERT(mRes);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mRes->Deinit();
    return NS_OK;
  }

private:
  nsRefPtr<BluetoothProfileResultHandler> mRes;
};

// static
void
BluetoothGattManager::DeinitGattInterface(BluetoothProfileResultHandler* aRes)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (sBluetoothGattInterface) {
    sBluetoothGattInterface->Cleanup(new CleanupResultHandler(aRes));
  } else if (aRes) {
    // We dispatch a runnable here to make the profile resource handler
    // behave as if GATT was initialized.
    nsRefPtr<nsRunnable> r = new CleanupResultHandlerRunnable(aRes);
    if (NS_FAILED(NS_DispatchToMainThread(r))) {
      BT_LOGR("Failed to dispatch cleanup-result-handler runnable");
    }
  }
}

class RegisterClientResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  RegisterClientResultHandler(BluetoothReplyRunnable* aReply,
                              const nsAString& aAppUuid)
  : mReply(aReply)
  , mAppUuid(aAppUuid)
  { }

  void OnError(BluetoothStatus status) MOZ_FINAL
  {
    BT_API2_LOGR("register client failed");
    BluetoothService* bs = BluetoothService::Get();
    NS_ENSURE_TRUE_VOID(bs);

    int clientIndex = GetClientIndex(mAppUuid);
    if (clientIndex >= 0) {
      sClients.RemoveElementAt(clientIndex);
    }

    BluetoothSignal signal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
      mAppUuid,
      BluetoothValue(false)); // disconnected
    bs->DistributeSignal(signal);

    NS_NAMED_LITERAL_STRING(errorStr, "Register gatt client failed");
    DispatchBluetoothReply(mReply,
                           BluetoothValue(),
                           errorStr);
    sRegisterClientRunnableArray.RemoveElement(mReply);
  }
private:
  nsRefPtr<BluetoothReplyRunnable> mReply;
  nsString mAppUuid;
};

void
BluetoothGattManager::RegisterClient(const nsAString& aAppUuid,
                                     BluetoothReplyRunnable* aRunnable)
{
  BT_API2_LOGR("register client");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);
  MOZ_ASSERT(!aAppUuid.IsEmpty());

  ENSURE_GATT_CLIENT_IF_IS_READY_VOID(aRunnable);

  BluetoothClient client(aAppUuid);
  sClients.AppendElement(client);

  sRegisterClientRunnableArray.AppendElement(aRunnable);

  BluetoothUuid uuid;
  StringToUuid(NS_ConvertUTF16toUTF8(aAppUuid).get(), uuid);
  sBluetoothGattClientInterface->RegisterClient(
    uuid, new RegisterClientResultHandler(aRunnable, aAppUuid));
}

class UnregisterClientResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  UnregisterClientResultHandler(int aClientIf, BluetoothReplyRunnable* aReply)
  : mClientIf(aClientIf)
  , mReply(aReply)
  { }

  void UnregisterClient() MOZ_OVERRIDE
  {
    int clientIndex = GetClientIndex(mClientIf);
    if (clientIndex >= 0) {
      sClients.RemoveElementAt(clientIndex);
    }

    if (mReply) {
      DispatchBluetoothReply(mReply, BluetoothValue(true), EmptyString());
    }
  }

  void OnError(BluetoothStatus status) MOZ_OVERRIDE
  {
    if (mReply) {
      NS_NAMED_LITERAL_STRING(errorStr, "Unregister gatt client failed");
      DispatchBluetoothReply(mReply,
                             BluetoothValue(),
                             errorStr);
    }
  }

private:
  int mClientIf;
  nsRefPtr<BluetoothReplyRunnable> mReply;
};

void
BluetoothGattManager::UnregisterClient(int aClientIf,
                                       BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  ENSURE_GATT_CLIENT_IF_IS_READY_VOID(aRunnable);

  sBluetoothGattClientInterface->UnregisterClient(
    aClientIf, new UnregisterClientResultHandler(aClientIf, aRunnable));
}

class ConnectResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  void OnError(BluetoothStatus status) MOZ_FINAL
  {
    BT_API2_LOGR("Connect Gatt Client Error");
    // TODO: send disconnected signal and reply status error
  }
};

void
BluetoothGattManager::Connect(int aClientIf,
                              const nsAString& aDeviceAddr,
                              bool aIsDirect,
                              BluetoothReplyRunnable* aRunnable)
{
  BT_API2_LOGR("GATT CONNECT");
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aRunnable);

  ENSURE_GATT_CLIENT_IF_IS_READY_VOID(aRunnable);

  sConnectRunnableArray.AppendElement(aRunnable);
  sBluetoothGattClientInterface->Connect(
    aClientIf, aDeviceAddr, aIsDirect, new ConnectResultHandler());
}

class DisconnectResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  DisconnectResultHandler(BluetoothReplyRunnable* aReply)
  : mReply(aReply)
  { }
  void OnError(BluetoothStatus status) MOZ_FINAL
  {
    // log and reply status error
  }
private:
  nsRefPtr<BluetoothReplyRunnable> mReply;
};

void
BluetoothGattManager::Disconnect(int aClientIf,
                                 const nsAString& aDeviceAddr,
                                 BluetoothReplyRunnable* aRunnable)
{
  MOZ_ASSERT(NS_IsMainThread());
  ENSURE_GATT_CLIENT_IF_IS_READY_VOID(aRunnable);

  int clientIndex = GetClientIndex(aClientIf);
  if (clientIndex < 0) {
    // TODO: reject promise
    return;
  }

  sDisconnectRunnableArray.AppendElement(aRunnable);

  sBluetoothGattClientInterface->Disconnect(
    aClientIf, aDeviceAddr, sClients[clientIndex].mConnId,
    new DisconnectResultHandler(aRunnable));
}

//
// Notification Handlers
//
void
BluetoothGattManager::RegisterClientNotification(int aStatus,
                                                 int aClientIf,
                                                 const BluetoothUuid& aAppUuid)
{
  BT_API2_LOGR("clientIf = %d, aStatus=%d", aClientIf, aStatus);
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  nsString uuid;
  UuidToString(aAppUuid, uuid);

  BT_API2_LOGR("appUuid = %s", NS_ConvertUTF16toUTF8(uuid).get());

  int clientIndex = GetClientIndex(uuid);
  if (aStatus || clientIndex < 0) {
    BluetoothSignal signal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
      uuid,
      BluetoothValue(false)); // disconnected

    bs->DistributeSignal(signal);

    if (!sRegisterClientRunnableArray.IsEmpty()) {
      NS_NAMED_LITERAL_STRING(errorStr, "Register gatt client failed");
      DispatchBluetoothReply(sRegisterClientRunnableArray[0],
                             BluetoothValue(),
                             errorStr);
      sRegisterClientRunnableArray.RemoveElementAt(0);
    }

    UnregisterClient(aClientIf, nullptr);
    return;
  }

  sClients[clientIndex].mClientIf = aClientIf;

  BluetoothSignal signal(NS_LITERAL_STRING("ClientRegistered"),
                         uuid,
                         BluetoothValue(uint32_t(aClientIf)));
  bs->DistributeSignal(signal);

  if (!sRegisterClientRunnableArray.IsEmpty()) {
    sRegisterClientRunnableArray.RemoveElementAt(0);
  }
}

void
BluetoothGattManager::ScanResultNotification(
  const nsAString& aBdAddr, int aRssi,
  const BluetoothGattAdvData& aAdvData)
{ }

class SearchServiceResultHandler MOZ_FINAL
  : public BluetoothGattClientResultHandler
{
public:
  void OnError(BluetoothStatus status) MOZ_FINAL
  {
    BT_API2_LOGR("SearchService Error");
  }
};

void
BluetoothGattManager::ConnectNotification(int aConnId,
                                          int aStatus,
                                          int aClientIf,
                                          const nsAString& aDeviceAddr)
{
  BT_API2_LOGR("clientIf=%d, connId=%d", aClientIf, aConnId);
  MOZ_ASSERT(NS_IsMainThread());
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  int clientIndex = GetClientIndex(aClientIf);
  if (aStatus != 0 || clientIndex < 0) {
    BT_API2_LOGR("Connect failed with status = %d", aStatus);

    if (clientIndex >= 0) {
      BluetoothSignal signal(
        NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
        sClients[clientIndex].mAppUuid,
        BluetoothValue(false)); // disconnected
      bs->DistributeSignal(signal);
    }

    if (!sConnectRunnableArray.IsEmpty()) {
      NS_NAMED_LITERAL_STRING(errorStr, "Connect failed");
      DispatchBluetoothReply(sConnectRunnableArray[0],
                             BluetoothValue(),
                             errorStr);
      sConnectRunnableArray.RemoveElementAt(0);
    }

    return;
  }

  sClients[clientIndex].mConnId = aConnId;

  if (!sConnectRunnableArray.IsEmpty()) {
    DispatchBluetoothReply(sConnectRunnableArray[0],
                           BluetoothValue(true),
                           EmptyString());
    sConnectRunnableArray.RemoveElementAt(0);
  }

  BluetoothSignal signal(
    NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
    sClients[clientIndex].mAppUuid,
    BluetoothValue(true)); // connected
  bs->DistributeSignal(signal);

  // Retrieve all services
  sBluetoothGattClientInterface->SearchService(
    aConnId, BluetoothUuid(), new SearchServiceResultHandler());
}

void
BluetoothGattManager::DisconnectNotification(int aConnId,
                                             int aStatus,
                                             int aClientIf,
                                             const nsAString& aDeviceAddr)
{
  MOZ_ASSERT(NS_IsMainThread());

  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  int clientIndex = GetClientIndex(aClientIf);

  if (aStatus != 0 || clientIndex < 0) {
    // TODO: error handling
    BT_LOGD("Connect failed with status = %d, clientIndex = %d",
            aStatus, clientIndex);
    return;
  }

  if (clientIndex >= 0) {
    sClients[clientIndex].mConnId = -1;

    BluetoothSignal signal(
      NS_LITERAL_STRING(GATT_CONNECTION_STATUS_CHANGED_ID),
      sClients[clientIndex].mAppUuid,
      BluetoothValue(false)); // disconnected
    bs->DistributeSignal(signal);
  }

  if (!sDisconnectRunnableArray.IsEmpty()) {
    DispatchBluetoothReply(sDisconnectRunnableArray[0],
                           BluetoothValue(true),
                           EmptyString());
    sDisconnectRunnableArray.RemoveElementAt(0);
  }
}

void
BluetoothGattManager::SearchCompleteNotification(int aConnId, int aStatus)
{
  BT_API2_LOGR();
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  int clientIndex = GetClientIndexByConnId(aConnId);
  NS_ENSURE_TRUE_VOID(clientIndex >= 0);

  BluetoothSignal signal(NS_LITERAL_STRING("SearchCompleted"),
                         sClients[clientIndex].mAppUuid, BluetoothValue());
  bs->DistributeSignal(signal);
}

void
BluetoothGattManager::SearchResultNotification(
  int aConnId, const BluetoothGattServiceId& aServiceId)
{
  BT_API2_LOGR();
  BluetoothService* bs = BluetoothService::Get();
  NS_ENSURE_TRUE_VOID(bs);

  int clientIndex = GetClientIndexByConnId(aConnId);
  NS_ENSURE_TRUE_VOID(clientIndex >= 0);

  nsString uuidString;
  UuidToString(aServiceId.mId.mUuid, uuidString);

  BT_API2_LOGR("uuid = %s", NS_ConvertUTF16toUTF8(uuidString).get());
  BT_API2_LOGR("instance id = %d", aServiceId.mId.mInstanceId);
  BT_API2_LOGR("is_primary = %d", aServiceId.mIsPrimary);

  InfallibleTArray<BluetoothNamedValue> values;
  BT_APPEND_NAMED_VALUE(values, "UUID", uuidString);
  BT_APPEND_NAMED_VALUE(values, "InstanceId", (uint32_t)aServiceId.mId.mInstanceId);
  BT_APPEND_NAMED_VALUE(values, "IsPrimary", (uint32_t)aServiceId.mIsPrimary);

  // notify target BluetoothGatt object to create GattService
  BluetoothSignal signal(NS_LITERAL_STRING("ServiceDiscovered"),
                         sClients[clientIndex].mAppUuid, values);
  bs->DistributeSignal(signal);
}

void
BluetoothGattManager::GetCharacteristicNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  int aCharProperty)
{
  // notify service object by uuid + instanceId
}

void
BluetoothGattManager::GetDescriptorNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId,
  const BluetoothGattId& aDescriptorId)
{ }

void
BluetoothGattManager::GetIncludedServiceNotification(
  int aConnId, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattServiceId& aIncludedServId)
{ }

void
BluetoothGattManager::RegisterNotificationNotification(
  int aConnId, int aIsRegister, int aStatus,
  const BluetoothGattServiceId& aServiceId,
  const BluetoothGattId& aCharId)
{ }

void
BluetoothGattManager::NotifyNotification(
  int aConnId, const BluetoothGattNotifyParam& aNotifyParam)
{ }

void
BluetoothGattManager::ReadCharacteristicNotification(
  int aConnId, int aStatus, const BluetoothGattReadParam& aReadParam)
{ }

void
BluetoothGattManager::WriteCharacteristicNotification(
  int aConnId, int aStatus, const BluetoothGattWriteParam& aWriteParam)
{ }

void
BluetoothGattManager::ReadDescriptorNotification(
  int aConnId, int aStatus, const BluetoothGattReadParam& aReadParam)
{ }

void
BluetoothGattManager::WriteDescriptorNotification(
  int aConnId, int aStatus, const BluetoothGattWriteParam& aWriteParam)
{ }

void
BluetoothGattManager::ExecuteWriteNotification(int aConnId, int aStatus)
{ }

void
BluetoothGattManager::ReadRemoteRssiNotification(int aClientIf,
                                                 const nsAString& aBdAddr,
                                                 int aRssi,
                                                 int aStatus)
{ }

void
BluetoothGattManager::ListenNotification(int aStatus,
                                         int aServerIf)
{ }

BluetoothGattManager::BluetoothGattManager()
{ }

BluetoothGattManager::~BluetoothGattManager()
{
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  NS_ENSURE_TRUE_VOID(obs);
  if (NS_FAILED(obs->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID))) {
    BT_WARNING("Failed to remove shutdown observer!");
  }
}

NS_IMETHODIMP
BluetoothGattManager::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const char16_t* aData)
{
  MOZ_ASSERT(sBluetoothGattManager);

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    HandleShutdown();
    return NS_OK;
  }

  MOZ_ASSERT(false, "BluetoothGattManager got unexpected topic!");
  return NS_ERROR_UNEXPECTED;
}

void
BluetoothGattManager::HandleShutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  mInShutdown = true;
  sBluetoothGattManager = nullptr;
}

NS_IMPL_ISUPPORTS(BluetoothGattManager, nsIObserver)
