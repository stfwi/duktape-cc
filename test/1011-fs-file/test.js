
const is_linux = sys.uname().sysname.search(/linux/i)===0;
const test_data_file = "test-data.txt";
fs.unlink(test_data_file);
var file;

// Closed file data.
try {
  test_note('--------------------------------------------------------');
  test_note('Checks for: new fs.file()');
  const file_implicit_construction = fs.file(); // new not strictly required.
  file = new fs.file();
  test_expect( !file.opened() );
  test_expect( file.closed() );
  test_expect( file.eof() );
  try { file.read(); test_fail("file.read() should have thrown") } catch(ex) { test_pass("file.read() threw for closed file."); }
  try { file.readln(); test_fail("file.readln() should have thrown") } catch(ex) { test_pass("file.readln() threw for closed file."); }
  try { file.write(""); test_fail("file.write() should have thrown") } catch(ex) { test_pass("file.write() threw for closed file."); }
  try { file.writeln(""); test_fail("file.writeln() should have thrown") } catch(ex) { test_pass("file.writeln() threw for closed file."); }
  try { file.printf("%d",1); test_fail("file.printf() should have thrown") } catch(ex) { test_pass("file.printf() threw for closed file."); }
  try { file.flush(); test_fail("file.flush() should have thrown") } catch(ex) { test_pass("file.flush() threw for closed file."); }
  try { file.tell(); test_fail("file.tell() should have thrown") } catch(ex) { test_pass("file.tell() threw for closed file."); }
  try { file.seek(); test_fail("file.seek() should have thrown") } catch(ex) { test_pass("file.seek() threw for closed file."); }
  try { file.size(); test_fail("file.size() should have thrown") } catch(ex) { test_pass("file.size() threw for closed file."); }
  try { file.stat(); test_fail("file.stat() should have thrown") } catch(ex) { test_pass("file.stat() threw for closed file."); }
  try { file.sync(); test_fail("file.sync() should have thrown") } catch(ex) { test_pass("file.sync() threw for closed file."); }
  try { file.lock(); test_fail("file.lock() should have thrown") } catch(ex) { test_pass("file.lock() threw for closed file."); }
  file.unlock(); // shall not throw
  file.close(); // shall not throw
} catch(ex) {
  test_fail("Unexpected exception " + ex.message);
}

// Read-open file nonexisting file.
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: new fs.file(test_data_file)');
  file = new fs.file(test_data_file);
  test_fail("new fs.file('not existing file') should have thrown")
} catch(ex) {
  test_pass("new fs.file() threw for not existing file: " + ex.message);
}

// Basic write and consistency
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: fs.file(test_data_file, "w")');
  file = fs.file(test_data_file, "w");
  test_expect( file.opened() );
  test_expect( !file.closed() );
  test_expect( !file.eof() );
  test_expect( file.write("12345\n") );
  test_expect( file.size() == 6 );
  test_expect( file.tell() == 6 );
  test_expect( file.writeln("12345") );
  test_expect( file.size() == 12 );
  test_expect( file.tell() == 12 );
  test_expect( file.printf("%5d\n", 12345) );
  test_expect( file.sync() );
  var file_stats = file.stat();
  test_note( "file.stat() = " + JSON.stringify(file_stats) );
  test_expect( fs.realpath(file_stats.path) == fs.realpath(test_data_file) );
  test_note( "file_stats.size == " + file_stats.size );
  test_note( "file.size() == " + file.size() );
  test_expect( file_stats.size == file.size() );
  file_stats = undefined;
  file.close();
} catch(ex) {
  test_fail("Unexpected exception: " + ex.message)
}

