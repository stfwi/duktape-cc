

global.e = function() {
  print("-> e()");
  wrapped_native_function_throwing_script_error();
  print("<- e()");
};

global.d = function() {
  print("-> d()");
  e();
  print("<- d()");
};

global.c = function() {
  print("-> c()");
  wrapped_apistack_call("d");
  print("<- c()");
};

global.b = function() {
  print("-> b()");
  wrapped_engine_call("c");
  print("<- a()");
};

global.a = function() {
  print("-> a()");
  wrapped_engine_eval("b();");
  print("<- a()");
};

try {
  a();
} catch(ex) {
  print("(exception caught and rethrown in js)");
  throw ex;
}
