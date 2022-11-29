/**
 * Test sandboxing interface creates a new engine, and evaluates a list of code in its global
 * scope (1st argument: array). The 2nd argument is an optional string with execution options:
 *
 *  - If `no-builtin` is not specified, `print()` and `eval()` is added.
 *  - If `no-eval` is specified, the code is compiled but not evaluated.
 *  - If `strict` is specified, the code compilation/evaluation is "use strict"-like.
 */
function check_sandbox()
{
  test_expect( test_sandboxed([ "const a=1;" ], "no-eval,strict,no-builtin").data === undefined );
  test_expect( test_sandboxed([ "const a=1;", "const b=2;" ], "no-eval,strict,no-builtin").data === undefined );
  test_expect( test_sandboxed([ "const a=1;", "const b=2;", "a+b"], "no-builtin").data === "3" );
  test_note  ( test_sandboxed([ "const a=1;", "const b=2;", "a+b"], "no-eval,no-builtin").data );
  test_expect( test_sandboxed([ "print('Test text from sandbox print(...)')" ], "strict").data === "undefined" );
  test_expect( test_sandboxed([ "(function(){return 1})()" ], "strict").data === "1" );
  test_note  ( test_sandboxed([ "throw 1000" ], "strict").error );
  test_expect( test_sandboxed([ "throw 1000" ], "strict").error === "1000" );
  test_expect( test_sandboxed([ "exit(1)" ], "strict").error !== undefined ); // There is no exit()
}

/**
 * TC39 checking. See harness.md.
 */
