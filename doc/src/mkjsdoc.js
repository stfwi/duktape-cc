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
function scan_js_docs() {
  var docs = {};
  fs.find(fs.realpath(sys.app.path + "../../duktape"), {
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

// result to stdout
console.write(scan_js_docs());
