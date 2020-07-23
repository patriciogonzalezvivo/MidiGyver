//
// Created by Matt Blair on 11/9/18.
//

#include "Context.h"
#include "JSContext.h"

const static char INSTANCE_ID[] = "\xff""\xff""obj";
const static char FUNC_ID[] = "\xff""\xff""fns";

JSContext::JSContext() {
    // Create duktape heap with default allocation functions and custom fatal error handler.
    _ctx = duk_create_heap(nullptr, nullptr, nullptr, nullptr, fatalErrorHandler);

    //// Create global geometry constants
    // TODO make immutable
    duk_push_number(_ctx, DataType::button);
    duk_put_global_string(_ctx, "button");

    duk_push_number(_ctx, DataType::toggle);
    duk_put_global_string(_ctx, "toggle");

    duk_push_number(_ctx, DataType::states);
    duk_put_global_string(_ctx, "states");

    duk_push_number(_ctx, DataType::scalar);
    duk_put_global_string(_ctx, "scalar");

    duk_push_number(_ctx, DataType::vector);
    duk_put_global_string(_ctx, "vector");

    duk_push_number(_ctx, DataType::color);
    duk_put_global_string(_ctx, "color");

    duk_push_number(_ctx, DataType::script);
    duk_put_global_string(_ctx, "script");

    // // //// Create global 'feature' object
    // // // Get Proxy constructor
    // // // -> [cons]
    // // duk_eval_string(_ctx, "Proxy");

    // // Add feature object
    // // -> [cons, { __obj: this }]
    // duk_idx_t featureObj = duk_push_object(_ctx);
    // duk_push_pointer(_ctx, this);
    // duk_put_prop_string(_ctx, featureObj, INSTANCE_ID);

    // // Add handler object
    // // -> [cons, {...}, { get: func, has: func }]
    // duk_idx_t handlerObj = duk_push_object(_ctx);
    // // Add 'get' property to handler
    // duk_push_c_function(_ctx, jsGetProperty, 3 /*nargs*/);
    // duk_put_prop_string(_ctx, handlerObj, "get");
    // // Add 'has' property to handler
    // duk_push_c_function(_ctx, jsHasProperty, 2 /*nargs*/);
    // duk_put_prop_string(_ctx, handlerObj, "has");

    // // Call proxy constructor
    // // [cons, feature, handler ] -> [obj|error]
    // if (duk_pnew(_ctx, 2) == 0) {
    //     // put feature proxy object in global scope
    //     if (!duk_put_global_string(_ctx, "feature")) {
    //         printf("Initialization failed");
    //     }
    // } else {
    //     printf("Failure: %s", duk_safe_to_string(_ctx, -1));
    //     duk_pop(_ctx);
    // }

    // Set up 'fns' array.
    duk_push_array(_ctx);
    if (!duk_put_global_string(_ctx, FUNC_ID)) {
        printf("'fns' object not set");
    }
}

JSContext::~JSContext() {
    duk_destroy_heap(_ctx);
}

void JSContext::setGlobalValue(const std::string& name, JSValue value) {
    value.ensureExistsOnStackTop();
    duk_put_global_lstring(_ctx, name.data(), name.length());
}

