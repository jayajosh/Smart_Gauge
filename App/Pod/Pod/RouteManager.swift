//
//  RouteManager.swift
//  Pod
//
//  Created by Joshua Aston-Adams on 11/05/2026.
//

import Foundation
import MapKit

class MapRouteManager: NSObject, ObservableObject {
    @Published var route: MKRoute?
    @Published var status = "No Route"
    
    func getRouteCoordinates(from route: MKRoute) -> [CLLocationCoordinate2D] {
        let pointCount = route.polyline.pointCount
        var coordinates = Array(repeating: CLLocationCoordinate2D(), count: pointCount)
        
        route.polyline.getCoordinates(&coordinates, range: NSRange(location: 0, length: pointCount))
        
        return coordinates
    }
    
    func calculateRoute(from start: CLLocationCoordinate2D, to destination: CLLocationCoordinate2D, ble: BLEManager) {
        let request = MKDirections.Request()
        request.source = MKMapItem(placemark: MKPlacemark(coordinate: start))
        request.destination = MKMapItem(placemark: MKPlacemark(coordinate: destination))
        request.transportType = .automobile
        request.requestsAlternateRoutes = true
        
        let directions = MKDirections(request: request)
        
        directions.calculate {response, error in
            if let error = error {
                DispatchQueue.main.async{
                    self.status = "Route Error: \(error.localizedDescription)"
                }
                return
            }
            
            guard let route = response?.routes.first else {
                DispatchQueue.main.async{
                    self.status = "No Route Found"
                }
                return
            }
            
            DispatchQueue.main.async {
                self.route = route
                self.status = "Route Ready"
                
                let coords = self.getRouteCoordinates(from: route)
                
                let simplified = coords
                
                let routeString = self.routeToString(simplified)
                
                let chunks = self.chunkString(routeString, maxBytes: 100)
                
                ble.sendRouteChunks(chunks)
            }
        }
    }
    
    func simplifyRoute(_ coords: [CLLocationCoordinate2D], step: Int = 5) -> [CLLocationCoordinate2D] {
        guard coords.count > step else {return coords}
        
        var simplified: [CLLocationCoordinate2D] = []
        
        for i in stride(from:0, to: coords.count, by: step) {
            simplified.append(coords[i])
        }
        
        if let last = coords.last {
            simplified.append(last)
        }
        
        return simplified
    }
    
    func routeToString(_ coords: [CLLocationCoordinate2D]) -> String {
        coords.map {
            String(format: "%.6f,%.6f", $0.latitude, $0.longitude)
        }.joined(separator: ";")
    }
    
    func chunkString(_ string: String, maxBytes: Int = 100) -> [String] {
        var chunks: [String] = []
        var current = ""
        
        for char in string {
            let test = current + String(char)
            
            if test.utf8.count > maxBytes {
                chunks.append(current)
                current = String(char)
            }
            else {
                current = test
            }
        }
        
        if !current.isEmpty {
            chunks.append(current)
        }
        
        return chunks
    }
}
