// <editor-fold desc="preprocessor" defaultstate="collapsed">
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.fs.hh>
#include <mod/mod.fs.ext.hh>
using namespace std;
// </editor-fold>

// <editor-fold desc="auxiliary fs test functions" defaultstate="collapsed">
void test_rmfiletree()
{
  if(::chdir(test_path().c_str()) == 0) {
    char s[PATH_MAX+2];
    s[PATH_MAX+1] = '\0';
    if((::getcwd(s, PATH_MAX) == 0) && (::strncmp(s, test_path().c_str(), PATH_MAX) == 0)) {
      sysexec(string("rm -rf *"));
    }
  }
}

void test_mkfiletree()
{
  test_comment("(Re)building test temporary file tree '" << test_path() << "'");
  #ifndef WINDOWS
    test_expect(::chdir("/tmp") == 0);
    sysexec(string("rm -rf ") + test_path() + "\\*");
    sysexec(string("mkdir -p ") + test_path());
    if(!exists(test_path()) || (::chdir(test_path().c_str()) != 0)) {
      test_fail("Test base temp directory does not exist or failed to chdir into it.");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    sysexec(string("mkdir -p ") + test_path("a"));
    sysexec(string("mkdir -p ") + test_path("b"));
    test_mkfile("null");
    test_mkfile("undefined");
    test_mkfile("z");
    test_mkfile("a/y");
    test_mkfile("b/x");
    sysexec(string("ln -s '") + test_path("a/y") + "' '" + test_path("ly") + "'");
    sysexec(string("ln -s '") + test_path("a") + "' '" + test_path("b/la") + "'");
  #else
    if(!test_expect_cond(::chdir(test_path().c_str()) == 0)) {
      throw std::runtime_error("Aborted due to failed test assertion: Test directory not existing.");
    }
    test_rmfiletree();
    if(!exists(test_path()) || (::chdir(test_path().c_str()) != 0)) {
      test_fail("Test base temp directory does not exist or failed to chdir into it.");
      throw std::runtime_error("Aborted due to failed test assertion.");
    }
    ::mkdir("a");
    ::mkdir("b");
    test_mkfile("null");
    test_mkfile("undefined");
    test_mkfile("z");
    test_mkfile("a\\y");
    test_mkfile("b\\x");
  #endif
}

// </editor-fold>

// <editor-fold desc="test_informational_functions" defaultstate="collapsed">
void test_informational_functions(duktape::engine& js)
{
  test_comment("test_informational_functions");
  test_mkfiletree();
  test_comment( "fs.cwd() = " << js.eval<string>("fs.cwd()") );
  test_comment( "fs.home() = " << js.eval<string>("fs.home()") );
  test_expect( js.eval<bool>("typeof fs.cwd() == 'string'") );
  test_expect( js.eval<bool>("typeof fs.home() == 'string'") );

  test_expect( js.eval<bool>("typeof fs.exists() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.isreadable() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.iswritable() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.isexecutable() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.isdir() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.isfile() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.islink() == 'boolean'") );
  test_expect( js.eval<bool>("typeof fs.isfifo() == 'boolean'") );

  test_expect( js.eval<bool>("fs.exists(fs.tmpdir()) === true") );
  test_expect( js.eval<bool>("fs.exists('.NOTEXISTING_') === false") );
  test_expect( js.eval<bool>("fs.isreadable(fs.tmpdir()) === true") );
  test_expect( js.eval<bool>("fs.iswritable(fs.tmpdir()) === true") );
  test_expect( js.eval<bool>("fs.isfile(fs.tmpdir()) === false") );
  test_expect( js.eval<bool>("fs.islink(fs.tmpdir()) === false") );
  test_expect( js.eval<bool>("fs.isfifo(fs.tmpdir()) === false") );

  test_expect( js.eval<bool>("fs.chdir(fs.tmpdir()) === true") );
  test_expect( js.eval<bool>("fs.cwd() === fs.tmpdir()") );
  test_expect( js.eval<bool>("fs.realpath('.') === fs.tmpdir()") );
}
// </editor-fold>

// <editor-fold desc="test_mkdir" defaultstate="collapsed">
void test_mkdir(duktape::engine& js)
{
  test_comment("test_mkdir");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.mkdir() === false; // no arg" ) );
  test_expect( js.eval<bool>("fs.mkdir(111) === false; // not string") );
  test_expect( js.eval<bool>("fs.mkdir('test-dir') === true") );
  test_expect( js.eval<bool>("fs.mkdir('test-dir') === true; // already existing") );
  test_expect( js.eval<bool>("fs.rmdir('test-dir') === true;") );
  test_expect( js.eval<bool>("fs.rmdir('test-dir') === false; // not existent") );

  // @todo: that should be nicer with an auxiliary function like [unixwin_path/win_path] unify_path(unix_path)
  #ifndef WINDOWS
  test_expect( js.eval<bool>("fs.mkdir('test-dir/1/2/3','p') === true;") );
  test_expect( js.eval<bool>("fs.isdir('test-dir/1/2/3') === true; // check") );
  test_expect( js.eval<bool>("fs.chdir('test-dir/1/2') === true") );
  test_expect( js.eval<bool>("fs.mkdir('../../4') === true") );
  test_expect( js.eval<bool>("fs.mkdir('./../../5/6/7', 'p') === true") );
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.isdir('test-dir/4') === true; // check") );
  test_expect( js.eval<bool>("fs.isdir('test-dir/5/6/7') === true; // check") );
  #else
  test_expect( js.eval<bool>("fs.mkdir('test-dir\\\\1\\\\2\\\\3','p') === true;") );
  test_expect( js.eval<bool>("fs.isdir('test-dir\\\\1\\\\2\\\\3') === true; // check") );
  test_expect( js.eval<bool>("fs.chdir('test-dir\\\\1\\\\2') === true") );
  test_expect( js.eval<bool>("fs.mkdir('..\\\\..\\\\4') === true") );
  test_expect( js.eval<bool>("fs.mkdir('.\\\\..\\\\..\\\\5\\\\6\\\\7', 'p') === true") );
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.isdir('test-dir\\\\4') === true; // check") );
  test_expect( js.eval<bool>("fs.isdir('test-dir\\\\5\\\\6\\\\7') === true; // check") );
  #endif
}
// </editor-fold>

// <editor-fold desc="test_stat_functions" defaultstate="collapsed">
// stat and functions returning partial ::stat information
void test_stat_functions(duktape::engine& js)
{
  test_comment("test_stat_functions");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir) === 'object';") );
  test_expect( js.eval<bool>("fs.stat() === undefined; // not string") );
  test_expect( js.eval<bool>("fs.stat(undefined) === undefined; // not string") );
  test_expect( js.eval<bool>("fs.stat(123) === undefined; // not string") );
  test_comment( string("fs.stat(testdir): ") + js.eval<string>("JSON.stringify(fs.stat(testdir))") );
  test_expect( js.eval<bool>("fs.stat(testdir).path === testdir") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).size === 'number'") );
  test_expect( js.eval<bool>("fs.stat(testdir).mtime instanceof Date") );
  test_expect( js.eval<bool>("fs.stat(testdir).ctime instanceof Date") );
  test_expect( js.eval<bool>("fs.stat(testdir).atime instanceof Date") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).uid === 'number'") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).gid === 'number'") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).inode === 'number'") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).device === 'number'") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).mode === 'string'") );
  test_expect( js.eval<bool>("typeof fs.stat(testdir).modeval === 'number'") );
  test_comment( "fs.mtime(testdir) = " << js.eval<string>("fs.mtime(testdir)") );
  test_comment( "fs.atime(testdir) = " << js.eval<string>("fs.atime(testdir)") );
  test_comment( "fs.ctime(testdir) = " << js.eval<string>("fs.ctime(testdir)") );
  test_comment( "fs.stat(testdir).mtime.valueOf() = " << js.eval<int64_t>("fs.stat(testdir).mtime.valueOf()") );
  test_comment( "fs.stat(testdir).atime.valueOf() = " << js.eval<int64_t>("fs.stat(testdir).atime.valueOf()") );
  test_comment( "fs.stat(testdir).ctime.valueOf() = " << js.eval<int64_t>("fs.stat(testdir).ctime.valueOf()") );
  test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).mtime.valueOf() - fs.mtime(testdir).valueOf()) < 1000") );
  test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).atime.valueOf() - fs.atime(testdir).valueOf()) < 1000") );
  test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).ctime.valueOf() - fs.ctime(testdir).valueOf()) < 1000") );
  test_expect( js.eval<bool>("fs.stat(testdir).owner === fs.owner(testdir)") );
  test_expect( js.eval<bool>("fs.stat(testdir).group === fs.group(testdir)") );
  test_expect( js.eval<bool>("fs.stat(testdir).size === fs.size(testdir)") );
}
// </editor-fold>

