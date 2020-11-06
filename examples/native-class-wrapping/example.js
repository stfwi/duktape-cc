/**
 * Script for the native_object wrapping example.
 * Here a native "circle" object is projected into
 * the JS engine as "Circle" prototype/constructor.
 */
(function(){
  // Show the type of the object, and the proto template
  // that is attached to the constructor function as "prototype"
  // property-
  print("typeof Circle = ", typeof(Circle) );
  print("typeof Circle.prototype = ", typeof(Circle.prototype) );

  // Two argument variants are allowed in the wrapper, default
  // construction and fully defined:
  const c1 = new Circle();
  const c2 = new Circle(10, 10, 50); // x,y,r
  try { const c3 = new Circle(10, 20); } catch(ex) { print("Initializing c3 with wrong args: ", ex); }

  // toString() is automatically used in print:
  print("c1=", c1);
  print("c2=", c2);

  c1.move(101,102);
  c2.move([201,202]);

  print("moved c1=", c1);
  print("moved c2=", c2);

  print("c1.x=", c1.x, ", c1.y=", c1.y, ", c1.r=", c1.r, ", c1.d=", c1.d, ", c1.a=", c1.a);
  print("c2.x=", c2.x, ", c2.y=", c2.y, ", c2.r=", c2.r, ", c2.d=", c2.d, ", c2.a=", c2.a);

  try { c1.move(); } catch(ex) { print("Moving c1 with empty args: ", ex); }
  try { c1.move(1001); } catch(ex) { print("Moving c1 with wrong args: ", ex); }
  try { c2.move([0,0,0]); } catch(ex) { print("Moving c2 with wrong args: ", ex); }
  try { c1.move(1,"ll"); } catch(ex) { print("Moving c1 with wrong args: ", ex); }

  try { c1.a = 1000; } catch(ex) { print("Setting c1.r: ", ex); }
  try { c1.notthere = 1000; } catch(ex) { print("Setting c1.notthere: ", ex); }
  try { var a=c1.notthere; } catch(ex) { print("Getting c1.notthere: ", ex); }

})();

/// Expected result:
//
//  $ make
//  [c++ ] ../../duktape/duktape.c  duktape.o
//  [c++ ] example.cc  example.o
//  [ld  ] duktape.o example.o  example.elf
//  [run ] binary ...
//  ----------------------------------------------------
//  typeof Circle =  function
//  typeof Circle.prototype =  object
//  Initializing c3 with wrong args:  Error: Circle constructor needs either none or three arguments (x,z,r)
//  c1= Circle{x:0.000000,y:0.000000,r:1.000000}
//  c2= Circle{x:10.000000,y:10.000000,r:50.000000}
//  moved c1= Circle{x:101.000000,y:102.000000,r:1.000000}
//  moved c2= Circle{x:201.000000,y:202.000000,r:50.000000}
//  c1.x= 101 , c1.y= 102 , c1.r= 1 , c1.d= 2 , c1.a= 3.141592653589793
//  c2.x= 201 , c2.y= 202 , c2.r= 50 , c2.d= 100 , c2.a= 7853.981633974483
//  Moving c1 with empty args:  Error: Circle.move() Move where? Say move(x,y) or move([x,y]).
//  Moving c1 with wrong args:  Error: Circle.move(): invalid arguments.
//  Moving c2 with wrong args:  Error: Circle.move(array) coordinate arrays must have two elements [x,y].
//  Moving c1 with wrong args:  TypeError: number required, found 'll' (stack index 1)
//  Setting c1.r:  Error: Native object property a is readonly.
//  Setting c1.notthere:  Error: Native object does not have the property 'notthere'
//  Getting c1.notthere:  Error: Native object does not have the property 'notthere'
//  ----------------------------------------------------

