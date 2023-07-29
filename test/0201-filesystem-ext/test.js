//
// Test script for fs.find().
// Directory structure is generated in c++ context before this script starts.
//

var files; // re-used also to check from c++ context - hence, global scoped.

test_comment("Used test directory structure contructed in the c++ test part TESTDIR=", testdir);

(function(){
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
})();

(function(){
  test_comment( 'Find with arg1 string fnmatch pattern -> list only test.js');
  test_comment( '|  files=fs.find(".", "test.js") =', files=fs.find(".", "test.js"));
  test_expect(files && (files.length === 1));
})();

(function(){
  test_comment( 'Find without arg1 -> list everything');
  test_comment( '|  files=fs.find(testdir) =', files=fs.find(testdir));
  test_expect(files && files.length);

  test_comment( 'Find with string arg1 -> list all files and dirs with the name "b"');
  test_comment( '|  files=fs.find(testdir, "b") =', files=fs.find(testdir, "b"));
  test_expect(files && files.length);

  test_comment( 'Find with pattern def in object -> list all files and dirs with the name "b"');
  test_comment( '|  files=fs.find(testdir, {name:"b"}) =', files=fs.find(testdir, {name:"b"}));
  test_expect(files && files.length);

  test_comment( 'Find with pattern and depth -> there is only one dir with the name "b"');
  test_comment( '|  files=fs.find(testdir, {name:"b", depth:0}) =', files=fs.find(testdir, {name:"b", depth:0}));
  test_expect(files && (files.length === 1));

  test_comment( 'Find with only depth -> list all root dir entries');
  test_comment( '|  files=fs.find(testdir, {depth:0}), files=', files=fs.find(testdir, {depth:0}));
  test_expect(files && files.length);

  test_comment( 'Find with filter callback returning true -> list all');
  test_comment( '|  files=fs.find(testdir, {filter:function(path){return true;}}), files=', files=fs.find(testdir, {filter:function(path){return true;}}));
  test_expect(files && files.length);

  test_comment( 'Find with filter callback returning nothing --> return empty array');
  test_comment( '|  files=fs.find(testdir, {filter:function(path){return;}}), files=', files=fs.find(testdir, {filter:function(path){return;}}));
  test_expect(files && (files.length === 0));

  test_comment( 'Find with filter callback returning a boolean string match --> return filtered array');
  test_comment( '|  files=fs.find(testdir, {filter:function(path){return (path.search(/a\\/b.*?c$/) >= 0) || (path.search(/a\\\\b.*?c$/) >= 0);}}), files=',
      files=fs.find(testdir, {filter:function(path){return (path.search(/a\/b.*?c$/) >= 0) || (path.search(/a\\b.*?c$/) >= 0);}}));
  test_expect(files && (files.length === 1));

  test_comment( 'Find with filter callback returning a string --> return replaced array');
  test_comment( '|  files=fs.find(testdir, {filter:function(path){return "#";}}), files=',
      files=fs.find(testdir, {filter:function(path){return "#";}}));
  test_expect(files && (files.length > 0) && (files[0] === "#"));

  test_comment( 'Find with type filter (directories)');
  test_comment( '|  files=fs.find(testdir, {type: "d"}), files=', files=fs.find(testdir, {type: "d"}));
  test_expect(files && (files.length === num_dirs));

  test_comment( 'find with type filter (files)');
  test_comment( '|  files=fs.find(testdir, {type: "f"}), files=', files=fs.find(testdir, {type: "f"}));
  test_expect(files && (files.length === num_files));

  test_comment( 'Find with type filter (symlinks), physical search (links listed as the links, not the link targets)');
  test_comment( '|  files=fs.find(testdir, {type: "l", logical:false}), files=', files=fs.find(testdir, {type: "l", logical:false}));
  test_expect(files && (files.length === num_symlinks));

  test_comment( 'Find with type filter (directories and symlinks and files)');
  test_comment( '|  files=fs.find(testdir, {type: "fdl"}), files=', files=fs.find(testdir, {type: "fdl"}));
  test_expect(files && (files.length === num_dirs+num_symlinks+num_files));
})();

(function(){
  test_comment( 'Find with pattern containing additional ? or *');
  test_comment( '|  files=fs.find(testdir, {name:"?-file"}) =', files=fs.find(testdir, {name:"?-file"})); // -> a/a-file
  test_expect(files && files.length==1);
  test_comment( '|  files=fs.find(testdir, {name:"*-file"}) =', files=fs.find(testdir, {name:"*-file"})); // -> a/a-file
  test_expect(files && files.length==1);
  test_comment( '|  files=fs.find(testdir, {name:"?-fi*"}) =', files=fs.find(testdir, {name:"?-fi*"})); // -> a/a-file
  test_expect(files && files.length==1);
})();

var found_files=false;
(function(){
  test_comment( 'Find in home using "~/" prefix. Aborted after first entry.');
  try { fs.find("~/", {name:"*", filter:function(path){found_files=true; throw 1;}}); } catch(ignored) {}
  test_comment( '|  files=fs.find("~/", {name:"*", filter:function(){throw...}})');
  test_expect(found_files===true);
})();

var found_exception=false;
(function(){
  test_comment( 'Find filter function return value check, returning an int should throw.');
  try { fs.find(testdir, {name:"*", filter:function(){return 1}}); } catch(ignored) {found_exception=true}
  test_comment( '|  files=fs.find(testdir, {name:"*", filter:function(){return 1}})');
  test_expect(found_exception===true);
})();

(function(){
  found_exception=false;
  test_comment( 'Find filter function no-directory exception check.');
  try { fs.find(testdir+"-not-really-there", {name:"*.tmp"}); } catch(ignored) {found_exception=true}
  test_expect(found_exception===true);
})();

(function(){
  found_exception=false;
  test_comment( 'Find filter function double spec if filter function.');
  try { fs.find(testdir, {name:"*.tmp", filter:function(path){return false}}, function(path){return false}); } catch(ignored) {found_exception=true}
  test_expect(found_exception===true);
})();

(function(){
  test_comment( 'Find filter function double spec if filter function.');
  try {
    test_expect( fs.find(testdir, {name:"*"}, function(path){return true}).length > 0 );
  } catch(ex) {
    test_fail("Unexpected exception: " + ex.message)
  }
})();