// <editor-fold desc="test_filemod_functions" defaultstate="collapsed">
// file mode <-> string conversion
void test_filemod_functions(duktape::engine& js)
{
  test_comment("test_filemod_functions");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.mod2str() === undefined") );
  test_expect( js.eval<bool>("fs.mod2str('127') === undefined") );
  test_expect( js.eval<bool>(string("fs.mod2str(") + to_string(0755) + ", 'l') === 'rwxr-xr-x'") );
  test_expect( js.eval<bool>(string("fs.mod2str(") + to_string(0700) + ", 'l') === 'rwx------'") );
  #ifndef WINDOWS
  test_expect( js.eval<bool>("fs.mod2str(fs.stat('/dev/null').modeval, 'e')[0] === 'c'") );
    test_expect( js.eval<bool>("fs.mod2str(fs.stat('/dev/sda').modeval, 'e')[0] === 'b'") );
    // @sw: note test for pipe, link, sock
  #endif
  test_expect( js.eval<bool>("fs.str2mod() === undefined") );
  test_expect( js.eval<bool>("fs.str2mod(799) === undefined") );
  test_expect( js.eval<bool>("fs.str2mod(788) === undefined") );
  test_expect( js.eval<bool>("fs.str2mod('---------   ') === undefined") );
  test_expect( js.eval<bool>(string("fs.str2mod('rwxrwx---') === ") + to_string(0770)) );
  test_expect( js.eval<bool>(string("fs.str2mod('drwxrwx---') === ") + to_string(0770)) );
  test_expect( js.eval<bool>(string("fs.str2mod('755') === ") + to_string(0755)) );
  test_expect( js.eval<bool>(string("fs.str2mod(755) === ") + to_string(0755)) );
  test_expect( js.eval<bool>(string("fs.str2mod('0644') === ") + to_string(0644)) );
}
// </editor-fold>

