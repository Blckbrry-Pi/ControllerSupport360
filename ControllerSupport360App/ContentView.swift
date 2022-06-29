//
//  ContentView.swift
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 6/25/22.
//

import SwiftUI

struct ContentView: View {
    var body: some View {
        Button("click meeeeee", action: DriverManager.shared.activate)
    }
}

struct ContentView_Previews: PreviewProvider {
    static var previews: some View {
        ContentView()
    }
}
