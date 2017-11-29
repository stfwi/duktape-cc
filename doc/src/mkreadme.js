var readme_src = fs.readfile(fs.realpath("doc/src/readme.src.md"));

// Include directive
readme_src = readme_src.replace(/\[\]\(\!include[\s]+([\w\d\-\\/\.]+)\)/g, function(m, path) {
  var contents = fs.readfile(fs.realpath(path));
  if(contents === undefined) throw new Error("File not readable or not existing: " + fs.realpath(path));
  return contents;
});

readme_src = readme_src.replace(/[\s]+$/, '') + "\n";

print(readme_src);
