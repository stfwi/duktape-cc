#!/usr/bin/djs
"use strict";

/**
 * Removes indentation spaces on all lines of the given text.
 * @param {string} text
 * @return {string}
 */
function unindent(text) {
  // Remove indent and append function documentation, could maybe be done better ...
  var min_leading_spaces = -1;
  var lines = text.replace(/[\s]+$/g, "").replace(/\t/g, " ").split("\n");
  for(var i in lines) {
    if(!lines[i].length) continue;
    var p = lines[i].search(/[^\s]/);
    if((p >= 0) && ((p < min_leading_spaces) || (min_leading_spaces < 0))) {
      min_leading_spaces = p;
    }
  }
  if(min_leading_spaces > 0) {
    for(var i in lines) {
      if(lines[i].length > min_leading_spaces) {
        lines[i] = lines[i].substr(min_leading_spaces);
      }
    }
  }
  return lines.join("\n");
}

/**
 * Generates JSDoc from the corresponding sections in the ".hh" files of the modules.
 * Note: Done in one go: fs.find() has a filter callback for each file, in which the
 *       files are directly read. While reading using fs.readfile(), the line filter
 *       callback is used to directly compose the relevant JSDoc sections.
 *
 * @return {String}
 */
function scan_js_docs(root_path) {
  var docs = {};
  const root = fs.realpath(root_path);
  if(!root) throw "Could not find duktape root path '"+root_path + "/duktape'";
  fs.find(root, {
    name:"*.hh",
    type:"f",
    filter: function(path) {
      var doc = "";
      var doc_span = "";
      var is_doc_span = false;
      fs.readfile(path, function(line) {
        line = line.replace(/[\s]+$/, "");
        if(line != "") {
          if(line.search(/^[\s]*?#if[\s]*?\([\s]*?0[\s]*?&&[\s]*?JSDOC[\s]*?\)/) >= 0) {
            doc_span = "";
            is_doc_span = true;
          } else if(line.search(/^[\s]*?#endif[\s]*?$/) >= 0) {
            if(is_doc_span) {
              doc_span = doc_span.replace(/[\s]+$/g,"").replace(/^[\r\n]+/g,"");
              doc += unindent(doc_span) + "\n\n";
            }
            doc_span = "";
            is_doc_span = false;
          } else if(is_doc_span) {
            doc_span += line + "\n";
          }
        }
      });
      if(doc.replace(/[\s]/ig,"").length > 0) {
        docs[fs.basename(path)] = doc;
      }
    }
  });

  // First fixed order, then by sort order
  var sorted_docs = "";
  var append_doc = function(key) {
    sorted_docs += "/** @file: " + key + " */\n\n";
    sorted_docs += docs[key]; delete docs[key];
  };
  append_doc("duktape.hh");
  append_doc("mod.stdlib.hh");
  append_doc("mod.stdio.hh");
  append_doc("mod.fs.hh");
  append_doc("mod.fs.ext.hh");
  append_doc("mod.fs.file.hh");
  append_doc("mod.sys.hh");
  append_doc("mod.sys.exec.hh");
  for(var key in docs) {
    sorted_docs += "/** @file: " + key + " */\n\n";
    sorted_docs += docs[key];
  }
  return (sorted_docs.replace(/[\s]+$/, "") + "\n").replace(/[\r]/g,'');
}

/**
 * Generates the function list from scanned JS docs.
 * @param {string} text_input
 * @returns {string}
 */
function make_function_list(text)
{
  text = text.split(/\n/);

  const objects = {};
  for(var i in text) {
    if(text[i].match(/^[\w\d\.]+[\s]+=[\s]+function\([^\)]+\)[\s{};]+/)) {
      const line = text[i]
        .replace(/^[\s]+/,"")
        .replace(/[\s]+$/,"")
        .replace(/[\s]*=[\s]*function[\s]*/,"")
        .replace(/[\s;{}]+$/g,"")
      ;
      const o = line.match(/[\.]/) ? line.replace(/[\.].*$/,"") : "";
      if(objects[o] === undefined) objects[o] = [];
      objects[o].push(line);
    }
  }

  const list_object_functions = function(it) {
    const replace_list = {
      "": "Global namespace",
      "fs": "File system object",
      "sys": "System object",
      "console": "Console object"
    };
    var text = "";
    if(replace_list[it] !== undefined) {
      text += "### " + replace_list[it] + "\n\n";
    } else {
      text += "### " + it + " object" + "\n\n";
    }
    for(var it2 in objects[it]) {
      text += "  - " + objects[it][it2] + "\n";
    }
    text += "\n\n";
    return text;
  }

  text = "";
  for(var it in objects) {
    text += list_object_functions(it);
  }
  text = text.replace(/[\s]+$/,'\n').replace(/[\r]/g, '');
  return text;
}

/**
 *
 * @param {string} readme_src_path
 * @param {object} variables
 * @returns {string}
 */
function make_readme(readme_src_path, variables)
{
  readme_src_path = fs.realpath(readme_src_path);
  if(!readme_src_path) throw "Could not find readme source file.";
  var text = fs.readfile(readme_src_path);
  if(!text) throw "Failed to read readme source file.";

  // [](!var VARIABLE_NAME) replacement.
  text = text.replace(/\[\]\(\!var[\s]+([\w\d\-\\/\.]+)\)/g, function(m, var_name) {
    if(variables[var_name] !== undefined) return variables[var_name];
    throw new Error("Missing replacement variable '"+var_name+"'");
  });

  text = (text.replace(/[\s]+$/, '') + "\n").replace(/[\r]/g,'');
  return text;
}

function main(args)
{
  const task = args.shift();
  const scanned = scan_js_docs("./duktape");
  if(task === "--stdmods") {
    console.write(scanned);
    return 0;
  } else if(task !== "--readme") {
    alert("Invalid task, use --stdmods or --readme.");
    return 1;
  } else {
    const variables = {
      function_list: make_function_list(scanned)
    }
    const readme = make_readme("doc/src/readme.src.md", variables);
    console.write(readme);
    return 0;
  }
}

main(sys.args);