function check_tc39_tests()
{
  const get_env = function(key_search) {
    for(var i=0; i<sys.envv.length; ++i) {
      const kv = sys.envv[i].split(/=/, 2);
      if(!kv || kv.shift().search(key_search)<0) continue;
      return (kv.shift()||"").replace(/[\\]/g,"/").replace(/[\s\/]+$/,"");
    }
    return "";
  };

  const get_tc39_root = function() {
    const tc39_root = get_env(/^TC39_262_ROOT$/i);
    if(!tc39_root) {
      test_note("Skipping interpreter test, TC39_262_ROOT not specified in the environment (e.g. 'make test TC39_262_ROOT=<path>').");
      test_note("This TC39 root directory shall be the location to the official test262 repository of the Technical Committee 39.");
      test_note("E.g. clone this from: https://github.com/tc39/test262 .");
      return undefined;
    } else if(!fs.isdir(tc39_root)) {
      test_fail("TC39_262_ROOT root directory not found: '" + tc39_root + "'");
      return undefined;
    } else if(!fs.isdir(tc39_root+"/harness") || !fs.isdir(tc39_root+"/test")) {
      test_fail("TC39_262_ROOT root directory does not contain the sub directories 'harness' and 'test': '" + tc39_root + "'");
      return undefined;
    } else {
      return tc39_root;
    }
  };

  const collect_es5_test_files = function(root_path, options) {
    root_path = (root_path || ".").replace(/\\/g, "/").replace(/\/+$/,"") + "/";
    options = options || {};
    const files = fs.find(root_path+"test", {
      name: '*.js',
      filter: function(path) {
        const contents = fs.read(path);
        path = path.replace(/\\/g, "/").substr(root_path.length);
        if(contents && (contents.search(/es5id\s*:/i) >= 0) ) return path.replace(/\\/g, "/");
        if(options.verbose) alert( ((!contents) ? "[skip] Empty file: " : "[skip] No ES5: ") + path);
        if(options.skipped!==undefined) options.skipped.push(path);
        return false;
      }
    });
    files.sort();
    if(options.out_file !== undefined) {
      fs.write(options.out_file, JSON.stringify(files, null, 1));
    }
    return files;
  };

  const get_test_list = function(tc39_root, filter_text) {
    filter_text = filter_text || "";
    const json_list = "test-list.json";
    if(!tc39_root) {
      return [];
    } else if(!fs.isfile(json_list)) {
      test_note("Collecting ES5 test files ...");
      fs.mkdir("tc39");
      collect_es5_test_files(tc39_root, {out_file:json_list, verbose:true});
    }
    test_note("Reading test list ...");
    return JSON.parse(fs.read(json_list)).filter(function(path){
      return (!filter_text) || path.indexOf(filter_text)>=0;
    });
  };

  const unpack_harness = function() {
    const sections = fs.read("harness.md").split(/^###[\s]*/mi);
    sections.shift(); // Intro header.
    const harness = {};
    sections.filter(function(sect){
      sect = sect.split(/[\r]?[\n]/);
      const name = sect.shift().toLowerCase().replace(/\.js$/,"");
      sect = sect.map(function(line){ return line.replace(/^[\s]{2}/,"") }).join("\n");
      harness[name] = sect;
    });
    const always_include = [ harness["sta"], harness["asserts"] ];
    delete harness["sta"];
    delete harness["asserts"];
    Object.freeze(harness);
    return {
      "always": always_include,
      "include": harness
    }
  };

  const dump_code = function(code) {
    var line_no = 0;
    code = code
      .split(/[\n]/)
      .map(function(line){
        var line_no_str = (""+(++line_no));
        while(line_no_str.length<4) line_no_str = " " + line_no_str;
        return line_no_str + "| " + line;
      })
      .join("\n");
    test_note("#--\n" + code);
  }

  const run_test = function(tc39_root, harness, test_rel_path, ignored) {
    ignored = ignored || {paths:[], contents:[]};
    if(ignored.paths.indexOf(test_rel_path)>=0) return "skip";

    const test_code = fs.read(tc39_root+"/"+test_rel_path);
    if(!test_code) { test_fail("Could not read '"+tc39_root+"/"+test_rel_path+"'"); return "fail"; };
    // Ignore conditions
    for(var i=0; i<ignored.contents.length; ++i) {
      const im = ignored.contents[i];
      if((typeof(im) === "string") && (test_code.indexOf(im) >= 0)) return "skip";
      if((typeof(im) === "function") && (im(test_code))) return "skip";
      if((typeof(im) === "object") && (test_code.search(im) >= 0)) return "skip";
    }
    // Specified "includes: [.....]"
    const includes = (function(){
      var inc = test_code.match(/[\n][\s]*includes:(.*?)[\n]/mi);
      if(!inc) return [];
      const incs = inc[1].replace(/^[\s\[]+/,"").replace(/[\s\]]+$/,"").toLowerCase().replace(/\s/g,"").split(/[,;]/).map(function(e){return e.replace(/\.js$/,"")});
      return incs.map(function(e){
        if(harness.include[e] === undefined) throw new Error("Unknown include: '"+e+"' (in '"+inc[1]+"')");
        return harness.include[e];
      });
    })();

    // Specified "flags: [.....]"
    const sandbox_flags = (function(){
      var flags = test_code.match(/[\n][\s]*flags:(.*?)[\n]/mi);
      if(!flags) return "";
      flags = flags[1].replace(/^[\s\[]+/,"").replace(/[\s\]]+$/,"").toLowerCase().replace(/\s/g,"").split(/[,;]/);
      const sandbox_flags = [];
      if(flags.length > 0) {
        if((flags.indexOf("nostrict")<0) && ((flags.indexOf("strict")>=0) || (flags.indexOf("onlystrict")>=0))) {
          sandbox_flags.push("strict");
        }
        test_note("test-flags: " + flags.join(",") + " / " + JSON.stringify(sandbox_flags));
      }
      if(test_code.indexOf("$DONOTEVALUATE")>=0) {
        sandbox_flags.join("noeval");
      }
      return sandbox_flags.join(",");
    })();
    // Run
    const sandbox_seq = [].concat(harness.always);
    includes.filter(function(e){ sandbox_seq.push(e); });
    sandbox_seq.push(test_code);
    const result = test_sandboxed(sandbox_seq, sandbox_flags);
    const expect_error = test_code.search(/[\n][\s]*negative:[\s]*[\n]/)>=0;
    // Evaluate
    if(result.error !== undefined) {
      test_note("test-result: " + JSON.stringify(result));
      if(result.dependency_index !== undefined) {
        dump_code(sandbox_seq[result.dependency_index]);
        return "fail";
      }
      if(expect_error) {
        test_note("expected-error");
        return "pass";
      } else {
        dump_code(test_code);
        return "fail";
      }
    } else {
      if(expect_error) {
        test_note("!Error was expected");
        return "fail";
      }
      if(result.data !== "" && result.data !== "undefined") {
        test_note("test-result: " + JSON.stringify(result));
      }
      return "pass";
    }
  };

  const ignored = eval(fs.readfile("ignore-list.js"));
  const filter_text = "";
  const tc39_root = get_tc39_root();
  const tests = get_test_list(tc39_root, filter_text);
  const harness = unpack_harness();
  const num_tests = tests.length;
  for(var i=0; i<tests.length; ++i) {
    const line = "[tc39|"+(i+1)+"/"+num_tests+"] " + tests[i];
    switch(run_test(tc39_root, harness, tests[i], ignored)) {
      case "pass": { test_pass(line); break; }
      case "skip": { test_note("Skipped: " + line); break; }
      case "fail": { test_fail(line); break; }
      default: { throw new Error("Unexpected test run result tag"); }
    }
  }
}


check_sandbox();
check_tc39_tests();
