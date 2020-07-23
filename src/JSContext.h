#pragma once

#include <string>
#include "JSValue.h"

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

class JSContext {
public:

    JSContext();
    ~JSContext();

    void    setGlobalValue(const std::string& name, JSValue value);
    bool    setFunction(JSFunctionIndex index, const std::string& source);
    // bool evaluateBooleanFunction(JSFunctionIndex index);

// protected:
    JSValue newNull();

    JSValue newBoolean(bool value);
    JSValue newNumber(double value);
    JSValue newString(const std::string& value);
    JSValue newArray();
    JSValue newObject();
    JSValue newFunction(const std::string& value);
    JSValue getFunctionResult(JSFunctionIndex index);

    JSScopeMarker getScopeMarker();
    void    resetToScopeMarker(JSScopeMarker marker);

private:

    // Used for proxy object.
    // static int jsGetProperty(duk_context *_ctx);
    // static int jsHasProperty(duk_context *_ctx);

    static void fatalErrorHandler(void* userData, const char* message);

    bool    evaluateFunction(uint32_t index);

    JSValue getStackTopValue() { return JSValue(_ctx, duk_normalize_index(_ctx, -1)); }

    duk_context* _ctx = nullptr;
    // friend JavaScriptScope<JSContext>;
};
