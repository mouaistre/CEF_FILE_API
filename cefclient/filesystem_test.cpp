// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefclient/filesystem_test.h"

#include <algorithm>
#include <string>

#include "include/wrapper/cef_stream_resource_handler.h"
#include "cefclient/resource_util.h"

namespace filesystem_test {

namespace {

const char* kTestUrl = "http://tests/filesystem";
const char* kMessageName = "binding_test";

// Handle messages in the browser process.
class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
 public:
  ProcessMessageDelegate() {
  }

  // From ClientHandler::ProcessMessageDelegate.
  virtual bool OnProcessMessageReceived(
      CefRefPtr<ClientHandler> handler,
      CefRefPtr<CefBrowser> browser,
      CefProcessId source_process,
      CefRefPtr<CefProcessMessage> message) OVERRIDE {

    std::string message_name = message->GetName();
    CefRefPtr<CefListValue> argList = message->GetArgumentList();
    int32 callbackId = -1;
    int32 error = NO_ERROR;
    CefRefPtr<CefProcessMessage> response = CefProcessMessage::Create("invokeCallback");
    CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
        
        // V8 extension messages are handled here. These messages come from the 
        // render process thread (in client_app.cpp), and have the following format:
        //   name - the name of the native function to call
        //   argument0 - the id of this message. This id is passed back to the
        //               render process in order to execute callbacks
        //   argument1 - argumentN - the arguments for the function
        //
        // Note: Functions without callback can be specified, but they *cannot*
        // take any arguments.
        
        // If we have any arguments, the first is the callbackId
        if (argList->GetSize() > 0) {
            callbackId = argList->GetInt(0);
            
            if (callbackId != -1)
                responseArgs->SetInt(0, callbackId);
        }

		if (message_name == "browseForOpen") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - argURL
            //  2: bool - enableRemoteDebugging
			/*
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString argURL = argList->GetString(1);
                bool enableRemoteDebugging = argList->GetBool(2);
                error = OpenLiveBrowser(argURL, enableRemoteDebugging);
            }*/
			responseArgs->SetInt(2, callbackId);
            
        }
		if (callbackId != -1) {
            responseArgs->SetInt(1, error);
          
            // Send response
            browser->SendProcessMessage(PID_RENDERER, response);
        }
      
        return true;



  }

  IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};

// Handle resource loading in the browser process.
class RequestDelegate: public ClientHandler::RequestDelegate {
 public:
  RequestDelegate() {
  }

  // From ClientHandler::RequestDelegate.
  virtual CefRefPtr<CefResourceHandler> GetResourceHandler(
      CefRefPtr<ClientHandler> handler,
      CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      CefRefPtr<CefRequest> request) OVERRIDE {
    std::string url = request->GetURL();
    if (url == kTestUrl) {
      // Show the file system contents
      CefRefPtr<CefStreamReader> stream =
          GetBinaryResourceReader("filesystem.html");
      ASSERT(stream.get());
      return new CefStreamResourceHandler("text/html", stream);
    }

    return NULL;
  }

  IMPLEMENT_REFCOUNTING(RequestDelegate);
};

}  // namespace

void CreateProcessMessageDelegates(
    ClientHandler::ProcessMessageDelegateSet& delegates) {
  delegates.insert(new ProcessMessageDelegate);
}

void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates) {
  delegates.insert(new RequestDelegate);
}

void RunTest(CefRefPtr<CefBrowser> browser) {
  // Load the test URL.
  browser->GetMainFrame()->LoadURL(kTestUrl);
}

}  // namespace binding_test
