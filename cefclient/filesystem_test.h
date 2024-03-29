// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
#define CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
#pragma once

#include "cefclient/client_app.h"
#include "cefclient/client_handler.h"
#include "appshell_extensions_platform.h"

namespace filesystem_test {

// Delegate creation. Called from ClientApp and ClientHandler.
void CreateProcessMessageDelegates(
    ClientHandler::ProcessMessageDelegateSet& delegates);
void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates);

// Run the test.
void RunTest(CefRefPtr<CefBrowser> browser);

}  // namespace filesystem_test

#endif  // CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
