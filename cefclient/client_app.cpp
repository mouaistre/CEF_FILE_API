// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

// This file is shared by cefclient and cef_unittests so don't include using
// a qualified path.
#include "client_app.h"  // NOLINT(build/include)

#include <string>

#include "include/cef_cookie.h"
#include "include/cef_process_message.h"
#include "include/cef_task.h"
#include "include/cef_v8.h"
#include "util.h"  // NOLINT(build/include)

namespace {

// Forward declarations.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target);
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target);

// Transfer a V8 value to a List index.
void SetListValue(CefRefPtr<CefListValue> list, int index,
                  CefRefPtr<CefV8Value> value) {
  if (value->IsArray()) {
    CefRefPtr<CefListValue> new_list = CefListValue::Create();
    SetList(value, new_list);
    list->SetList(index, new_list);
  } else if (value->IsString()) {
    list->SetString(index, value->GetStringValue());
  } else if (value->IsBool()) {
    list->SetBool(index, value->GetBoolValue());
  } else if (value->IsInt()) {
    list->SetInt(index, value->GetIntValue());
  } else if (value->IsDouble()) {
    list->SetDouble(index, value->GetDoubleValue());
  }
}

// Transfer a V8 array to a List.
void SetList(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target) {
  ASSERT(source->IsArray());

  int arg_length = source->GetArrayLength();
  if (arg_length == 0)
    return;

  // Start with null types in all spaces.
  target->SetSize(arg_length);

  for (int i = 0; i < arg_length; ++i)
    SetListValue(target, i, source->GetValue(i));
}

// Transfer a List value to a V8 array index.
void SetListValue(CefRefPtr<CefV8Value> list, int index,
                  CefRefPtr<CefListValue> value) {
  CefRefPtr<CefV8Value> new_value;

  CefValueType type = value->GetType(index);
  switch (type) {
    case VTYPE_LIST: {
      CefRefPtr<CefListValue> list = value->GetList(index);
      new_value = CefV8Value::CreateArray(list->GetSize());
      SetList(list, new_value);
      } break;
    case VTYPE_BOOL:
      new_value = CefV8Value::CreateBool(value->GetBool(index));
      break;
    case VTYPE_DOUBLE:
      new_value = CefV8Value::CreateDouble(value->GetDouble(index));
      break;
    case VTYPE_INT:
      new_value = CefV8Value::CreateInt(value->GetInt(index));
      break;
    case VTYPE_STRING:
      new_value = CefV8Value::CreateString(value->GetString(index));
      break;
    default:
      break;
  }

  if (new_value.get()) {
    list->SetValue(index, new_value);
  } else {
    list->SetValue(index, CefV8Value::CreateNull());
  }
}

// Transfer a List to a V8 array.
void SetList(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target) {
  ASSERT(target->IsArray());

  int arg_length = source->GetSize();
  if (arg_length == 0)
    return;

  for (int i = 0; i < arg_length; ++i)
    SetListValue(target, i, source);
}