// <editor-fold desc="test_readfile_writefile" defaultstate="collapsed">
// readfile / writefile
void test_readfile_writefile(duktape::engine& js)
{
  test_comment("test_readfile_writefile");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.writefile('testfile', 'testfiletext') === true") );
  test_expect( js.eval<bool>("fs.readfile('testfile') === 'testfiletext'") );
  test_expect( js.eval<bool>("fs.unlink('testfile') === true") );
  test_expect( js.eval<bool>("fs.writefile('testfile', Duktape.dec('hex', '4142434445')) === true") );
  test_expect( js.eval<bool>("fs.readfile('testfile') === 'ABCDE'") );
  test_note  ( js.eval<string>("fs.readfile('testfile', 'binary')") );
  test_note  ( "fs.readfile('testfile', 'binary').byteLength = " << js.eval<string>("fs.readfile('testfile', 'binary').byteLength") );
  test_note  ( "(new DataView(fs.readfile('testfile', 'binary'))).getUint8(0) = " << js.eval<string>("(new DataView(fs.readfile('testfile', 'binary'))).getUint8(0)") );
  test_expect( js.eval<bool>("typeof(fs.readfile('testfile', 'binary')) === 'object'") );
  test_expect( js.eval<bool>("fs.readfile('testfile', 'binary').byteLength === 5") );
  test_expect( js.eval<bool>("(new DataView(fs.readfile('testfile', 'binary'))).getUint8(0) === 65") );
  test_expect( js.eval<bool>("typeof(fs.readfile('testfile')) !== 'object'") );
  test_expect( js.eval<bool>("typeof(fs.readfile('testfile', 'text')) !== 'object'") );
  test_expect( js.eval<bool>("typeof(fs.readfile('testfile', {binary:false})) !== 'object'") );
  test_expect( js.eval<bool>("typeof(fs.readfile('testfile', {binary:true})) === 'object'") );
  test_expect( js.eval<bool>("fs.unlink('testfile') === true") );

  #define LINETEST "'a\\nb\\nc\\nd\\nE\\nF\\n1.5\\nLAST_NONL'"
  test_expect( js.eval<bool>("fs.writefile('testfile', " LINETEST ") === true") );
  test_expect( js.eval<bool>("fs.readfile('testfile') === " LINETEST) );
  test_expect( js.eval<bool>("fs.readfile('testfile', function(line){return true;}) === " LINETEST) );
  test_expect( js.eval<bool>("fs.readfile('testfile', function(line){return;}) === ''") );
  test_expect( js.eval<bool>("fs.readfile('testfile', function(line){return false;}) === ''") );
  test_expect( js.eval<string>("fs.readfile('testfile', function(line){return line == 'a';})") == "a\n" );
  test_expect( js.eval<bool>("fs.readfile('testfile', function(line){return line.toUpperCase();}) === " LINETEST ".toUpperCase();") );
  test_expect( js.eval<bool>("fs.unlink('testfile') === true") );
  #undef LINETEST
}
// </editor-fold>

