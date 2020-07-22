#pragma once

#include "duktape/duktape.h"
#include <string>

class JSValue {
public:

    JSValue() = default;
    JSValue(duk_context* ctx, duk_idx_t index) : _ctx(ctx), _index(index) {}
    JSValue(JSValue&& other) noexcept : _ctx(other._ctx), _index(other._index) {
        other._ctx = nullptr;
    }
    ~JSValue() = default;

    operator bool() const { return _ctx != nullptr; }
    JSValue& operator=(const JSValue& other) = delete;
    JSValue& operator=(JSValue&& other) noexcept {
        _ctx = other._ctx;
        _index = other._index;
        other._ctx = nullptr;
        return *this;
    }

    bool    isUndefined() { return duk_is_undefined(_ctx, _index) != 0; }
    bool    isNull() { return duk_is_null(_ctx, _index) != 0; }
    bool    isBoolean() { return duk_is_boolean(_ctx, _index) != 0; }
    bool    isNumber() { return duk_is_number(_ctx, _index) != 0; }
    bool    isString() { return duk_is_string(_ctx, _index) != 0; }
    bool    isArray() { return duk_is_array(_ctx, _index) != 0; }
    bool    isObject() { return duk_is_object(_ctx, _index) != 0; }

    bool    toBool() { return duk_to_boolean(_ctx, _index) != 0; }
    int     toInt() { return duk_to_int(_ctx, _index); }
    double  toDouble() { return duk_to_number(_ctx, _index); }
    std::string toString() { return std::string(duk_to_string(_ctx, _index)); }

    size_t  getLength() { return duk_get_length(_ctx, _index); }
    duk_idx_t getStackIndex() { return _index; }

    JSValue getValueAtIndex(size_t index) {
        duk_get_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
        return JSValue(_ctx, duk_normalize_index(_ctx, -1));
    }

    JSValue getValueForProperty(const std::string& name) {
        duk_get_prop_lstring(_ctx, _index, name.data(), name.length());
        return JSValue(_ctx, duk_normalize_index(_ctx, -1));
    }

    void    setValueAtIndex(size_t index, JSValue value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_index(_ctx, _index, static_cast<duk_uarridx_t>(index));
    }

    void    setValueForProperty(const std::string& name, JSValue value) {
        value.ensureExistsOnStackTop();
        duk_put_prop_lstring(_ctx, _index, name.data(), name.length());
    }

    void    ensureExistsOnStackTop() {
        auto dukTopIndex = duk_get_top_index(_ctx);
        if (_index != dukTopIndex) {
            duk_require_stack_top(_ctx, dukTopIndex + 1);
            duk_dup(_ctx, _index);
        }
    }

private:
    duk_context* _ctx = nullptr;
    duk_idx_t _index = 0;
};