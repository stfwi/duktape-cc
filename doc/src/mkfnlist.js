#!/usr/bin/djs

var inp = console.read().split(/\n/);
var objects = {};

for(var i in inp) {
  if(inp[i].match(/^[\w\d\.]+[\s]+=[\s]+function\([^\)]+\)[\s{};]+/)) {
    var line = inp[i]
      .replace(/^[\s]+/,"")
      .replace(/[\s]+$/,"")
      .replace(/[\s]*=[\s]*function[\s]*/,"")
      .replace(/[\s;{}]+$/g,"")
    ;

    var o = line.match(/[\.]/) ? line.replace(/[\.].*$/,"") : "";
    if(objects[o] === undefined) objects[o] = [];
    objects[o].push(line);
  }
}

function list_object_functions(key) {
  var text = "";
  var replace_list = {
    "": "Global namespace",
    "fs": "File system object",
    "sys": "System object",
    "console": "Console object"
  };
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

var text = "";
for(var it in objects) {
  text += list_object_functions(it);
}

text = text.replace(/[\s]+$/,'\n').replace(/[\r]/g, '');
console.write(text);
