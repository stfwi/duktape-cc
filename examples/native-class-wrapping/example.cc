#include <duktape/duktape.hh>
#include <duktape/mod/mod.stdio.hh>
#include <duktape/mod/mod.stdlib.hh>

using namespace std;

/**
 * A native class that is used as example to be
 * instantiated and accessible from the JS engine.
 *
 * Actually, circle is quite understandable as
 * class, but this wrapping feature aims more for
 * the bigger things, where the wrapping overhead
 * is small compared to the functionality of the
 * class - like e.g. Image, Drivers, Serial, CAN,
 * OpenCV, matrix solvers, or simply classes that
 * are part of your application and have to be
 * configured before using them.
 */
struct circle
{
  double x, y, r;

  circle() noexcept : x(0), y(0), r(1) {}
  circle(double xc, double yc, double rc) noexcept : x(xc), y(yc), r(rc) {}
  circle(const circle&) = default;
  circle(circle&&) = default;
  ~circle() = default;

  void move(double newx, double newy) noexcept
  { x=newx; y=newy; }

  static constexpr double pi = double(4) * atan(double(1));

  double circumference() const noexcept
  { return pi * r * 2.; }

  double area() const noexcept
  { return pi * r * r; }

};


/**
 * Main: Prepare the JS engine, wrap the circle class,
 * and run the example script.
 */
int main(int, const char**)
{
  // Define the JS engine, as well as some default modules for
  // console I/O.
  duktape::engine js;
  duktape::mod::stdio::define_in(js);
  duktape::mod::stdlib::define_in(js);

  // Define the native circle wrapper ...
  js.define(
    // First construct a new `native_object` with your class type
    // and the name in the JS engine. This can also be a canonical
    // name like "Math.geometry.Circle2d". Parent objects will
    // automatically be created if missing.
    duktape::native_object<circle>("Circle")
    // First action to do should be the constructor. This function
    // shall check the arguments given from the engine (we use the stack)
    // format, because this is more flexible, and also allows to pass
    // objects, arrays or whatever.
    // You must return the `new` instance of your native object. Or you
    // can throw a `script_error` if the arguments are wrong. Here I don't
    // do all the numeric checks, just (1)no arguments->default construct,
    // (2) three arguemnts-> construct with x,y,r (assuming it's really doubles),
    // (3) throw because nothing else is accepted in this example.
    .constructor([](duktape::api& stack) {
      if(stack.top()==0) {
        return new circle();
      } else if(stack.top()==3) {
        return new circle(stack.get<double>(0), stack.get<double>(1), stack.get<double>(2));
      } else {
        throw duktape::script_error("Circle constructor needs either none or three arguments (x,z,r)");
      }
    })
    // Then let's project some aspects of the native object to properties
    // in the engine. `circle.x/y/z` sounds good. They are gettable and settable,
    // so we add getters and setters. Both function types are passed the stack
    // to interact with the JS, and a reference to your native instance for interacting
    // with that.
    .getter("x", [](duktape::api& stack, circle& instance){
      // Getter for "var posX = mycircle.x;".
      // What you push on the stack will be returned in the
      // JS engine. It could also be an object or array etc.
      // Note that the circle reference is intentionally not
      // a const reference, even for a getter. This is because
      // the native methods you have to call may not be const,
      // or you may want side effects, like counting read accesses.
      stack.push(instance.x);
    })
    .setter("x", [](duktape::api& stack, circle& instance){
      // Setter for x. If you omit it, the property is readonly.
      // Modify your instance according to the input argument
      // on the top of the JS stack. 
      instance.x = stack.get<double>(-1);
    })
    // Other getters/setters. Boring.
    .getter("y", [](duktape::api& stack, circle& instance){
      stack.push(instance.y);
    })
    .setter("y", [](duktape::api& stack, circle& instance){
      instance.y = stack.get<double>(-1);
    })
    .getter("r", [](duktape::api& stack, circle& instance){
      stack.push(instance.r);
    })
    .setter("r", [](duktape::api& stack, circle& instance){
      instance.r = stack.get<double>(-1);
    })
    // Of corse you can use the wrapper to "fake" members of
    // the native object. There is no diameter "d" in the 
    // class at all, but for JS users it might be convenient 
    // to have it.
    .getter("d", [](duktape::api& stack, circle& instance){
      stack.push(instance.r * 2.0);
    })
    .setter("d", [](duktape::api& stack, circle& instance){
      instance.r = stack.get<double>(-1) * 0.5;
    })
    // Let's provide the area also as a readonly property. 
    .getter("a", [](duktape::api& stack, circle& instance){
      stack.push(instance.area());
    })
    // Methods can be added, too: Works in the same way as
    // getters and setters, except that you can have multiple
    // arguments on the stack. That is why you have to return
    // true or false, depending if you have a return value,
    // or not (`void`). If you want to return a value, push
    // it on the stack and return true.
    .method("move", [](duktape::api& stack, circle& instance) {
      // Let's do this more detailed, even if example lambda:
      using namespace duktape;
      if(stack.top()==0) {
        // I frequently forget the args ...
        throw script_error("Circle.move() Move where? Say move(x,y) or move([x,y]).");
      } else if(stack.top()==1 && stack.is_array(0)) {
        // Given as circle.move([x,y]);
        auto coords = stack.req<vector<double>>(0); // req throws if a value is not double
        if(coords.size()!=2) throw script_error("Circle.move(array) coordinate arrays must have two elements [x,y].");
        instance.move(coords[0],coords[1]);
      } else if(stack.top()==2) {
        // Given as "circle.move(x,y);"
        instance.move(stack.req<double>(0),stack.req<double>(1));
      } else {
        // Enough for now, you have the gist of it ;)
        throw script_error("Circle.move(): invalid arguments.");
      }
      // Return a ref to `this` for method chaining.
      stack.push_this();
      return true;
    })
    // More methods
    .method("area", [](duktape::api& stack, circle& instance) {
      stack.push(instance.area());
      return true;
    })
    .method("circumference", [](duktape::api& stack, circle& instance) {
      stack.push(instance.circumference());
      return true;
    })
    // Although there is a default toString() generated for your
    // native objects, you may want to overwrite this method with
    // a more detailed one. The default only returns the name of
    // the JS class and native class from the `typeid(T).name()`.
    .method("toString", [](duktape::api& stack, circle& instance) {
      stack.push(std::string("Circle{x:")
        + std::to_string(instance.x)
        + std::string(",y:")
        + std::to_string(instance.y)
        + std::string(",r:")
        + std::to_string(instance.r)
        + std::string("}"));
      return 1;
    })
  );

  // Run the example script, catching eveything we can.
  try {
    js.include("example.js");
    return 0;
  } catch(const duktape::exit_exception& e) {
    return e.exit_code();
  } catch(const duktape::script_error& e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  } catch(const duktape::engine_error& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch(const std::exception& e) {
    cerr << "Fatal: " << e.what() << endl;
    return 1;
  } catch (...) {
    throw;
  }
  return 1;
}
