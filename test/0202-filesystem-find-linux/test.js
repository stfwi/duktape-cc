var files;

comment("used test directory structure contructed in the c++ test part TESTDIR=", testdir);
try {
  fs.find();
  fail("fs.find(): No exception on missing path argument.");
} catch(ex) {
  pass("fs.find(): Exception on missing path argument ('"+ex+"').");
}
try {
  fs.find("");
  fail("fs.find(''): No exception on empty path.");
} catch(ex) {
  pass("fs.find(''): Exception on empty path ('"+ex+"').");
}

expect( fs.find(".") !== undefined, 'fs.find(".") !== undefined' );

// file test.js is in the current directory.
// existing root path
// pattern specified with string (2nd arg)
comment( 'find with arg1 string fnmatch pattern -> list only test.js')
  && comment('files=fs.find(".", "test.js") =', files=fs.find(".", "test.js"))
  && expect( files !== undefined, 'files !== undefined')
  && expect( files.length !== undefined, 'files.length !== undefined')
  && expect( files.length === 1, 'files.length === 1')
  ;

// Test dir structure generated in c++ context.
//
comment( 'find without arg1 -> list everything')
  && comment( 'files=fs.find(testdir) =', files=fs.find(testdir))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  ;

comment( 'find with string arg1 -> list all files and dirs with the name "b"')
  && comment( 'files=fs.find(testdir, "b") =', files=fs.find(testdir, "b"))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  ;

comment( 'find with pattern def in object -> list all files and dirs with the name "b"')
  && comment( 'files=fs.find(testdir, {name:"b"}) =', files=fs.find(testdir, {name:"b"}))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  ;

comment( 'find with pattern and depth -> there is only one dir with the name "b"')
  && comment( 'files=fs.find(testdir, {name:"b", depth:1}) =', files=fs.find(testdir, {name:"b", depth:1}))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  && expect(files.length === 1, 'files.length === 1')
  ;

comment( 'find with only depth -> list all root dir entries')
  && comment( 'files=fs.find(testdir, {depth:1}), files=', files=fs.find(testdir, {depth:1}))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  ;

comment( 'find with filter callback returning true -> list all')
  && comment( 'files=fs.find(testdir, {filter:function(path){return true;}}), files=', files=fs.find(testdir, {filter:function(path){return true;}}))
  && expect(!!files, '!!files')
  && expect(!!files.length, '!!files.length')
  ;

comment( 'find with filter callback returning nothing --> return empty array')
  && comment( 'files=fs.find(testdir, {filter:function(path){return;}}), files=', files=fs.find(testdir, {filter:function(path){return;}}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === 0, 'files.length === 0')
  ;

comment( 'find with filter callback returning a boolean string match --> return filtered array')
  && comment( 'files=fs.find(testdir, {filter:function(path){return path.search(/a\\/b.*?c$/) >= 0;}}), files=',
     files=fs.find(testdir, {filter:function(path){return path.search(/a\/b.*?c$/) >= 0;}}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === 1, 'files.length === 1')
  ;

comment( 'find with filter callback returning a string --> return replaced array')
  && comment( 'files=fs.find(testdir, {filter:function(path){return "#";}}), files=',
     files=fs.find(testdir, {filter:function(path){return "#";}}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length > 0, 'files.length > 0')
  && expect(files[0] === "#", 'files[0] === "#"')
  ;

comment( 'find with type filter (directories)')
  && comment( 'files=fs.find(testdir, {type: "d"}), files=',
     files=fs.find(testdir, {type: "d"}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === 1+num_dirs, 'files.length === 1+num_dirs')
  ;

comment( 'find with type filter (files)')
  && comment( 'files=fs.find(testdir, {type: "f"}), files=',
     files=fs.find(testdir, {type: "f"}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === num_files, 'files.length === num_files')
  ;

comment( 'find with type filter (symlinks), physical search (links listed as the links, not the link targets)')
  && comment( 'files=fs.find(testdir, {type: "l", logical:false}), files=',
     files=fs.find(testdir, {type: "l", logical:false}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === num_symlinks, 'files.length === num_symlinks')
  ;

comment( 'find with type filter (directories and symlinks and files)')
  && comment( 'files=fs.find(testdir, {type: "fdl"}), files=',
     files=fs.find(testdir, {type: "fdl"}))
  && expect(!!files, '!!files')
  && expect(files.length !== undefined, 'files.length !== undefined')
  && expect(files.length === 1+num_dirs+num_symlinks+num_files, 'files.length === 1+num_dirs+num_symlinks+num_files')
  ;
