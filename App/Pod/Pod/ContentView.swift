//
//  ContentView.swift
//  Pod
//
//  Created by Joshua Aston-Adams on 26/04/2026.
//

import SwiftUI
import MapKit

enum GaugeStyle: Int, CaseIterable, Identifiable {
    case cat = 0
    case fire = 1
    //case hypersport = 2
    
    var id: Int { rawValue}
    
    var name: String {
        switch self {
            case .cat: return "Cat"
            case .fire: return "Fire"
            //case .hypersport: return "Hypersport"
        }
    }
}

struct ContentView: View {
    
    @State private var backgroundStyle: GaugeStyle = .cat
    @State private var needleStyle: GaugeStyle = .cat
    @State private var measureStyle: GaugeStyle = .cat
    
    @StateObject private var ble = BLEManager()
    
    @StateObject var locationManager = LocationManager()
    @StateObject var searchCompleter = SearchManager()
    @StateObject var routeManager = MapRouteManager()
    
    @State private var destinationText = ""
    @State private var selectedDestination: CLLocationCoordinate2D?
 
    var body: some View {
        NavigationStack {
            Form {
                Section("Connection") {
                    Text(ble.status)
                }

                Section("Gauge Settings") {
                    Picker("Background", selection: $backgroundStyle) {
                        ForEach(GaugeStyle.allCases) { style in
                            Text(style.name).tag(style)
                        }
                    }

                    Picker("Measure", selection: $measureStyle) {
                        ForEach(GaugeStyle.allCases) { style in
                            Text(style.name).tag(style)
                        }
                    }

                    Picker("Needle", selection: $needleStyle) {
                        ForEach(GaugeStyle.allCases) { style in
                            Text(style.name).tag(style)
                        }
                    }

                    Button("Update Gauge Visuals") {
                        ble.sendGaugeSettings(
                            bg: backgroundStyle.rawValue,
                            measure: measureStyle.rawValue,
                            needle: needleStyle.rawValue
                        )
                    }
                }

                Section("Navigation") {
                    TextField("Search destination", text: $destinationText)
                        .textFieldStyle(.roundedBorder)
                        .onChange(of: destinationText) { newValue in
                            searchCompleter.updateQuery(newValue)
                        }
                    ForEach(Array(searchCompleter.results.enumerated()), id: \.offset) { _, result in
                        Button {
                            destinationText = result.title + " " + result.subtitle
                            searchCompleter.results = []

                            let request = MKLocalSearch.Request(completion: result)
                            let search = MKLocalSearch(request: request)

                            search.start { response, error in
                                guard let coordinate = response?.mapItems.first?.placemark.coordinate else {
                                    print("No coordinate found")
                                    return
                                }

                                selectedDestination = coordinate
                                routeManager.status = "Destination selected"
                            }

                        } label: {
                            VStack(alignment: .leading) {
                                Text(result.title)
                                Text(result.subtitle)
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                            }
                        }
                    }
                    Text(routeManager.status)
                    Button("Send route") {
                        guard let start = locationManager.currentLocation?.coordinate else {
                            print("No current location")
                            return
                        }

                        guard let destination = selectedDestination else {
                            print("No destination selected")
                            return
                        }

                        routeManager.calculateRoute(from: start, to: destination, ble: ble)
                    }
                    .disabled(selectedDestination == nil)
                }
            }
            .navigationTitle("Gauge Pod")
        }
    }
}
#Preview {
    ContentView()
}
