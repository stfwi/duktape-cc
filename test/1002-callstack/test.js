
function a() {
  b();
}

function b() {
  c();
}

function c() {
  print("trace() = " + JSON.stringify(trace()) );
}

print("sys.executable() = " + sys.executable() );

a();

print( JSON.stringify(Duktape.env) );
