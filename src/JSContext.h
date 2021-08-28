#pragma once

#include <string>
#include "JSValue.h"

using JSScopeMarker = int32_t;
using JSFunctionIndex = uint32_t;

class JSContext {
public:

    JSContext();
    ~JSContext();

    JSValue newNull();
    JSValue newBoolean(bool value);
    JSValue newNumber(float value);
    JSValue newString(const std::string& value);
    JSValue newArray();
    JSValue newObject();
    JSValue newFunction(const std::string& value);

    bool    setFunction(JSFunctionIndex index, const std::string& source);
    JSValue getFunctionResult(JSFunctionIndex index);

    bool    addNativeFunction(const std::string& _name, duk_c_function func, size_t nargs);

    void    setGlobalValue(const std::string& name, JSValue value);

    JSScopeMarker getScopeMarker();
    void    resetToScopeMarker(JSScopeMarker marker);

    uint32_t idCounter;

private:
    static void fatalErrorHandler(void* userData, const char* message);

    bool    evaluateFunction(uint32_t index);

    JSValue getStackTopValue() { return JSValue(_ctx, duk_normalize_index(_ctx, -1)); }

    duk_context* _ctx = nullptr;
};
