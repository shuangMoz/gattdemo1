/* -*- Mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; tab-width: 40 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

[CheckPermissions="bluetooth"]
interface BluetoothGatt : EventTarget
{
  readonly attribute BluetoothConnectionState       connectionState;
  [Cached, Pure]
  readonly attribute sequence<BluetoothGattService> services;
  // Fired when the connection state attribute changed
           attribute EventHandler                   onconnectionstatechanged;

  [NewObject, Throws]
  Promise<void>                                     connect();
  [NewObject, Throws]
  Promise<void>                                     disconnect();
};

enum BluetoothConnectionState
{
  "disconnected",
  "disconnecting",
  "connected",
  "connecting"
};
