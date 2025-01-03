
#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <mod/mod.sys.hh>
#include <mod/mod.fs.hh>
using namespace std;
using namespace testenv;


namespace {

  void test_informational_functions(duktape::engine& js)
  {
    test_info("test_informational_functions");
    test_makefiletree();
    test_info( "fs.cwd() = ", js.eval<string>("fs.cwd()") );
    test_info( "fs.home() = ", js.eval<string>("fs.home()") );
    test_expect( js.eval<bool>("typeof fs.cwd() == 'string'") );
    test_expect( js.eval<bool>("typeof fs.home() == 'string'") );
    test_expect( js.eval<bool>("typeof fs.application() == 'string'") );
    test_expect( js.eval<bool>("fs.isfile(fs.application())") );
    test_expect( js.eval<bool>("typeof fs.dirname(fs.application()) === 'string'") );
    test_expect( js.eval<bool>("typeof fs.basename(fs.application()) === 'string'") );
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
    test_expect( js.eval<bool>("fs.realpath('./') === fs.tmpdir()") );
    test_expect( js.eval<bool>("fs.realpath('~') === fs.home()") );
    test_expect( js.eval<bool>("fs.realpath('~/') === fs.home()") );
  }

  void test_mkdir(duktape::engine& js)
  {
    test_info("test_mkdir");
    test_makefiletree();
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("fs.mkdir() === false; // no arg" ) );
    test_expect( js.eval<bool>("fs.mkdir(111) === false; // not string") );
    test_expect( js.eval<bool>("fs.mkdir('test-dir') === true") );
    test_expect( js.eval<bool>("fs.mkdir('test-dir') === true; // already existing") );
    test_expect( js.eval<bool>("fs.rmdir('test-dir') === true;") );
    test_expect( js.eval<bool>("fs.rmdir('test-dir') === false; // not existent") );

