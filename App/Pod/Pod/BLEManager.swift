//
//  BLEManager.swift
//  Pod
//
//  Created by Joshua Aston-Adams on 03/05/2026.
//

import Foundation
import CoreBluetooth

class BLEManager: NSObject, ObservableObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    @Published var isConnected = false
    @Published var status = "Not connected"

    private var centralManager: CBCentralManager!
    private var gaugePeripheral: CBPeripheral?
    private var settingsCharacteristic: CBCharacteristic?

    private let serviceUUID = CBUUID(string: "9f1c0d2a-2b5a-4e2b-8d37-0a4e1f6b9001")
    private let settingsCharUUID = CBUUID(string: "9f1c0d2a-2b5a-4e2b-8d37-0a4e1f6b9002")

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            status = "Scanning..."
            centralManager.scanForPeripherals(withServices: [serviceUUID])
        } else {
            status = "Bluetooth unavailable"
        }
    }

    func centralManager(_ central: CBCentralManager,
                        didDiscover peripheral: CBPeripheral,
                        advertisementData: [String : Any],
                        rssi RSSI: NSNumber) {
        status = "Found gauge"

        gaugePeripheral = peripheral
        gaugePeripheral?.delegate = self

        centralManager.stopScan()
        centralManager.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager,
                        didConnect peripheral: CBPeripheral) {
        status = "Connected"
        isConnected = true
        peripheral.discoverServices([serviceUUID])
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverServices error: Error?) {
        guard let services = peripheral.services else { return }

        for service in services {
            peripheral.discoverCharacteristics([settingsCharUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral,
                    didDiscoverCharacteristicsFor service: CBService,
                    error: Error?) {
        guard let characteristics = service.characteristics else { return }

        for characteristic in characteristics {
            if characteristic.uuid == settingsCharUUID {
                settingsCharacteristic = characteristic
                status = "Ready"
            }
        }
    }

    func sendGaugeSettings(bg: Int, measure: Int, needle: Int) {
        guard let peripheral = gaugePeripheral,
              let characteristic = settingsCharacteristic else {
            status = "Not ready"
            return
        }

        let json = "SETTINGS:{\"bg\":\(bg),\"measure\":\(measure),\"needle\":\(needle)}"

        guard let data = json.data(using: .utf8) else { return }

        peripheral.writeValue(data, for: characteristic, type: .withResponse)
        status = "Sent \(json)"
    }
    
    func sendText(_ text: String) {
        guard let peripheral = gaugePeripheral,
              let characteristic = settingsCharacteristic else {
            status = "Not ready"
            return
        }

        guard let data = text.data(using: .utf8) else {
            status = "Encode failed"
            return
        }

        peripheral.writeValue(data, for: characteristic, type: .withResponse)
        status = "Sent: \(text.prefix(20))"
    }

    func sendRouteChunks(_ chunks: [String]) {
        guard isConnected, settingsCharacteristic != nil else {
            status = "BLE not ready"
            return
        }

        sendText("ROUTE_START:\(chunks.count)")
        Thread.sleep(forTimeInterval: 0.12)

        for (index, chunk) in chunks.enumerated() {
            sendText("ROUTE_CHUNK:\(index):\(chunk)")
            
            Thread.sleep(forTimeInterval: 0.12)
        }

        sendText("ROUTE_END")
        Thread.sleep(forTimeInterval: 0.12)
        status = "Route sent: \(chunks.count) chunks"
    }
}

