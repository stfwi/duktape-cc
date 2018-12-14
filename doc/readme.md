
# JavaScript function documentation

The file stdmod.js contains a JavaDoc like representation
of the available functionality including the optional module
functions in the `mod` directory.

Object declarations with empty function bodies enable IDEs
to support auto completion.

## Unexported Duktape API functions in `duktape::api`

Some functions of the Duktape API only apply to the C implementation
and should not be used in c++. The list of these functions is moved
from the `duktape.hh` header to this location:

```
    //
    // Not appropriate or replaced with std::string (general lstring usage)
    // ------------------------------------------------------------------------------------------
    // duk_context *duk_create_heap_default(void);
    // void duk_destroy_heap(duk_context *ctx);
    // int duk_pcompile_string(duk_context *ctx, unsigned flags, const char *src);
    // int duk_pcompile_string_filename(duk_context *ctx, unsigned flags, const char *src);
    // int duk_peval_file(duk_context *ctx, const char *path);
    // int duk_peval_file_noresult(duk_context *ctx, const char *path);
    // int duk_peval_noresult(duk_context *ctx);
    // int duk_peval_string(duk_context *ctx, const char *src);
    // int duk_peval_string_noresult(duk_context *ctx, const char *src);
    // const char *duk_get_string(duk_context *ctx, duk_idx_t idx);
    // const char *duk_push_string(duk_context *ctx, const char *str);
    // const char *duk_push_sprintf(duk_context *ctx, const char *fmt, ...);
    // duk_bool_t duk_put_prop_string(duk_context *ctx, index_t obj_index, const char *key);
    // const char *duk_safe_to_lstring(duk_context *ctx, index_t index, size_t *out_len)
    // const char *duk_safe_to_string(duk_context *ctx, index_t index);
    // const char *duk_to_lstring(duk_context *ctx, index_t index, size_t *out_len)
    // const char *duk_to_string(duk_context *ctx, index_t index);
    // const char* get_string(duk_context *ctx, index_t index)
    // const char* require_string(duk_context *ctx, index_t index)
    // void duk_pop_2(duk_context *ctx);
    // void duk_pop_3(duk_context *ctx);
    // const char *duk_push_vsprintf(duk_context *ctx, const char *fmt, va_list ap);
    // duk_int_t duk_get_current_magic(duk_context *ctx);
    // duk_int_t duk_get_magic(duk_context *ctx, duk_idx_t idx);
    // void duk_set_magic(duk_context *ctx, duk_idx_t idx, duk_int_t magic);
    // void *duk_get_heapptr(duk_context *ctx, duk_idx_t idx);
    // void *duk_require_heapptr(duk_context *ctx, duk_idx_t idx);
    // duk_idx_t duk_push_heapptr(duk_context *ctx, void *ptr);
    // void duk_new(duk_context *ctx, duk_idx_t nargs); // use pnew
    // const char *duk_require_string(duk_context *ctx, duk_idx_t idx);
    // duk_error(duk_context *ctx, duk_errcode_t err_code, const char *fmt, ...);
    // duk_ret_t duk_range_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_range_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_error_va(duk_context *ctx, duk_errcode_t err_code, const char *fmt, va_list ap);
    // duk_ret_t duk_eval_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_generic_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_generic_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_reference_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_reference_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_syntax_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_syntax_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_type_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_type_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_ret_t duk_uri_error(duk_context *ctx, const char *fmt, ...);
    // duk_ret_t duk_uri_error_va(duk_context *ctx, const char *fmt, va_list ap);
    // duk_eval_noresult(ctx);
    // void duk_eval_string(duk_context *ctx, const char *src);
    // void duk_eval_string_noresult(duk_context *ctx, const char *src);
    // duk_bool_t duk_put_global_string(duk_context *ctx, const char *key);
    // index_t duk_push_error_object(duk_context *ctx, duk_errcode_t err_code, const char *fmt, ...);
    // const char *duk_push_sprintf(duk_context *ctx, const char *fmt, ...);
    // const char *duk_push_vsprintf(duk_context *ctx, const char *fmt, va_list ap);
    // duk_idx_t duk_push_error_object_va(duk_context *ctx, duk_errcode_t err_code, const char *fmt, va_list ap);
    // duk_ret_t duk_fatal(duk_context *ctx, const char *err_msg);
    // void dump_context_stderr() const  { duk_dump_context_stderr(ctx_); }
    // void dump_context_stdout() const  { duk_dump_context_stdout(ctx_); }
    // ------------------------------------------------------------------------------------------
    // Do be checked to include with c++ type (chrono, extra class etc) or if already covered
    // (e.g. with Date conversion trait).
    // ------------------------------------------------------------------------------------------
    // duk_idx_t duk_push_error_object(duk_context *ctx, duk_errcode_t err_code, const char *fmt, ...); // covered with throw_error()
    // duk_double_t duk_components_to_time(duk_context *ctx, duk_time_components *comp); // Covered with Date conversion
    // void duk_time_to_components(duk_context *ctx, duk_double_t time, duk_time_components *comp);
    // void duk_debugger_attach(duk_context *ctx,.......);
    // void duk_debugger_cooperate(duk_context *ctx);
    // void duk_debugger_detach(duk_context *ctx);
    // duk_bool_t duk_debugger_notify(duk_context *ctx, duk_idx_t nvalues);
    // void duk_debugger_pause(duk_context *ctx);
    // duk_idx_t duk_push_c_lightfunc(duk_context *ctx, duk_c_function func, duk_idx_t nargs, duk_idx_t length, duk_int_t magic);
```