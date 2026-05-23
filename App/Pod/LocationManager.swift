//
//  LocationManager.swift
//  Pod
//
//  Created by Joshua Aston-Adams on 20/05/2026.
//

import Foundation
import CoreLocation

class LocationManager: NSObject, ObservableObject, CLLocationManagerDelegate {
    @Published var currentLocation: CLLocation?
    @Published var status = "Location not ready"

    private let manager = CLLocationManager()

    override init() {
        super.init()

        manager.delegate = self
        manager.desiredAccuracy = kCLLocationAccuracyBest
        manager.requestWhenInUseAuthorization()
        manager.startUpdatingLocation()
    }

    func locationManager(_ manager: CLLocationManager,
                         didUpdateLocations locations: [CLLocation]) {
        guard let location = locations.last else { return }

        DispatchQueue.main.async {
            self.currentLocation = location
            self.status = "Location ready"
        }
    }

    func locationManager(_ manager: CLLocationManager,
                         didFailWithError error: Error) {
        DispatchQueue.main.async {
            self.status = "Location error: \(error.localizedDescription)"
        }
    }
}
