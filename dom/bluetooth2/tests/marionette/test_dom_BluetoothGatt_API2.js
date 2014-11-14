/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

///////////////////////////////////////////////////////////////////////////////
// Test Purpose:
//   To verify entire connect/disconnect process of BluetoothGatt.
//   Testers have to put a discoverable remote device near the testing device
//   and click the 'confirm' button on remote device when it receives a pairing
//   request from testing device.
//   To pass this test, Bluetooth address of the remote device should equal to
//   ADDRESS_OF_TARGETED_REMOTE_DEVICE.
//
// Test Procedure:
//   [0] Set Bluetooth permission and enable default adapter.
//   [1] Start LE scan.
//   [2] Wait for the specified 'devicefound' event.
//   [3] Type checking for BluetoothDeviceEvent and BluetoothDevice.
//   [4] Connect to remote gatt server.
//
// Test Coverage:
//   - BluetoothDevice.connectGatt()
//
//   - BluetoothGatt.connect()
//   - BluetoothGatt.disconnect()
//
///////////////////////////////////////////////////////////////////////////////

MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

const ADDRESS_OF_TARGETED_REMOTE_DEVICE = "7d:23:48:ac:b4:15";
const NAME_OF_TARGETED_REMOTE_DEVICE = "Jocelyn LE test";
//const NAME_OF_TARGETED_REMOTE_DEVICE = "Polar H7 2AA27B"

function waitForSpecificLeDeviceFoundByName(aDiscoveryHandle, aNames) {
  let deferred = Promise.defer();

  ok(aDiscoveryHandle instanceof BluetoothDiscoveryHandle,
    "aDiscoveryHandle should be a BluetoothDiscoveryHandle");

  let devicesArray = [];
  aDiscoveryHandle.ondevicefound = function onDeviceFound(aEvent) {
    ok(aEvent instanceof BluetoothDeviceEvent,
      "aEvent should be a BluetoothDeviceEvent");
    log("  === found addr = " + aEvent.device.address);
    log("  === found name = " + aEvent.device.name);
    if (aNames.indexOf(aEvent.device.name) != -1) {
      devicesArray.push(aEvent);
    }
    if (devicesArray.length == aNames.length) {
      aDiscoveryHandle.ondevicefound = null;
      ok(true, "BluetoothAdapter has found all remote devices.");
      deferred.resolve(devicesArray);
    }
  };

  return deferred.promise;
}

startBluetoothTest(false, function testCaseMain(aAdapter) {
  log("Checking adapter attributes ...");

  is(aAdapter.state, "enabled", "adapter.state");
  isnot(aAdapter.address, "", "adapter.address");

  // Since adapter has just been re-enabled, these properties should be 'false'.
  is(aAdapter.discovering, false, "adapter.discovering");
  is(aAdapter.discoverable, false, "adapter.discoverable");

  log("adapter.address: " + aAdapter.address);
  log("adapter.name: " + aAdapter.name);

  return Promise.resolve()
    .then(function() {
      log("[1] Start LE scan ... ");
      return aAdapter.startDiscovery();
    })
    .then(function(discoveryHandle) {
      log("[2] Wait for the specified 'devicefound' event ... ");
      return waitForSpecificLeDeviceFoundByName(discoveryHandle, [NAME_OF_TARGETED_REMOTE_DEVICE]);
    })
    .then(function(deviceEvents) {
      log("[3] Type checking for BluetoothDeviceEvent and BluetoothDevice ... ");

      let device = deviceEvents[0].device;
      ok(deviceEvents[0] instanceof BluetoothDeviceEvent, "device should be a BluetoothDeviceEvent");
      ok(device instanceof BluetoothDevice, "device should be a BluetoothDevice");

      log("  - BluetoothDevice.address: " + device.address);
      log("  - BluetoothDevice.name: " + device.name);
      log("  - BluetoothDevice.cod: " + device.cod);
      log("  - BluetoothDevice.paired: " + device.paired);
      log("  - BluetoothDevice.uuids: " + device.uuids);
      log("  - BluetoothDevice.type:  " + device.type);

      return device;
    })
    .then(function(device) {
      log("[4] Connect to remote gatt server ... ");
      return device.connectGatt(false);
/*    })
    .then(function(gatt) {
      is(gatt.connectionState, "connected", "gatt.connectionState");
      gatt.onconnectionstatechanged = function onConnectionStateChanged(evt) {
        log("  - Received 'onconnectionstatechanged' event ");
        log("gatt.connectionState = " + gatt.connectionState);
        //is(gatt.connectionState, "disconnected", "gatt.connectionState");
      }
      return gatt.disconnect();
*/
    });
});