    test_expect( js.eval<bool>("fs.mkdir('test-dir/1/2/3','p') === true;") );
    test_expect( js.eval<bool>("fs.isdir('test-dir/1/2/3') === true; // check") );
    test_expect( js.eval<bool>("fs.chdir('test-dir/1/2') === true") );
    test_expect( js.eval<bool>("fs.mkdir('../../4') === true") );
    test_expect( js.eval<bool>("fs.mkdir('./../../5/6/7', 'p') === true") );
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("fs.isdir('test-dir/4') === true; // check") );
    test_expect( js.eval<bool>("fs.isdir('test-dir/5/6/7') === true; // check") );
    #ifdef OS_WINDOWS
    test_expect( js.eval<bool>("fs.mkdir('test-dir\\\\11\\\\12\\\\13','p') === true;") );
    test_expect( js.eval<bool>("fs.isdir('test-dir\\\\11\\\\12\\\\13') === true; // check") );
    test_expect( js.eval<bool>("fs.chdir('test-dir\\\\11\\\\12') === true") );
    test_expect( js.eval<bool>("fs.mkdir('..\\\\..\\\\14') === true") );
    test_expect( js.eval<bool>("fs.mkdir('.\\\\..\\\\..\\\\15\\\\16\\\\17', 'p') === true") );
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("fs.isdir('test-dir\\\\14') === true; // check") );
    test_expect( js.eval<bool>("fs.isdir('test-dir\\\\15\\\\16\\\\17') === true; // check") );
    #endif
  }

  void test_stat_functions(duktape::engine& js)
  {
    // stat and functions returning partial ::stat information
    test_info("test_stat_functions");
    test_makefiletree();
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("typeof fs.stat(testdir) === 'object';") );
    test_expect( js.eval<bool>("fs.stat() === undefined; // not string") );
    test_expect( js.eval<bool>("fs.stat(undefined) === undefined; // not string") );
    test_expect( js.eval<bool>("fs.stat(123) === undefined; // not string") );
    test_info( string("fs.stat(testdir): ") + js.eval<string>("JSON.stringify(fs.stat(testdir))") );
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
    test_info( "fs.mtime(testdir) = ", js.eval<string>("fs.mtime(testdir)") );
    test_info( "fs.atime(testdir) = ", js.eval<string>("fs.atime(testdir)") );
    test_info( "fs.ctime(testdir) = ", js.eval<string>("fs.ctime(testdir)") );
    test_info( "fs.stat(testdir).mtime.valueOf() = ", js.eval<int64_t>("fs.stat(testdir).mtime.valueOf()") );
    test_info( "fs.stat(testdir).atime.valueOf() = ", js.eval<int64_t>("fs.stat(testdir).atime.valueOf()") );
    test_info( "fs.stat(testdir).ctime.valueOf() = ", js.eval<int64_t>("fs.stat(testdir).ctime.valueOf()") );
    test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).mtime.valueOf() - fs.mtime(testdir).valueOf()) < 1000") );
    test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).atime.valueOf() - fs.atime(testdir).valueOf()) < 1000") );
    test_expect( js.eval<bool>("Math.abs(fs.stat(testdir).ctime.valueOf() - fs.ctime(testdir).valueOf()) < 1000") );
    test_expect( js.eval<bool>("fs.stat(testdir).owner === fs.owner(testdir)") );
    test_expect( js.eval<bool>("fs.stat(testdir).group === fs.group(testdir)") );
    test_expect( js.eval<bool>("fs.stat(testdir).size === fs.size(testdir)") );
  }

  void test_filemod_functions(duktape::engine& js)
  {
    test_info("test_filemod_functions");
    #ifndef OS_WINDOWS
      // file mode <-> string conversion
      test_makefiletree();
      test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
      test_expect( js.eval<bool>("fs.mod2str() === undefined") );
      test_expect( js.eval<bool>("fs.mod2str('127') === undefined") );
      test_expect( js.eval<bool>(string("fs.mod2str(") + to_string(0755) + ", 'l') === 'rwxr-xr-x'") );
      test_expect( js.eval<bool>(string("fs.mod2str(") + to_string(0700) + ", 'l') === 'rwx------'") );
      test_expect( js.eval<bool>("fs.mod2str(fs.stat('/dev/null').modeval, 'e')[0] === 'c'") );
      if(::access("/dev/sda", F_OK)==0) test_expect( js.eval<bool>("fs.mod2str(fs.stat('/dev/sda').modeval, 'e')[0] === 'b'") );
      test_expect( js.eval<bool>("fs.str2mod() === undefined") );
      test_expect( js.eval<bool>("fs.str2mod('') === undefined") );
      test_expect( js.eval<bool>("fs.str2mod(799) === undefined") );
      test_expect( js.eval<bool>("fs.str2mod(788) === undefined") );
      test_expect( js.eval<bool>("fs.str2mod('---------   ') === undefined") );
      test_expect( js.eval<bool>(string("fs.str2mod('rwxrwx---') === ") + to_string(0770)) );
      test_expect( js.eval<bool>(string("fs.str2mod('drwxrwx---') === ") + to_string(0770)) );
      test_expect( js.eval<bool>(string("fs.str2mod('755') === ") + to_string(0755)) );
      test_expect( js.eval<bool>(string("fs.str2mod(755) === ") + to_string(0755)) );
      test_expect( js.eval<bool>(string("fs.str2mod('0644') === ") + to_string(0644)) );
    #else
      (void)js;
    #endif
  }

  void test_readfile_writefile(duktape::engine& js)
  {
    test_info("test_readfile_writefile");
    test_makefiletree();
    {
      test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
      test_expect( js.eval<bool>("fs.writefile('testfile', 'testfiletext') === true") );
      test_expect( js.eval<bool>("fs.readfile('testfile') === 'testfiletext'") );
      test_expect( js.eval<bool>("fs.readfile('testfile', {filter:function(line){return line;}}) === 'testfiletext'") );
      test_expect( js.eval<bool>("fs.readfile('testfile', {filter:function(line){return '';}}) === ''") );
      test_expect_except( js.eval<bool>("fs.readfile('testfile', {filter:'no-function'})") ); // filter not a function
      test_expect_except( js.eval<bool>("fs.readfile('testfile', {filter:1000})") ); // filter not a function
      test_expect_except( js.eval<bool>("fs.readfile('testfile', 1000)") ); // invalid config
      test_expect_except( js.eval<bool>("fs.readfile('testfile', {binary:true, filter:function(){}})") ); // filter only for text, not binary.
      test_expect_except( js.eval<bool>("fs.readfile('testfile', function(){return 10}})") ); // filter return type invalid
      test_expect( js.eval<bool>("fs.unlink('testfile') === true") );
    }
    {
      test_expect_except( js.eval<bool>("fs.writefile('testfile')") ); // no data argument
      test_expect_except( js.eval<bool>("fs.writefile('testfile', function(){})") ); // data cannot be a function (no data generators allowed).

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
    }
    {
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

  }

  void test_read_write(duktape::engine& js)
  {
    test_info("test_read_write");
    test_makefiletree();
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("fs.write('testfile', 'testfiletext') === true") );
    test_expect( js.eval<bool>("fs.read('testfile') === 'testfiletext'") );
    test_expect( js.eval<bool>("fs.unlink('testfile') === true") );
    test_expect( js.eval<bool>("fs.write('testfile', Duktape.dec('hex', '4142434445')) === true") );
    test_expect( js.eval<bool>("fs.read('testfile') === 'ABCDE'") );
    test_note  ( js.eval<string>("fs.read('testfile', 'binary')") );
    test_note  ( "fs.read('testfile', 'binary').byteLength = " << js.eval<string>("fs.read('testfile', 'binary').byteLength") );
    test_note  ( "(new DataView(fs.read('testfile', 'binary'))).getUint8(0) = " << js.eval<string>("(new DataView(fs.read('testfile', 'binary'))).getUint8(0)") );
    test_expect( js.eval<bool>("typeof(fs.read('testfile', 'binary')) === 'object'") );
    test_expect( js.eval<bool>("fs.read('testfile', 'binary').byteLength === 5") );
    test_expect( js.eval<bool>("(new DataView(fs.read('testfile', 'binary'))).getUint8(0) === 65") );
    test_expect( js.eval<bool>("typeof(fs.read('testfile')) !== 'object'") );
    test_expect( js.eval<bool>("typeof(fs.read('testfile', 'text')) !== 'object'") );
    test_expect( js.eval<bool>("typeof(fs.read('testfile', {binary:false})) !== 'object'") );
    test_expect( js.eval<bool>("typeof(fs.read('testfile', {binary:true})) === 'object'") );
    test_expect( js.eval<bool>("fs.unlink('testfile') === true") );

    #define LINETEST "'a\\nb\\nc\\nd\\nE\\nF\\n1.5\\nLAST_NONL'"
    test_expect( js.eval<bool>("fs.write('testfile', " LINETEST ") === true") );
    test_expect( js.eval<bool>("fs.read('testfile') === " LINETEST) );
    test_expect( js.eval<bool>("fs.read('testfile', function(line){return true;}) === " LINETEST) );
    test_expect( js.eval<bool>("fs.read('testfile', function(line){return;}) === ''") );
    test_expect( js.eval<bool>("fs.read('testfile', function(line){return false;}) === ''") );
    test_expect( js.eval<string>("fs.read('testfile', function(line){return line == 'a';})") == "a\n" );
    test_expect( js.eval<bool>("fs.read('testfile', function(line){return line.toUpperCase();}) === " LINETEST ".toUpperCase();") );
    test_expect( js.eval<bool>("fs.unlink('testfile') === true") );
    #undef LINETEST
  }

  void test_readdir_function(duktape::engine& js)
  {
    test_info("test_readdir_function");
    test_makefiletree();
    #ifndef OS_WINDOWS
    test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
    test_expect( js.eval<bool>("fs.mkdir('readdirtest') === true") );
    test_expect( js.eval<bool>("fs.chdir('readdirtest') === true") );
    test_expect( js.eval<bool>("fs.mkdir('1/2/3/4/5', 'p') === true") );
    test_expect( js.eval<bool>("fs.mkdir('2/2/3/4/5', 'p') === true") );
    test_expect( js.eval<bool>("fs.mkdir('3/2/3/4/5', 'p') === true") );
    test_expect( js.eval<bool>("fs.mkdir('4/2/3/4/5', 'p') === true") );
    test_expect( js.eval<bool>("fs.mkdir('5/2/3/4/5', 'p') === true") );
    test_info( "fs.readdir('.') = ", js.eval<string>("fs.readdir('.').sort().join(',')") );
    test_expect( js.eval<bool>("fs.readdir({invalid:true}) === undefined") );
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
    test_info( "fs.readdir('.') = ", js.eval<string>("fs.readdir('.').sort().join(',')") );
    test_expect( js.eval<string>("fs.readdir('.').sort().join(',')") == "1,2,3,4,5" );
    test_expect( js.eval<string>("fs.readdir().sort().join(',')") == "1,2,3,4,5" );
    test_expect( js.eval<string>("fs.readdir(testdir+'\\\\readdirtest').sort().join(',')") == "1,2,3,4,5" );
    #endif
    test_info("test_glob_function");
    test_info( "fs.glob('*') = ", js.eval<string>("JSON.stringify(fs.glob('*'))") );
    test_expect( js.eval<string>("fs.glob('*').join(',')") == "1,2,3,4,5" );
  }


  void test_chmod_functions(duktape::engine& js)
  {
    test_info("test_chmod_functions");
    #ifndef OS_WINDOWS
      test_makefiletree();
      test_expect( js.eval<bool>("fs.chdir(testdir) === true") );
      test_expect( js.eval<bool>("fs.writefile('testfile', 'test') === true") );
      if(test_expect_cond( js.eval<bool>("fs.isfile('testfile')")) ) {
        test_expect( js.eval<bool>("fs.chmod('testfile', 'rwxr--r--') === true") );
        test_expect( js.eval<bool>("fs.stat('testfile').mode === '744'"));
        test_expect( js.eval<bool>("fs.chmod('testfile', '755') === true") );
        test_info( js.eval<string>("fs.stat('testfile').mode") );
        test_expect( js.eval<bool>("fs.isreadable('testfile') === true") );
        test_expect( js.eval<bool>("fs.isexecutable('testfile') === true") );
        test_expect( js.eval<bool>("fs.iswritable('testfile') === true") );
        test_expect( js.eval<bool>("fs.chmod('testfile', '600') === true") );
        test_info( js.eval<string>("fs.stat('testfile').mode") );
        test_expect( js.eval<bool>("fs.isreadable('testfile') === true") );
        test_expect( js.eval<bool>("fs.isexecutable('testfile') === false") );
        test_expect( js.eval<bool>("fs.iswritable('testfile') === true") );
        test_expect( (js.eval<bool>("fs.stat('testfile').mode === '600'")) );
        test_expect( js.eval<bool>("fs.chmod('testfile', '644') === true") );
        test_info( js.eval<string>("fs.stat('testfile').mode") );
        test_expect( js.eval<bool>("fs.stat('testfile').mode === '644'"));
        test_expect( js.eval<bool>("fs.symlink('testfile', 'testfile.ln') === true") );
        test_expect( js.eval<bool>("fs.readlink('testfile.ln') === 'testfile'") );
        test_expect( js.eval<bool>("fs.lstat('testfile').mode === '644'") );
        test_expect( js.eval<bool>("fs.lstat('testfile.ln').mode !== undefined") ); // what it is exactly depends on umask.
        test_expect( js.eval<bool>("fs.unlink('testfile.ln') === true") );
        test_expect( js.eval<bool>("fs.utime() === false") ); // no file, no mtime
        test_expect( js.eval<bool>("fs.utime('testfile') === false") ); // no mtime
        test_expect( js.eval<bool>("fs.utime('testfile', new Date(100000000e3), new Date(100000000e3)) === true") ); // mtime and atime
        test_expect( js.eval<bool>("fs.stat('testfile').mtime.valueOf() === 100000000e3") );
        test_expect( js.eval<bool>("fs.stat('testfile').atime.valueOf() === 100000000e3") );
        test_expect( js.eval<bool>("fs.utime('testfile', new Date(200000000e3)) === true") ); // mtime
        test_expect( js.eval<bool>("fs.stat('testfile').mtime.valueOf() === 200000000e3") );
        test_expect( js.eval<bool>("fs.rename() === false") );
        test_expect( js.eval<bool>("fs.rename('testfile') === false") );
        test_expect( js.eval<bool>("fs.rename('testfile','testfile1') === true") );
        test_expect( js.eval<bool>("fs.rename('testfile1','testfile') === true") );
        test_expect( js.eval<bool>("fs.hardlink('testfile') === false") );
        test_expect( js.eval<bool>("fs.hardlink('testfile','testfile1') === true") );
        test_expect( js.eval<bool>("fs.isfile('testfile1') === true") );
        test_expect( js.eval<bool>("fs.read('testfile1') === fs.read('testfile')") );
        test_expect( js.eval<bool>("fs.unlink('testfile1') === true") );
      }
      test_expect( js.eval<bool>("fs.unlink('testfile')") );
    #else
      (void)js;
    #endif
  }

}

void test(duktape::engine& js)
{
  duktape::mod::system::define_in<>(js);
  duktape::mod::filesystem::generic::define_in<>(js);
  duktape::mod::filesystem::basic::define_in<>(js);
  js.define("testdir", test_path());
  test_makefiletree();

  try {
    test_informational_functions(js);
    test_mkdir(js);
    test_stat_functions(js);
    test_readfile_writefile(js);
    test_read_write(js);
    test_readdir_function(js);
    test_filemod_functions(js);
    test_chmod_functions(js);
  } catch(...) {
    test_rmfiletree();
    throw;
  }
}