// Basic read-write and consistency
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: fs.file(test_data_file, "rw")');
  const prev_size = fs.size(test_data_file);
  test_expect( prev_size > 0 );
  file = fs.file(test_data_file, "rw+");
  test_expect(!file.closed() );
  test_expect(!file.eof() );
  test_expect( file.opened() );
  test_expect( file.size() == prev_size );
  test_expect( file.seek(5) === 5 );
  test_expect( file.seek(0) === 0 );
  test_expect( file.tell() === 0 );
  test_expect( file.write("0123456789\n0123456789\n0123456789\n")  );
  test_expect( file.seek(0, 'end') === file.size() );
  test_expect( file.seek(0, 'seek_end') === file.size() );
  test_expect( file.seek(0, 'start') === 0 );
  test_expect( file.seek(0, 'begin') === 0 );
  test_expect( file.seek(1, 'start') === 1 );
  test_expect( file.seek(0, 'cur') === 1 );
  test_expect( file.seek(1, 'current') === 2 );
  test_expect( file.seek(1, 'seek_cur') === 3 );
  test_expect( file.seek(0) === 0 );
  test_expect( file.read(10) == "0123456789" ); // read 10 bytes
  test_expect( file.tell() === 10 );
  test_expect( file.read(1000) == "\n0123456789\n0123456789\n" ); // read rest, further reads will trigger EOF
  test_expect(!file.read(1) );
  test_expect( file.tell() === file.size() );
  test_expect( file.writeln("abcdef") );
  test_expect( file.printf("%s %d %08x\n", "ghihk", 10, 0x01234567) );
  test_expect( file.sync() );
  test_expect( file.flush() );
  test_expect( file.lock() );
  test_expect( file.unlock() );
} catch(ex) {
  test_fail("Unexpected exception: " + ex.message)
}

// open
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: fs.file.open(test_data_file, "rw")');
  const prev_size = fs.size(test_data_file);
  test_expect( prev_size > 0 );
  file = fs.file();
  test_expect( file.closed() );
  test_expect( file.open(test_data_file, "w") );
  test_expect( file.write("TEST") );
  test_expect(!file.closed() );
  test_expect( file.close() );
  test_expect( file.closed() );
  test_expect( file.open(test_data_file, "r") );
  test_expect( file.read() == "TEST" );
  test_expect(!file.closed() );
  try { file.seek(-1); test_fail("file.seek(-1) should have thrown.") } catch(ex) { test_pass("file.seek(-1) threw."); }
  test_expect( file.close() );
} catch(ex) {
  test_fail("Unexpected exception: " + ex.message)
}

