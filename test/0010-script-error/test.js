

function e() {
  print("-> e()");
  wrapped_native_function_throwing_script_error();
  print("<- e()");
}

function d() {
  print("-> d()");
  e();
  print("<- d()");
}

function c() {
  print("-> c()");
  wrapped_apistack_call("d");
  print("<- c()");
}

function b() {
  print("-> b()");
  wrapped_engine_call("c");
  print("<- a()");
}

function a() {
  print("-> a()");
  wrapped_engine_eval("b();");
  print("<- a()");
}

try {
  a();
} catch(ex) {
  print("(exception caught and rethrown in js)");
  throw ex;
}
