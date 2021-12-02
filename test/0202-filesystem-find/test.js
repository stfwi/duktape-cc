var files;

test_comment("used test directory structure contructed in the c++ test part TESTDIR=", testdir);
try {
  fs.find();
  test_fail("fs.find(): No exception on missing path argument.");
} catch(ex) {
  test_pass("fs.find(): Exception on missing path argument ('"+ex+"').");
}
try {
  fs.find("");
  test_fail("fs.find(''): No exception on empty path.");
} catch(ex) {
  test_pass("fs.find(''): Exception on empty path ('"+ex+"').");
}

test_expect( fs.find(".") !== undefined );

// file test.js is in the current directory.
// existing root path
// pattern specified with string (2nd arg)
test_comment( 'find with arg1 string fnmatch pattern -> list only test.js');
test_comment('files=fs.find(".", "test.js") =', files=fs.find(".", "test.js"));
test_expect( files !== undefined );
test_expect( files.length !== undefined );
test_expect( files.length === 1 );

// Test dir structure generated in c++ context.
//
test_comment( 'find without arg1 -> list everything');
test_comment( 'files=fs.find(testdir) =', files=fs.find(testdir));
test_expect(!!files);
test_expect(!!files.length);

test_comment( 'find with string arg1 -> list all files and dirs with the name "b"');
test_comment( 'files=fs.find(testdir, "b") =', files=fs.find(testdir, "b"));
test_expect(!!files);
test_expect(!!files.length);

test_comment( 'find with pattern def in object -> list all files and dirs with the name "b"');
test_comment( 'files=fs.find(testdir, {name:"b"}) =', files=fs.find(testdir, {name:"b"}));
test_expect(!!files);
test_expect(!!files.length);

test_comment( 'find with pattern and depth -> there is only one dir with the name "b"');
test_comment( 'files=fs.find(testdir, {name:"b", depth:0}) =', files=fs.find(testdir, {name:"b", depth:0}));
test_expect(!!files);
test_expect(!!files.length);
test_expect(files.length === 1);

test_comment( 'find with only depth -> list all root dir entries');
test_comment( 'files=fs.find(testdir, {depth:0}), files=', files=fs.find(testdir, {depth:0}));
test_expect(!!files);
test_expect(!!files.length);

test_comment( 'find with filter callback returning true -> list all');
test_comment( 'files=fs.find(testdir, {filter:function(path){return true;}}), files=', files=fs.find(testdir, {filter:function(path){return true;}}));
test_expect(!!files);
test_expect(!!files.length);

test_comment( 'find with filter callback returning nothing --> return empty array');
test_comment( 'files=fs.find(testdir, {filter:function(path){return;}}), files=', files=fs.find(testdir, {filter:function(path){return;}}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === 0);

test_comment( 'find with filter callback returning a boolean string match --> return filtered array');
test_comment( 'files=fs.find(testdir, {filter:function(path){return (path.search(/a\\/b.*?c$/) >= 0) || (path.search(/a\\\\b.*?c$/) >= 0);}}), files=',
     files=fs.find(testdir, {filter:function(path){return (path.search(/a\/b.*?c$/) >= 0) || (path.search(/a\\b.*?c$/) >= 0);}}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === 1);

test_comment( 'find with filter callback returning a string --> return replaced array');
test_comment( 'files=fs.find(testdir, {filter:function(path){return "#";}}), files=',
     files=fs.find(testdir, {filter:function(path){return "#";}}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length > 0);
test_expect(files[0] === "#");

test_comment( 'find with type filter (directories)');
test_comment( 'files=fs.find(testdir, {type: "d"}), files=', files=fs.find(testdir, {type: "d"}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === num_dirs);

test_comment( 'find with type filter (files)');
test_comment( 'files=fs.find(testdir, {type: "f"}), files=', files=fs.find(testdir, {type: "f"}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === num_files);

test_comment( 'find with type filter (symlinks), physical search (links listed as the links, not the link targets)');
test_comment( 'files=fs.find(testdir, {type: "l", logical:false}), files=', files=fs.find(testdir, {type: "l", logical:false}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === num_symlinks);

test_comment( 'find with type filter (directories and symlinks and files)');
test_comment( 'files=fs.find(testdir, {type: "fdl"}), files=', files=fs.find(testdir, {type: "fdl"}));
test_expect(!!files);
test_expect(files.length !== undefined);
test_expect(files.length === num_dirs+num_symlinks+num_files);