// open/constructor open options
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: fs.file.open(file, OPTIONS)');
  file = fs.file(test_data_file);
  try { test_expect( file.open(test_data_file, "r") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // read
  file.close();
  try { test_expect( file.open(test_data_file, "w") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // write (clear contents)
  file.close();
  try { test_expect( file.open(test_data_file, "a") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); }  // append
  file.close();
  try { test_expect( file.open(test_data_file, "rb") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // read-binary
  file.close();
  try { test_expect( file.open(test_data_file, "rwb") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // read-write-binary
  file.close();
  try { test_expect( file.open(test_data_file, "r+") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // ANSI like read-write
  file.close();
  try { test_expect( file.open(test_data_file, "w+") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // ANSI like read-write
  file.close();
  try { test_expect( file.open(test_data_file, "a+") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // ANSI like read-write
  file.close();
  try { test_expect( file.open(test_data_file, "rwn") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // read-write-nonblocking
  file.close();
  try { test_expect( file.open(test_data_file, "rws") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // read-write-synchroneous
  file.close();
  try { test_expect( file.open(test_data_file, "wp") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // preserve contents on write
  file.close();
  try { test_expect( file.open(test_data_file, "we") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // write no-create/existing only
  file.close();
  try { test_expect( file.open(test_data_file, "wc") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // write create
  file.close();
  try { test_expect( file.open(test_data_file, "wt") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // write textmode
  file.close();
  try { test_expect( file.open(test_data_file, "-w,-t;w t") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // ignored separators, flag duplication allowed
  file.close();
  try { file.open(test_data_file, "y"); test_fail("Unexpected fs.file.open(.., 'y') should have thrown (invalid flag"); } catch(ex) { test_pass("Invalid fs.file.open flag threw: " + ex.message); }
  file.close();
  try { file.open(test_data_file, "wcx"); test_fail("Unexpected fs.file.open(.., 'wx') should have thrown (exclusive create)"); } catch(ex) { test_pass("Invalid fs.file.open flag threw: " + ex.message); }
  file.close();
  try { file.open(test_data_file, "w6666"); test_fail("Unexpected fs.file.open(.., '<invalid-mode>') should have thrown (exclusive create)"); } catch(ex) { test_pass("Invalid fs.file.open mode threw: " + ex.message); }
  file.close();

  if(is_linux) {
    fs.unlink(test_data_file+".tmp");
    try { test_expect( file.open(test_data_file+".tmp", "wcx0600") ) } catch(ex) { test_fail("Unexpected fs.file.open exception:" + ex.message); } // exclusive create, file mode 600
    file.close();
    const file_stat = fs.stat(test_data_file+".tmp");
    fs.unlink(test_data_file+".tmp");
    test_note("File mode of created file: " + file_stat.mode);
    test_expect( file_stat.mode == "600");
  }
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

// binary
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: I/O size');
  file = fs.file(test_data_file, "rwb");
  var wdata = (("#".repeat(15)) + "\n").repeat(65536);
  const n_wr1 = file.write(wdata);
  test_note("file.write(wdata) == " + n_wr1);
  test_expect(n_wr1 == wdata.length);
  wdata = null;
  file.close();
  file.open(test_data_file, "rb");
  var rdata = file.read();
  test_note("file.read().length == " + rdata.length);
  test_expect(rdata.length == n_wr1);
  file.close();
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

// non-blocking readln is not supported
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: readln nonblocking');
  file = fs.file(test_data_file, "w");
  var wdata = (("#".repeat(15)) + "\n").repeat(10);
  const n_wr1 = file.write(wdata);
  test_note("file.write(wdata) == " + n_wr1);
  test_expect(n_wr1 == wdata.length);
  wdata = null;
  file.close();
  file.open(test_data_file, "rn");
  var line = file.readln();
  file.close();
  test_fail("file.readln() should have thrown for non-blocking i/o");
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

// readln
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: readln');
  file = fs.file(test_data_file, "w");
  const num_lines = 15;
  const line_data = "#".repeat(15);
  const wdata = (line_data+"\n").repeat(num_lines);
  const n_wr1 = file.write(wdata);
  test_note("file.write(wdata) == " + n_wr1);
  test_expect(n_wr1 == wdata.length);
  file.close();
  test_note("file size is == " + fs.size(test_data_file));
  file.open(test_data_file, "r");
  var lines = "";
  for(var n=0; n<=num_lines; ++n) {
    const line = file.readln();
    if(line === undefined) break;
    lines += line + "\n";
  }
  file.close();
  test_note( "lines.length==" + lines.length, "num_lines="+num_lines );
  test_expect( lines.replace(/[\s]+$/,"") == wdata.replace(/[\s]+$/,"") ); // win32 triggers EOF in the next loop, so the manual `lines` composition of the test adds one newline too much.
} catch(ex) {
  test_fail("Unexpected exception: " + ex.message);
}

// non-blocking writeln is not supported
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: writeln nonblocking');
  file = fs.file(test_data_file, "wn");
  file.writeln("data");
  file.close();
  test_fail("file.writeln() should have thrown for non-blocking i/o");
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

// non-blocking printf is not supported
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: printf nonblocking');
  file = fs.file(test_data_file, "wn");
  file.prinf("%d", 10);
  test_fail("file.printf() should have thrown for non-blocking i/o");
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

// seek invalid whence
try {
  if(file) file.close();
  test_note('--------------------------------------------------------');
  test_note('Checks for: seek invalid');
  file = fs.file(test_data_file, "r");
  file.seek(1,"unknown-whence");
  test_fail("file.seek() should have thrown for invalid whence");
} catch(ex) {
  test_pass("Expected exception: " + ex.message)
}

if(file) file.close();
fs.unlink(test_data_file);
