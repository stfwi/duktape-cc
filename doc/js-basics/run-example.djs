#!/usr/local/bin/djs -s
(function(){
  const code_tmp_file = "example.djs.tmp"; // eval is more restricted than include, so we write a temp file.
  const mdfile = sys.args.shift();
  const example = sys.args.shift();
  fs.unlink(code_tmp_file);
  var examples = {}, code_line = -1, example_name = "", example_code = "";
  const reset = function() { example_name=""; example_code=""; code_line=-1; }

  fs.read(mdfile, {filter:function(line){
    if(code_line < 0) {
      if(line.search(/^[\s]*```js[\s]*$/) < 0) return;
      reset();
      code_line = 0;
    } else {
      if(line.search(/^[\s]*```/) < 0) {
        example_code += line + "\n";
        if(++code_line == 1) {
          const m = line.match(/^[\s]*[\/]+[\s]*@example[\s]+([^\s]+)[\s]*$/);
          if(!m) reset();
          example_name = m[1].toLowerCase().replace(/[^0-9a-z\-]/g,"");
          if(example_name == "") throw new Error('Invalid example name: "'+m[1]+'" (must be [0-9a-z\-]).');
          if(examples[example_name] !== undefined) throw new Error('Example name already defined: "'+example_name+'"');
        }
      } else if(line.search(/^[\s]*```[\s]*$/) === 0) {
        examples[example_name] = example_code;
        reset();
      } else {
        throw new Error('Expected code block termination (only "```"), but saw "'+line+'".');
      }
    }
  }});

  print("[info] Examples:" + JSON.stringify(Object.keys(examples)));

  if(example!="") {
    for(var key in examples) {
      if(key == example) {
        const code = examples[key];
        examples = {};
        examples[key] = code;
        break;
      }
    }
  }

  for(var key in examples) {
    print("[run ] Running example:" + key);
    fs.write(code_tmp_file, examples[key]+"\n");
    try {
      print("[>>>>]");
      const args = [ "-s", code_tmp_file ];
      sys.args.filter(function(e){args.push(e)});
      const ec = sys.exec({
        program: sys.app.path,
        args: args,
        stdin: "STDIN TEST STRING\NWITH TWO LINES\n",
        stdout: function(l){ print("[sout] " + l)},
        stderr: function(l){ print("[serr] " + l)}
      });
      print("[<<<<]");
      print("[term] Exit code: " + JSON.stringify(ec.exitcode))
      print("[----] ---------------------------------------------------------------");
    } finally {
      fs.unlink(code_tmp_file);
    }
  }

})();
