
const is_linux = sys.uname().sysname.search(/linux/i)===0;

if(sys.mmap === undefined) {
  test_note("Skipping test, sys.mmap() not compiled in.");
  exit(0);
}

function expect_except(comment, fn)
{
  comment = (!comment) ? ("") : (" ("+comment+")");
  if(typeof(fn)!=='function') test_fail("NO FUNCTION PASSED IN");
  try {
    fn();
    test_fail("Exception was expeted" + comment + ".");
  } catch(ex) {
    test_pass("Expected exception: '"+ex.message+"'" + comment + ".");
  }
}

function expect_noexcept(fn)
{
  if(typeof(fn)!=='function') test_fail("NO FUNCTION PASSED IN");
  try {
    return fn();
  } catch(ex) {
    test_fail("Unexpected exception: '"+ex.message+"'.");
  }
}

expect_except("Readonly flag on nonexisting file must throw", function(){ new sys.mmap("test0-notthere.mmap", "r", 1024) });
expect_except("No-create flag on nonexisting file must throw", function(){ new sys.mmap("test0-notthere.mmap", "wn", 1024) });
expect_except("No-create flag on nonexisting file must throw", function(){ new sys.mmap("test0-notthere.mmap", "wnrsp", 1024) });
expect_except("Zero-size must throw", function(){ new sys.mmap("test0-notthere.mmap", "w", 0) });
expect_except("Negative must throw", function(){ new sys.mmap("test0-notthere.mmap", "w", -10) });
expect_except("Invalid number of args", function(){ new sys.mmap("test0-notthere.mmap", "w") });
expect_except("Invalid number of args", function(){ new sys.mmap("test0-notthere.mmap") });
expect_except("Invalid number of args", function(){ new sys.mmap() });
expect_except("Invalid flag", function(){ new sys.mmap("test0-notthere.mmap", "wnxq", 10) });
expect_except("Invalid path", function(){ new sys.mmap("", "wn", 10) });

//---------

const mmap_file = test_relpath("test1.mmap");
const mmap_size = 1024;

test_note("mmap_file = ", mmap_file);
test_note("mmap_size = ", mmap_size);

const mmap1 = new sys.mmap(mmap_file, "wr", mmap_size);
test_expect(typeof(mmap1) === "object");
test_expect(mmap1.size == mmap_size);
test_expect(mmap1.length == mmap_size);
test_expect(mmap1.offset == 0);
test_expect(mmap1.path == mmap_file);
test_expect(mmap1.error == "");
test_expect(mmap1.closed === false);
test_expect(mmap1.sync() === mmap1);
test_expect(mmap1.close() === mmap1);
test_expect(mmap1.closed === true);
test_expect(mmap1.sync() === mmap1);

const mmap2 = new sys.mmap(mmap_file, "wrn", mmap_size);
const mmap3 = new sys.mmap(mmap_file, "wrn", mmap_size);
test_expect(typeof(mmap2) === "object");
test_expect(mmap2.size == mmap_size);
test_expect(mmap2.length == mmap_size);
test_expect(mmap2.offset == 0);
test_expect(mmap2.path == mmap_file);
test_expect(mmap2.error == "");
test_expect(mmap2.closed === false);
test_expect(mmap3.closed === false);

test_expect( mmap2.set(0, 0xaa) );
test_expect( mmap3.get(0) === 0xaa );
test_expect( mmap2.set(0, 0x55) );
test_expect( mmap3.get(0) === 0x55 );
test_note( typeof(mmap3.get(0,2)) );
test_expect( typeof(mmap3.get(0,2)) === 'object' );
test_expect( new Uint8Array(mmap3.get(0,2)).length == 2 );
test_expect( new Uint8Array(mmap3.get(0,2))[0] == 0x55 );
test_expect( new Uint8Array(mmap3.get(0,2))[1] == 0x00 );
test_expect( mmap2.set(0, new Uint8Array([1,2,3])) == mmap2 );

test_expect( mmap2.set(mmap_size-1, 0xaa) );
test_expect( mmap3.get(mmap_size-1) === 0xaa );
test_expect( mmap2.set(mmap_size-1, 0x55) );
test_expect( mmap3.get(mmap_size-1) === 0x55 );

expect_except(".set: no arguments", function(){mmap2.set()} );
expect_except(".set: no value argument", function(){mmap2.set(0)} );
expect_except(".set: invalid type", function(){mmap2.set(0, [0xaa,0x00,0x00])} );
expect_except(".set[3] exceeding the mem range", function(){mmap2.set(mmap_size-2, new Uint8Array([1,2,3]))} );
expect_except(".set: exceeding the mem range", function(){mmap2.set(mmap_size, 0xaa)} );
expect_except(".get: exceeding the mem range", function(){mmap2.get(mmap_size, 0xaa)} );

test_expect(mmap2.close() === mmap2);
test_expect(mmap3.close() === mmap3);
