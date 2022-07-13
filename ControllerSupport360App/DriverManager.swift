//
//  DriverManager.swift
//  ControllerSupport360App
//
//  Created by Skyler Calaman on 6/26/22.
//

import Foundation
import SystemExtensions
import os.log

class DriverManager : NSObject, OSSystemExtensionRequestDelegate {
    
    static let shared = DriverManager()
    
    func activate() {
        deactivate()
        os_log("attempting to activate...")
        let activationRequest = OSSystemExtensionRequest.activationRequest(forExtensionWithIdentifier: "dev.skyc.ControllerSupport360", queue: .main)
        activationRequest.delegate = self
        OSSystemExtensionManager.shared.submitRequest(activationRequest)
    }
    
    func deactivate() {
        os_log("attempting to deactivate...")
        // This doesn't seem to work in b1 not sure why
        let activationRequest = OSSystemExtensionRequest.deactivationRequest(forExtensionWithIdentifier: "dev.skyc.ControllerSupport360", queue: .main)
        activationRequest.delegate = self
        OSSystemExtensionManager.shared.submitRequest(activationRequest)
    }
    
    func request(_ request: OSSystemExtensionRequest, actionForReplacingExtension existing: OSSystemExtensionProperties, withExtension ext: OSSystemExtensionProperties) -> OSSystemExtensionRequest.ReplacementAction {
        os_log("sysex actionForReplacingExtension %@ %@", existing, ext)
        
        return .replace
    }
    
    func requestNeedsUserApproval(_ request: OSSystemExtensionRequest) {
        os_log("sysex needsUserApproval")
        
    }
    
    func request(_ request: OSSystemExtensionRequest, didFinishWithResult result: OSSystemExtensionRequest.Result) {
        os_log("sysex didFinishWithResult %@", result.rawValue)
        
    }
    
    func request(_ request: OSSystemExtensionRequest, didFailWithError error: Error) {
//        request.debugDescription
        os_log("sysex didFailWithError %@ (%@)", error.localizedDescription, request.identifier)
    }
}