// <editor-fold desc="test_chmod_functions" defaultstate="collapsed">
void test_chmod_functions(duktape::engine& js)
{
  test_comment("test_chmod_functions");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.writefile('testfile', 'test') === true") );
  if(js.eval<bool>("fs.isfile('testfile')")) {
    test_expect( js.eval<bool>("fs.chmod('testfile', '755') === true") );
    test_comment( js.eval<string>("fs.stat('testfile').mode") );
    test_expect( js.eval<bool>("fs.isreadable('testfile') === true") );
    test_expect( js.eval<bool>("fs.isexecutable('testfile') === true") );
    test_expect( js.eval<bool>("fs.iswritable('testfile') === true") );
    test_expect( js.eval<bool>("fs.chmod('testfile', '600') === true") );
    test_comment( js.eval<string>("fs.stat('testfile').mode") );
    test_expect( js.eval<bool>("fs.isreadable('testfile') === true") );
    test_expect( js.eval<bool>("fs.isexecutable('testfile') === false") );
    test_expect( js.eval<bool>("fs.iswritable('testfile') === true") );
    test_expect( (js.eval<bool>("fs.stat('testfile').mode === '600'")) );
    test_expect( js.eval<bool>("fs.chmod('testfile', '644') === true") );
    test_comment( js.eval<string>("fs.stat('testfile').mode") );
    test_expect( (js.eval<bool>("fs.stat('testfile').mode === '644'")));
  }
  test_expect( js.eval<bool>("fs.unlink('testfile')") );
}
// </editor-fold>