class FileSystemExtensionHandler : public CefV8Handler {
 public:
  explicit FileSystemExtensionHandler(CefRefPtr<ClientApp> client_app)
    : client_app_(client_app),messageId(0) {
  }

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) {
    bool handled = false;

	 // Pass all messages to the browser process. Look in appshell_extensions.cpp for implementation.
     CefRefPtr<CefBrowser> browser = CefV8Context::GetCurrentContext()->GetBrowser();
     ASSERT(browser.get());
     CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(name);
     CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
          
     // The first argument must be a callback function
     if (arguments.size() > 0 && !arguments[0]->IsFunction()) {
          std::string functionName = name;
          fprintf(stderr, "Function called without callback param: %s\n", functionName.c_str());
          return false;
     } 
          
     if (arguments.size() > 0) {
         // The first argument is the message id
         client_app_->AddCallback(messageId, CefV8Context::GetCurrentContext(), arguments[0]);
		 SetListValue(messageArgs, 0, CefV8Value::CreateInt(messageId));
      }
          
      // Pass the rest of the arguments
      for (unsigned int i = 1; i < arguments.size(); i++)
              SetListValue(messageArgs, i, arguments[i]);
      browser->SendProcessMessage(PID_BROWSER, message);
          
      messageId++;
	
    return true;
  }

 private:
  CefRefPtr<ClientApp> client_app_;
  int32 messageId;
  IMPLEMENT_REFCOUNTING(FileSystemExtensionHandler);
};
/*
// Handles the native implementation for the File System extension.
class FileSystemExtensionHandler : public CefV8Handler {
 public:
  explicit FileSystemExtensionHandler(CefRefPtr<ClientApp> client_app)
    : client_app_(client_app)
    , messageId(0) {

  }

  virtual bool Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception) {
      
      // The only messages that are handled here is getElapsedMilliseconds(),
      // GetCurrentLanguage(), GetApplicationSupportDirectory(), and GetRemoteDebuggingPort().
      // All other messages are passed to the browser process.
      //if (name == "GetElapsedMilliseconds") {
        //  retval = CefV8Value::CreateDouble(client_app_->GetElapsedMilliseconds());
      //} else if (name == "GetCurrentLanguage") {
      //    retval = CefV8Value::CreateString(client_app_->GetCurrentLanguage());
      //} else if (name == "GetApplicationSupportDirectory") {
      //    retval = CefV8Value::CreateString(ClientApp::AppGetSupportDirectory());
      //} else if (name == "GetRemoteDebuggingPort") {
      //    retval = CefV8Value::CreateInt(REMOTE_DEBUGGING_PORT);
      //} else

		fprintf(stdout, "Hello");
		
          // Pass all messages to the browser process. Look in appshell_extensions.cpp for implementation.
          CefRefPtr<CefBrowser> browser = 
                CefV8Context::GetCurrentContext()->GetBrowser();
          ASSERT(browser.get());
          CefRefPtr<CefProcessMessage> message = 
                CefProcessMessage::Create(name);
          CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
          
          // The first argument must be a callback function
          if (arguments.size() > 0 && !arguments[0]->IsFunction()) {
              std::string functionName = name;
              fprintf(stderr, "Function called without callback param: %s\n", functionName.c_str());
              return false;
          } 
          
          if (arguments.size() > 0) {
              // The first argument is the message id
			  int browser_id = browser->GetIdentifier();

              client_app_->SetMessageCallback(name,browser_id, CefV8Context::GetCurrentContext(), arguments[0]);
              SetListValue(messageArgs, 0, CefV8Value::CreateInt(messageId));
          }
          
          // Pass the rest of the arguments
          for (unsigned int i = 1; i < arguments.size(); i++)
              SetListValue(messageArgs, i, arguments[i]);
          browser->SendProcessMessage(PID_BROWSER, message);
          
          messageId++;
	
	return true;
  }
  */
}  // namespace

ClientApp::ClientApp() {
  CreateBrowserDelegates(browser_delegates_);
  CreateRenderDelegates(render_delegates_);

  // Default schemes that support cookies.
  cookieable_schemes_.push_back("http");
  cookieable_schemes_.push_back("https");
}

void ClientApp::OnContextInitialized() {
  // Register cookieable schemes with the global cookie manager.
  CefRefPtr<CefCookieManager> manager = CefCookieManager::GetGlobalManager();
  ASSERT(manager.get());
  manager->SetSupportedSchemes(cookieable_schemes_);

  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnContextInitialized(this);
}

void ClientApp::OnBeforeChildProcessLaunch(
      CefRefPtr<CefCommandLine> command_line) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnBeforeChildProcessLaunch(this, command_line);
}

void ClientApp::OnRenderProcessThreadCreated(
    CefRefPtr<CefListValue> extra_info) {
  BrowserDelegateSet::iterator it = browser_delegates_.begin();
  for (; it != browser_delegates_.end(); ++it)
    (*it)->OnRenderProcessThreadCreated(this, extra_info);
}

