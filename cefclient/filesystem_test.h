// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
#define CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
#pragma once

#include "cefclient/client_app.h"
#include "cefclient/client_handler.h"

// Extension error codes. These MUST be in sync with the error
// codes in appshell_extensions.js
#if !defined(OS_WIN) // NO_ERROR is defined on windows
static const int NO_ERROR                   = 0;
#endif
static const int ERR_UNKNOWN                = 1;
static const int ERR_INVALID_PARAMS         = 2;
static const int ERR_NOT_FOUND              = 3;
static const int ERR_CANT_READ              = 4;
static const int ERR_UNSUPPORTED_ENCODING   = 5;
static const int ERR_CANT_WRITE             = 6;
static const int ERR_OUT_OF_SPACE           = 7;
static const int ERR_NOT_FILE               = 8;
static const int ERR_NOT_DIRECTORY          = 9;
static const int ERR_FILE_EXISTS            = 10;

namespace filesystem_test {

// Delegate creation. Called from ClientApp and ClientHandler.
void CreateProcessMessageDelegates(
    ClientHandler::ProcessMessageDelegateSet& delegates);
void CreateRequestDelegates(ClientHandler::RequestDelegateSet& delegates);

// Run the test.
void RunTest(CefRefPtr<CefBrowser> browser);

}  // namespace filesystem_test

#endif  // CEF_TESTS_CEFCLIENT_FILESYSTEM_TEST_H_