bool JSContext::setFunction(JSFunctionIndex index, const std::string& source) {
    // Get all functions (array) in context
    if (!duk_get_global_string(_ctx, FUNC_ID)) {
        std::cout << "AddFunction - functions array not initialized" << std::endl;
        duk_pop(_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    duk_push_string(_ctx, source.c_str());
    duk_push_string(_ctx, "");

    if (duk_pcompile(_ctx, DUK_COMPILE_FUNCTION) == 0) {
        duk_put_prop_index(_ctx, -2, index);
    } 
    else {
        printf("Compile failed: %s\n%s\n---",
             duk_safe_to_string(_ctx, -1),
             source.c_str());
        duk_pop(_ctx);
        return false;
    }

    // Pop the functions array off the stack
    duk_pop(_ctx);

    return true;
}

// bool JSContext::evaluateBooleanFunction(uint32_t index) {
//     if (!evaluateFunction(index)) {
//         return false;
//     }

//     // Evaluate the "truthiness" of the function result at the top of the stack.
//     bool result = duk_to_boolean(_ctx, -1) != 0;

//     // pop result
//     duk_pop(_ctx);

//     return result;
// }

JSValue JSContext::getFunctionResult(uint32_t index) {
    if (!evaluateFunction(index)) {
        return JSValue();
    }
    return getStackTopValue();
}

JSValue JSContext::newNull() {
    duk_push_null(_ctx);
    return getStackTopValue();
}

JSValue JSContext::newBoolean(bool value) {
    duk_push_boolean(_ctx, static_cast<duk_bool_t>(value));
    return getStackTopValue();
}

JSValue JSContext::newNumber(double value) {
    duk_push_number(_ctx, value);
    return getStackTopValue();
}

JSValue JSContext::newString(const std::string& value) {
    duk_push_lstring(_ctx, value.data(), value.length());
    return getStackTopValue();
}

JSValue JSContext::newArray() {
    duk_push_array(_ctx);
    return getStackTopValue();
}

JSValue JSContext::newObject() {
    duk_push_object(_ctx);
    return getStackTopValue();
}

JSValue JSContext::newFunction(const std::string& value) {
    if (duk_pcompile_lstring(_ctx, DUK_COMPILE_FUNCTION, value.data(), value.length()) != 0) {
        auto error = duk_safe_to_string(_ctx, -1);
        printf("Compile failed in global function: %s\n%s\n---", error, value.c_str());
        duk_pop(_ctx); // Pop error.
        return JSValue();
    }
    return getStackTopValue();
}

JSScopeMarker JSContext::getScopeMarker() {
    return duk_get_top(_ctx);
}

void JSContext::resetToScopeMarker(JSScopeMarker marker) {
    duk_set_top(_ctx, marker);
}

// // Implements Proxy handler.has(target_object, key)
// int JSContext::jsHasProperty(duk_context *_ctx) {

//     duk_get_prop_string(_ctx, 0, INSTANCE_ID);
//     auto context = static_cast<const JSContext*>(duk_to_pointer(_ctx, -1));
//     if (!context || !context->_feature) {
//         printf("Error: no context set %p %p", context, context ? context->_feature : nullptr);
//         duk_pop(_ctx);
//         return 0;
//     }

//     const char* key = duk_require_string(_ctx, 1);
//     auto result = static_cast<duk_bool_t>(context->_feature->props.contains(key));
//     duk_push_boolean(_ctx, result);

//     return 1;
// }

// // Implements Proxy handler.get(target_object, key)
// int JSContext::jsGetProperty(duk_context *_ctx) {

//     // Get the JavaScriptContext instance from JS Feature object (first parameter).
//     duk_get_prop_string(_ctx, 0, INSTANCE_ID);
//     auto context = static_cast<const JSContext*>(duk_to_pointer(_ctx, -1));
//     if (!context || !context->_feature) {
//         printf("Error: no context set %p %p",  context, context ? context->_feature : nullptr);
//         duk_pop(_ctx);
//         return 0;
//     }

//     // Get the property name (second parameter)
//     const char* key = duk_require_string(_ctx, 1);

//     auto it = context->_feature->props.get(key);
//     if (it.is<std::string>()) {
//         duk_push_string(_ctx, it.get<std::string>().c_str());
//     } else if (it.is<double>()) {
//         duk_push_number(_ctx, it.get<double>());
//     } else {
//         duk_push_undefined(_ctx);
//     }
//     // FIXME: Distinguish Booleans here as well

//     return 1;
// }

void JSContext::fatalErrorHandler(void*, const char* message) {
    printf("Fatal Error in DuktapeJavaScriptContext: %s", message);
    abort();
}

bool JSContext::evaluateFunction(uint32_t index) {
    // Get all functions (array) in context
    if (!duk_get_global_string(_ctx, FUNC_ID)) {
        printf("EvalFilterFn - functions array not initialized");
        duk_pop(_ctx); // pop [undefined] sitting at stack top
        return false;
    }

    // Get function at index `id` from functions array, put it at stack top
    if (!duk_get_prop_index(_ctx, -1, index)) {
        printf("EvalFilterFn - function %d not set", index);
        duk_pop(_ctx); // pop "undefined" sitting at stack top
        duk_pop(_ctx); // pop functions (array) now sitting at stack top
        return false;
    }

    // pop fns array
    duk_remove(_ctx, -2);

    // call popped function (sitting at stack top), evaluated value is put on stack top
    if (duk_pcall(_ctx, 0) != 0) {
        printf("EvalFilterFn: %s", duk_safe_to_string(_ctx, -1));
        duk_pop(_ctx);
        return false;
    }

    return true;
}