// <editor-fold desc="test_readdir_function" defaultstate="collapsed">
void test_readdir_function(duktape::engine& js)
{
  test_comment("test_readdir_function");
  test_mkfiletree();
  #ifndef WINDOWS
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.mkdir('readdirtest') === true") );
  test_expect( js.eval<bool>("fs.chdir('readdirtest') === true") );
  test_expect( js.eval<bool>("fs.mkdir('1/2/3/4/5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('2/2/3/4/5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('3/2/3/4/5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('4/2/3/4/5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('5/2/3/4/5', 'p') === true") );
  test_comment( "fs.readdir('.') = " << js.eval<string>("fs.readdir('.').sort().join(',')") );
  test_expect( js.eval<string>("fs.readdir('.').sort().join(',')") == "1,2,3,4,5" );
  test_expect( js.eval<string>("fs.readdir().sort().join(',')") == "1,2,3,4,5" );
  test_expect( js.eval<string>("fs.readdir(testdir+'/readdirtest').sort().join(',')") == "1,2,3,4,5" );
  #else
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.mkdir('readdirtest') === true") );
  test_expect( js.eval<bool>("fs.chdir('readdirtest') === true") );
  test_expect( js.eval<bool>("fs.mkdir('1\\\\2\\\\3\\\\4\\\\5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('2\\\\2\\\\3\\\\4\\\\5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('3\\\\2\\\\3\\\\4\\\\5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('4\\\\2\\\\3\\\\4\\\\5', 'p') === true") );
  test_expect( js.eval<bool>("fs.mkdir('5\\\\2\\\\3\\\\4\\\\5', 'p') === true") );
  test_comment( "fs.readdir('.') = " << js.eval<string>("fs.readdir('.').sort().join(',')") );
  test_expect( js.eval<string>("fs.readdir('.').sort().join(',')") == "1,2,3,4,5" );
  test_expect( js.eval<string>("fs.readdir().sort().join(',')") == "1,2,3,4,5" );
  test_expect( js.eval<string>("fs.readdir(testdir+'\\\\readdirtest').sort().join(',')") == "1,2,3,4,5" );
  #endif
  test_comment("test_glob_function");
  test_comment( "fs.glob('*') = " << js.eval<string>("JSON.stringify(fs.glob('*'))") );
  test_expect( js.eval<string>("fs.glob('*').join(',')") == "1,2,3,4,5" );
}
// </editor-fold>

// <editor-fold desc="test_unsafe_functions" defaultstate="collapsed">
void test_unsafe_functions(duktape::engine& js)
{
  test_comment("test_unsafe_functions");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_comment( "fs.tempnam('blaaa') = " << js.eval<string>("fs.tempnam('blaaa')") );
  test_expect( js.eval<bool>("fs.tempnam('blaaa') !== undefined") );
}
// </editor-fold>

// <editor-fold desc="test_copy_function" defaultstate="collapsed">
void test_copy_function(duktape::engine& js)
{
  test_comment("test_copy_function");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  // file copy: file only
  test_expect( js.eval<bool>("fs.copy(testdir+'/z', testdir+'/CP')") && exists(test_path("CP")) );
  test_expect( js.eval<bool>("fs.copy('z','CP2')") && exists(test_path("CP2")) );

  // file copy: not recursive
  test_expect( !js.eval<bool>("fs.copy(testdir+'/a', testdir+'/DIR')") && !exists(test_path("DIR")) );
  test_expect( !js.eval<bool>("fs.copy('a','DIR')") && !exists(test_path("DIR")) );

  // file copy: copy into directory
  test_expect( js.eval<bool>("fs.copy('CP','a')") && exists(test_path("a/CP")) );

  // file copy: recursive copy
  test_expect( !js.eval<bool>("fs.copy('a','b')") && !exists(test_path("a/b")) );
  test_expect( js.eval<bool>("fs.copy('a','b/',{recursive:true})") && exists(test_path("b/a")) );

  // file copy: recursive, explicit new target file name
  test_expect( js.eval<bool>("fs.copy('a','b/a/d',{recursive:true})") && exists(test_path("b/a/d")) );
  test_expect( js.eval<bool>("fs.copy(testdir+'/b',testdir+'/a/','r')") && exists(test_path("a/b")) );

  // option masking
  test_expect( js.eval<bool>("fs.copy('z','-K')") && exists(test_path("-K")) );
}
// </editor-fold>

