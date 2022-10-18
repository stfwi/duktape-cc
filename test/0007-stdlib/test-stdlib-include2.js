test_pass("File test-stdlib-include2.js included.");

test_note("Defining global.intval1=1");
global.intval1 = 1;

test_note("Defining file var test_stdlib_include2='test-stdlib-include2' ...");
var test_stdlib_include2 = 'test-stdlib-include2';

test_note("Defining TEST_STDLIB_INCLUDE2_NOVAR = {value:'test'} ...");
TEST_STDLIB_INCLUDE2_NOVAR = {value:'test'};

test_note("Defining array [1,2,3] without variable definition (last instruction) ...");
[1,2,3]