void ClientApp::OnRenderThreadCreated(CefRefPtr<CefListValue> extra_info) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnRenderThreadCreated(this, extra_info);
}

void ClientApp::OnWebKitInitialized() {
  // Register the client_app extension.


  std::string fs_code =
    "var fs;"
    "if (!fs)"
    "  fs = {};"
    "(function() {"
    "  fs.browseForOpen = function(callback,title) {"
    "    native function browseForOpen();"
    "    return browseForOpen(callback,title);"
    "  };"
    "})();";
  CefRegisterExtension("v8/fs", fs_code,
	  new FileSystemExtensionHandler(this));


  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWebKitInitialized(this);
}

void ClientApp::OnBrowserCreated(CefRefPtr<CefBrowser> browser) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserCreated(this, browser);
}

void ClientApp::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnBrowserDestroyed(this, browser);
}

bool ClientApp::OnBeforeNavigation(CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame,
                                   CefRefPtr<CefRequest> request,
                                   NavigationType navigation_type,
                                   bool is_redirect) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    if ((*it)->OnBeforeNavigation(this, browser, frame, request,
                                  navigation_type, is_redirect)) {
      return true;
    }
  }

  return false;
}

void ClientApp::OnContextCreated(CefRefPtr<CefBrowser> browser,
                                 CefRefPtr<CefFrame> frame,
                                 CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextCreated(this, browser, frame, context);
}

void ClientApp::OnContextReleased(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnContextReleased(this, browser, frame, context);

  // Remove any JavaScript callbacks registered for the context that has been
  // released.
  if (!callback_map_.empty()) {
    CallbackMap::iterator it = callback_map_.begin();
    for (; it != callback_map_.end();) {
      if (it->second.first->IsSame(context))
        callback_map_.erase(it++);
      else
        ++it;
    }
  }
}

void ClientApp::OnUncaughtException(CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame,
                                    CefRefPtr<CefV8Context> context,
                                    CefRefPtr<CefV8Exception> exception,
                                    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->OnUncaughtException(this, browser, frame, context, exception,
                               stackTrace);
  }
}

void ClientApp::OnWorkerContextCreated(int worker_id,
                                       const CefString& url,
                                       CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWorkerContextCreated(this, worker_id, url, context);
}

void ClientApp::OnWorkerContextReleased(int worker_id,
                                        const CefString& url,
                                        CefRefPtr<CefV8Context> context) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnWorkerContextReleased(this, worker_id, url, context);
}

void ClientApp::OnWorkerUncaughtException(
    int worker_id,
    const CefString& url,
    CefRefPtr<CefV8Context> context,
    CefRefPtr<CefV8Exception> exception,
    CefRefPtr<CefV8StackTrace> stackTrace) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it) {
    (*it)->OnWorkerUncaughtException(this, worker_id, url, context, exception,
                                     stackTrace);
  }
}

void ClientApp::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser,
                                     CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefDOMNode> node) {
  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end(); ++it)
    (*it)->OnFocusedNodeChanged(this, browser, frame, node);
}


CefRefPtr<CefV8Value> ListValueToV8Value(CefRefPtr<CefListValue> value, int index)
{
    CefRefPtr<CefV8Value> new_value;
    
    CefValueType type = value->GetType(index);
    switch (type) {
        case VTYPE_LIST: {
            CefRefPtr<CefListValue> list = value->GetList(index);
            new_value = CefV8Value::CreateArray(list->GetSize());
            SetList(list, new_value);
        } break;
        case VTYPE_BOOL:
            new_value = CefV8Value::CreateBool(value->GetBool(index));
            break;
        case VTYPE_DOUBLE:
            new_value = CefV8Value::CreateDouble(value->GetDouble(index));
            break;
        case VTYPE_INT:
            new_value = CefV8Value::CreateInt(value->GetInt(index));
            break;
        case VTYPE_STRING:
            new_value = CefV8Value::CreateString(value->GetString(index));
            break;
        default:
            new_value = CefV8Value::CreateNull();
            break;
    }
    
    return new_value;
}