// <editor-fold desc="test_move_function" defaultstate="collapsed">
void test_move_function(duktape::engine& js)
{
  test_comment("test_move_function");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( js.eval<bool>("fs.move(testdir+'/z', testdir+'/ZZ') // rename file") && exists(test_path("ZZ")) && !exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move('ZZ', 'z') // rename file") && !exists(test_path("ZZ")) && exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move(testdir+'/a/y', testdir+'/b') // move file to dir") && exists(test_path("b/y")) && !exists(test_path("a/y")) );
  test_expect( js.eval<bool>("fs.move('b/y', '.') // move file to dir") && exists(test_path("y")) && !exists(test_path("b/y")) );
  test_expect( js.eval<bool>("fs.move('b', 'a') // move dir recursively to dir") && exists(test_path("a/b")) && !exists(test_path("b")) );
  test_expect( js.eval<bool>("fs.isfile('z') ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move() ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move(undefined, undefined) ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move(null, undefined) ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('z') ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('', '') ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('', 'z') ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('z', '') ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('z', null) ") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move('*', 'a') // no shell escaping") && exists(test_path("z")) );
  test_expect( !js.eval<bool>("fs.move(testdir+'/*', 'a') // no shell escaping") && exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.move('z', \"'z'\") // no shell escaping") && exists(test_path("'z'")) );
}
// </editor-fold>

// <editor-fold desc="test_remove_function" defaultstate="collapsed">
void test_remove_function(duktape::engine& js)
{
  test_comment("test_remove_function");
  test_mkfiletree();
  test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
  test_expect( !js.eval<bool>("fs.remove()") );
  test_expect( !js.eval<bool>("fs.remove('')") );
  test_expect( !js.eval<bool>("fs.remove(undefined)") && exists(test_path("undefined")) );
  test_expect( !js.eval<bool>("fs.remove(null)") && exists(test_path("null")) );
  test_expect( js.eval<bool>("fs.remove(testdir+'/z')") && !exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.remove(testdir+'/z') // remove not existing file is ok") && !exists(test_path("z")) );
  test_expect( js.eval<bool>("fs.remove('a/y')") && !exists(test_path("a/y")) );
  test_expect( js.eval<bool>("fs.remove('a/y')") && !exists(test_path("a/y")) );
  test_expect( !js.eval<bool>("fs.remove(testdir+'/a')") && exists(test_path("a")) );
  test_expect( !js.eval<bool>("fs.remove('a')") && exists(test_path("a")) );
  test_expect( js.eval<bool>("fs.remove('a', {recursive:true})") && !exists(test_path("a")) );
  test_expect( js.eval<bool>("fs.remove('a')") && !exists(test_path("a")) );
  test_expect( js.eval<bool>("fs.remove(testdir+'/b', '-r')") && !exists(test_path("b")) );
}
// </editor-fold>

// <editor-fold desc="test main" defaultstate="collapsed">
void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);
  duktape::mod::filesystem::generic::define_in<>(js);
  duktape::mod::filesystem::basic::define_in<>(js);
  duktape::mod::filesystem::extended::define_in<>(js);
  js.define("testdir", test_path());
  test_mkfiletree();

  try {
    test_informational_functions(js);
    test_mkdir(js);
    test_stat_functions(js);
    test_readfile_writefile(js);
    test_readdir_function(js);
#ifndef WINDOWS
    test_filemod_functions(js);

    test_move_function(js);
    test_remove_function(js);
#endif
    test_rmfiletree();
  } catch(...) {
    test_rmfiletree();
    throw;
  }
}
// </editor-fold>