//Simple stack class to ensure calls to Enter and Exit are balanced
class StContextScope {
public:
    StContextScope( const CefRefPtr<CefV8Context>& ctx )
    : m_ctx(NULL) {
        if( ctx && ctx->Enter() ) {
            m_ctx = ctx;
        }
    }
    
    ~StContextScope() {
        if(m_ctx) {
            m_ctx->Exit();
        }
    }
    
    const CefRefPtr<CefV8Context>& GetContext() const { 
        return m_ctx;
    }
    
private:
    CefRefPtr<CefV8Context> m_ctx;
    
};


bool ClientApp::OnProcessMessageReceived(
    CefRefPtr<CefBrowser> browser,
    CefProcessId source_process,
    CefRefPtr<CefProcessMessage> message) {
  ASSERT(source_process == PID_BROWSER);

  bool handled = false;

  RenderDelegateSet::iterator it = render_delegates_.begin();
  for (; it != render_delegates_.end() && !handled; ++it) {
    handled = (*it)->OnProcessMessageReceived(this, browser, source_process,
                                              message);
  }

  if (!handled) {
        if (message->GetName() == "invokeCallback") {
            // This is called by the appshell extension handler to invoke the asynchronous 
            // callback function
            
            CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
            int32 callbackId = messageArgs->GetInt(0);
                    
            CefRefPtr<CefV8Context> context = callback_map_[callbackId].first;
            CefRefPtr<CefV8Value> callbackFunction = callback_map_[callbackId].second;
            CefV8ValueList arguments;
            context->Enter();
               
            for (size_t i = 1; i < messageArgs->GetSize(); i++) {
                arguments.push_back(ListValueToV8Value(messageArgs, i));
            }
            
            callbackFunction->ExecuteFunction(NULL, arguments);
            context->Exit();
            
            callback_map_.erase(callbackId);
			handled=true;
        } 
		/*else if (message->GetName() == "executeCommand") {
            // This is called by the browser process to execute a command via JavaScript
            // 
            // The first argument is the command name. This is required.
            // The second argument is a message id. This is optional. If set, a response
            // message will be sent back to the browser process.
            
            CefRefPtr<CefListValue> messageArgs = message->GetArgumentList();
            CefString commandName = messageArgs->GetString(0);
            int messageId = messageArgs->GetSize() > 1 ? messageArgs->GetInt(1) : -1;
            bool handled = false;
            
            StContextScope ctx(browser->GetMainFrame()->GetV8Context());
            
            CefRefPtr<CefV8Value> global = ctx.GetContext()->GetGlobal();
            
            if (global->HasValue("brackets")) { 
                
                CefRefPtr<CefV8Value> brackets = global->GetValue("brackets");
                
                if (brackets->HasValue("shellAPI")) {
                    
                    CefRefPtr<CefV8Value> shellAPI = brackets->GetValue("shellAPI");
                    
                    if (shellAPI->HasValue("executeCommand")) {
                        
                        CefRefPtr<CefV8Value> executeCommand = shellAPI->GetValue("executeCommand");
                        
                        if (executeCommand->IsFunction()) {
                            
                            CefRefPtr<CefV8Value> retval;
                            CefV8ValueList args;
                            args.push_back(CefV8Value::CreateString(commandName));
                            
                            retval = executeCommand->ExecuteFunction(global, args);
                            
                            if (retval) {
                                handled = retval->GetBoolValue();
                            }
                        }
                    }
                }
            }
            
            // Return a message saying whether or not the command was handled
            if (messageId != -1) {
                CefRefPtr<CefProcessMessage> result = CefProcessMessage::Create("executeCommandCallback");
                result->GetArgumentList()->SetInt(0, messageId);
                result->GetArgumentList()->SetBool(1, handled);
                
                browser->SendProcessMessage(PID_BROWSER, result);
            }
        }*/
    }
    
	return handled;

}
