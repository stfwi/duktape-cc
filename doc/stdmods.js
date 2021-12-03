/** @file: duktape.hh */

/**
 * Reference to the global object, mainly useful
 * for accessing in strict mode ('use strict';).
 * @var {object}
 */
var global = {};

/** @file: mod.stdlib.hh */

/**
 * Exits the script interpreter with a specified exit code.
 *
 * @param {number} status_code
 */
exit = function(status_code) {};

/**
 * Includes a JS file and returns the result of
 * the last statement.
 * Note that `include()` is NOT recursion protected.
 *
 * @param {string} path
 * @return {any}
 */
include = function(path) {};

/** @file: mod.stdio.hh */

/**
 * print(a,b,c, ...). Writes data stringifyed to STDOUT.
 *
 * @param {...*} args
 */
print = function(args) {};

/**
 * alert(a,b,c, ...). Writes data stringifyed to STDERR.
 *
 * @param {...*} args
 */
alert = function(args) {};

/**
 * Returns a first character entered as string. First function argument
 * is a text that is printed before reading the input stream (without newline,
 * "?" or ":"). The input is read from STDIN.
 *
 * @param {string} text
 * @return {string}
 */
confirm = function(text) {};

/**
 * Returns a line entered via the input stream STDIN.
 *
 * @return {string}
 */
prompt = function() {};

/**
 * C style formatted output to STDOUT. Note that not
 * all formats can be used like in C/C++ because ECMA
 * is has no strong type system. E.g. %u, %ul etc does
 * not make sense because numbers are intrinsically
 * floating point, and the (data type) size of the number
 * matters when using %u. If unsupported arguments are
 * passed the method will throw an exception.
 *
 * Supported formatters are:
 *
 *  - %d, %ld, %lld: The ECMA number is coerced
 *    into an integer and printed. Digits and sign
 *    are supported (%4d, %+06l).
 *
 *  - %f, %lf, %g: Floating point format, also with additional
 *    format specs like %.8f, %+10.5f etc.
 *
 *  - %x: Hexadecimal representation of numbers (also %08x
 *    or %4X etc supported).
 *
 *  - %o: Octal representation of numbers (also %08o)
 *
 *  - %s: String coercing with optional minimum width (e.g. %10s)
 *
 *  - %c: When number given, the ASCII code of the lowest byte
 *        (e.g. 65=='A'). When string given the first character
 *        of the string.
 *
 * Not supported:
 *
 *  - Parameter argument (like "%2$d", the "2$" would apply the
 *    same format to two arguments passed to the function).
 *
 *  - Dynamic width (like "%*f", the "*" would be used to pass
 *    the output width of the floating point number as argument).
 *
 *  - Unsigned %u, %n
 *
 * @param {string} format
 * @param {...*} args
 */
printf = function(format, args) {};

/**
 * C style formatted output into a string. For
 * formatting details please @see printf.
 *
 * @param {string} format
 * @param {...*} args
 * @return {string}
 */
sprintf = function(format, args) {};

/**
 * Console object known from various JS implementations.
 *
 * @var {object}
 */
var console = {};
/**
 * Writes a line to the log stream (automatically appends a newline).
 * The default log stream is STDERR.
 *
 * @param {...*} args
 */
console.log = function(args) {};

/**
 * Read from STDIN until EOF. Using the argument `arg` it is possible
 * to use further functionality:
 *
 *  - By default (if `arg` is undefined or not given) the function
 *    returns a string when all data are read.
 *
 *  - If `arg` is boolean and `true`, then the input is read into
 *    a buffer variable, not a string. The function returns when
 *    all data are read.
 *
 * - If `arg` is a function, then a string is read line by line,
 *   and each line passed to this callback function. If the function
 *   returns `true`, then the line is added to the output. The
 *   function may also preprocess line and return a String. In this
 *   case the returned string is added to the output.
 *   Otherwise, if `arg` is not `true` and no string, the line is
 *   skipped.
 *
 * @param {function|boolean} arg
 * @return {string|buffer}
 */
console.read = function(arg) {};

/**
 * Write to STDOUT without any conversion, whitespaces between the given arguments,
 * and without newline at the end.
 *
 * @param {...*} args
 */
console.write = function(args) {};

/** @file: mod.fs.hh */

/**
 * Global file system object.
 * @var {object}
 */
var fs = {};

/**
 * Reads a file, returns the contents or undefined on error.
 *
 * - Normally the file is read as text file, no matter if the data in the file
 *   really correspond to a readable text.
 *
 * - If `conf` is a string containing the word "binary", the data are read
 *   binary and returned as `buffer`.
 *
 * - If `conf` is a callable function, the data are read as text line by line,
 *   and for each line the callback is invoked, where the line text is passed
 *   as argument. The callback ("filter function") can:
 *
 *    - return `true` to keep the passed line in the file read output.
 *    - return `false` or `undefined` to exclude the line from the output.
 *    - return a `string` to put the returned string into the output instead
 *      of the original passed text (inline editing).
 *
 * - Binary reading and using the filter callback function excludes another.
 *
 * - Not returning anything/undefined has a purpose of storing the line data
 *   somewhere else, e.g. when parsing:
 *
 *     var config = {};
 *     fs.readfile("whatever.conf", function(s) {
 *       // parse, parse ... get some key and value pair ...
 *       config[key] = value;
 *       // no return return statement, output of fs.filter()
 *       // is not relevant.
 *     });
 *
 * @param {string} path
 * @param {string|function} [conf]
 * @return {string|buffer}
 */
fs.read = function(path, conf) {};

/**
 * Writes data into a file.
 *
 * @param {string} path
 * @param {string|buffer|number|boolean|object} data
 * @return {boolean}
 */
fs.write = function(path, data) {};

/**
 * Appends data at the end of a file.
 *
 * @see fs.append
 * @param {string} path
 * @param {string|buffer|number|boolean|object} data
 * @return {boolean}
 */
fs.append = function(path, data) {};

/**
 * Alias of `fs.read()`. Reads a file, returns the contents or undefined
 * on error. See `fs.read()` for full options.
 *
 * @see fs.read
 * @param {string} path
 * @param {string|function} [conf]
 * @return {string|buffer}
 */
fs.readfile = function(path, conf) {};

/**
 * Writes data into a file. Alias of `fs.write()`.
 *
 * @see fs.write
 * @param {string} path
 * @param {string|buffer|number|boolean|object} data
 * @return {boolean}
 */
fs.writefile = function(path, data) {};

/**
 * Appends data at the end of a file. Alias of `fs.append()`.
 *
 * @see fs.append
 * @param {string} path
 * @param {string|buffer|number|boolean|object} data
 * @return {boolean}
 */
fs.appendfile = function(path, data) {};

/**
 * Returns the current working directory or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return {string|undefined}
 */
fs.cwd = function() {};

/**
 * Returns the temporary directory or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return {string|undefined}
 */
fs.tmpdir = function() {};

/**
 * Returns the home directory of the current used or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return {string|undefined}
 */
fs.home = function() {};

/**
 * Returns the real, full path (with resolved symbolic links) or `undefined`
 * on error or if the file does not exist.
 * Does strictly require one String argument (the path).
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.realpath = function(path) {};

/**
 * Returns directory part of the given path (without tailing slash/backslash)
 * or `undefined` on error.
 * Does strictly require one String argument (the path).
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.dirname = function(path) {};

/**
 * Returns file base part of the given path (name and extension, without parent directory)
 * or `undefined` on error.
 * Does strictly require one String argument (the path).
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.basename = function(path) {};

/**
 * Returns a plain object containing information about a given file,
 * directory or `undefined` on error.
 * Does strictly require one String argument (the path).
 *
 * The returned object has the properties:
 *
 * {
 *    path: String,   // given path (function argument)
 *    size: Number,   // size in bytes
 *    mtime: Date,    // Last modification time
 *    ctime: Date,    // Creation time
 *    atime: Date,    // Last accessed time
 *    owner: String,  // User name of the file owner
 *    group: String,  // Group name of the file group
 *    uid: Number,    // User ID of the owner
 *    gid: Number,    // Group ID of the group
 *    inode: Number,  // Inode of the file
 *    device: Number, // Device identifier/no of the file
 *    mode: String,   // Octal mode representation like "644" or "755"
 *    modeval: Number // Numeric file mode bitmask, use `fs.mod2str(mode)` to convert to a string like 'drwxr-xr-x'.
 * }
 *
 * @param {string} path
 * @return {object|undefined}
 */
fs.stat = function(path) {};

/**
 * Returns a plain object containing information about a given file,
 * where links are not resolved. Return value is the same as in
 * `fs.stat()`.
 *
 * @see fs.stat()
 * @param {string} path
 * @return {object|undefined}
 */
fs.lstat = function(path) {};

/**
 * Returns last modified time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {Date|undefined}
 */
fs.mtime = function(path) {};

/**
 * Returns creation time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {Date|undefined}
 */
fs.ctime = function(path) {};

/**
 * Returns last access time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {Date|undefined}
 */
fs.atime = function(path) {};

/**
 * Returns the name of the file owner of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.owner = function(path) {};

/**
 * Returns the group name of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.group = function(path) {};

/**
 * Returns the file size in bytes of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {number|undefined}
 */
fs.size = function(path) {};

/**
 * Returns a string representation of a file mode bit mask, e.g.
 * for a (octal) mode `0755` directory the output will be '755' or 'rwxrwxrwx' (see flags below).
 * Does strictly require one (integral) Number argument (the input mode) at first.
 *
 * If flags are given they modify the output as follows:
 *
 *  flags == 'o' (octal)    : returns a string with the octal representation (like 755 or 644)
 *  flags == 'l' (long)     : returns a string like 'rwxrwxrwx', like `ls -l` but without preceeding file type character.
 *  flags == 'e' (extended) : output like `ls -l` ('d'=directory, 'c'=character device, 'p'=pipe, ...)
 *
 * @param {number} mode
 * @param {string} [flags]
 * @return {string|undefined}
 */
fs.mod2str = function(mode, flags) {};

/**
 * Returns a numeric representation of a file mode bit mask given as string, e.g.
 * "755", "rwx------", etc.
 * Does strictly require one argument (the input mode). Note that numeric arguments
 * will be reinterpreted as string, so that 755 is NOT the bit mask 0x02f3, but seen
 * as 0755 octal.
 *
 * @param {string} mode
 * @return {number|undefined}
 */
fs.str2mod = function(mode) {};

/**
 * Returns true if a given path points to an existing "node" in the file system (file, dir, pipe, link ...),
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.exists = function(path) {};

/**
 * Returns true if the current user has write permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.iswritable = function(path) {};

/**
 * Returns true if the current user has read permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.isreadable = function(path) {};

/**
 * Returns true if the current user has execution permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.isexecutable = function(path) {};

/**
 * Returns true if a given path points to a directory, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.isdir = function(path) {};

/**
 * Returns true if a given path points to a regular file, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.isfile = function(path) {};

/**
 * Returns true if a given path points to a link, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.islink = function(path) {};

/**
 * Returns true if a given path points to a fifo (named pipe), false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.isfifo = function(path) {};

/**
 * Switches the current working directory to the specified path. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 *
 * @param {string} path
 * @return {boolean}
 */
fs.chdir = function(path) {};

/**
 * Creates a new empty directory for the specified path. Returns true on success, false on error.
 * Require one String argument (the input path), and one optional option argument.
 * If the options is "p" or "parents" (similar to unix `mkdir -p`), parent directories will be
 * created recursively. If the directory already exists, the function returns success, if the
 * creation of the directory or a parent directory fails, the function returns false.
 * Note that it is possible that the path might be only partially created in this case.
 *
 * @param {string} path
 * @param {string} [options]
 * @return {boolean}
 */
fs.mkdir = function(path, options) {};

/**
 * Removes an empty directory specified by a given path. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 * Note that the function also fails if the directory is not empty (no recursion),
 *
 * @param {string} path
 * @return {boolean}
 */
fs.rmdir = function(path) {};

/**
 * Removes a file or link form the file system. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 * Note that the function also fails if the given path is a directory. Use `fs.rmdir()` in this case.
 *
 * @param {string} path
 * @return {boolean}
 */
fs.unlink = function(path) {};

/**
 * Changes the name of a file or directory. Returns true on success, false on error.
 * Does strictly require two String arguments: The input path and the new name path.
 * Note that this is a basic filesystem i/o function that fails if the parent directory,
 * or the new file does already exist.
 *
 * @param {string} path
 * @param {string} new_path
 * @return {boolean}
 */
fs.rename = function(path, new_path) {};

/**
 * Lists the contents of a directory (basenames only), undefined if the function failed to open the directory
 * for reading. Results are unsorted.
 *
 * @param {string} path
 * @return {array|undefined}
 */
fs.readdir = function(path) {};

/**
 * File pattern (fnmatch) based listing of files.
 *
 * @param {string} pattern
 * @return {array|undefined}
 */
fs.glob = function(pattern) {};

/**
 * Creates a symbolic link, returns true on success, false on error.
 *
 * @param {string} path
 * @param {string} link_path
 * @return {boolean}
 */
fs.symlink = function(path, link_path) {};

/**
 * Changes the modification and access time of a file or directory. Returns true on success, false on error.
 * Does strictly require three argument: The input path (String), the last-modified time (Date) and the last
 * access time (Date).
 *
 * @param {string} path
 * @param {Date} [mtime]
 * @param {Date} [atime]
 * @return {boolean}
 */
fs.utime = function(path, mtime, atime) {};

/**
 * Creates a (hard) link, returns true on success, false on error.
 *
 * @param {string} path
 * @param {string} link_path
 * @return {boolean}
 */
fs.hardlink = function(path, link_path) {};

/**
 * Returns the target path of a symbolic link, returns a String or `undefined` on error.
 * Does strictly require one String argument (the path).
 * Note: Windows: returns undefined, not implemented.
 *
 * @param {string} path
 * @return {string|undefined}
 */
fs.readlink = function(path) {};

/**
 * Creates a (hard) link, returns true on success, false on error.
 *
 * @param {string} path
 * @param {string|number} [mode]
 * @return {boolean}
 */
fs.chmod = function(path, mode) {};

/**
 * Contains the (execution path) PATH separator,
 * e.g. ":" for Linux/Unix or ";" for win32.
 *
 * @var {string}
 */
fs.pathseparator = "";

/**
 * Contains the directory separator, e.g. "/"
 * for Linux/Unix or "\" for win32.
 *
 * @var {string}
 */
fs.directoryseparator = "";

/** @file: mod.fs.ext.hh */

/**
 * Recursive directory walking. The argument `path` specifies the root directory
 * of the file search - that is not a pattern with wildcards, but a absolute or
 * relative path. The second argument `options` can be
 *
 *  - a string: then it is the pattern to filter by the file name.
 *
 *  - a plain object with one or more of the properties:
 *
 *      - name: {string} Filter by file name match pattern (fnmatch based, means with '*','?', etc).
 *
 *      - type: {string} Filter by file type, where
 *
 *          - "d": Directory
 *          - "f": Regular file
 *          - "l": Symbolic link
 *          - "p": Fifo (pipe)
 *          - "s": Socket
 *          - "c": Character device (like /dev/tty)
 *          - "b": Block device (like /dev/sda)
 *          - "h": Include hidden files (Win: hidden flag, Linux/Unix: no effect, intentionally
 *                 not applied to files with a leading dot, which are normal files, dirs etc).
 *
 *      - depth: {number} Maximum directory recursion depth. `0` lists nothing, `1` the contents of the
 *               root directory, etc.
 *
 *      - icase: {boolean} File name matching is not case sensitive (Linux/Unix: default false, Win32: default true)
 *
 *      - filter: [Function A callback invoked for each file that was not yet filtered out with the
 *                criteria listed above. The callback gets the file path as first argument. With that
 *                you can:
 *
 *                  - Add it to the output by returning `true`.
 *
 *                  - Not add it to the output list by returning `false`, `null`, or `undefined`. That is
 *                    useful e.g. if you don't want to list any files, but process them instead, or update
 *                    global/local accessible variables depending on the file paths you get.
 *
 *                  - Add a modified path or other string by returning a String. That is really useful
 *                    e.g. if you want to directly return the contents of files, or checksums etc etc etc.
 *                    You get a path, and specify the output yourself.
 *
 * @throws {Error}
 * @param {string} path
 * @param {string|Object} [options]
 * @param {function} [filter]
 * @return {array|undefined}
 */
fs.find = function(path, options, filter) {};

/**
 * Copies a file from one location `source_path` to another (`target_path`),
 * similar to the `cp` shell command. The argument `options` can  encompass
 * the key-value pairs
 *
 *    {
 *      "recursive": {boolean}=false
 *    }
 *
 * Optionally, it is possible to specify the string 'r' or '-r' instead of
 * `{recursive:true}` as third argument.
 *
 * @throws {Error}
 * @param {string} source_path
 * @param {string} target_path
 * @param {object} [options]
 * @return {boolean}
 */
fs.copy = function(source_path, target_path, options) {};

/**
 * Moves a file or directory from one location `source_path` to another (`target_path`),
 * similar to the `mv` shell command. File are NOT moved accross disks (method will fail).
 *
 * @throws {Error}
 * @param {string} source_path
 * @param {string} target_path
 * @return {boolean}
 */
fs.move = function(source_path, target_path) {};

/**
 * Deletes a file or directory (`target_path`), similar to the `rm` shell
 * command. The argument `options` can  encompass the key-value pairs
 *
 *    {
 *      "recursive": {boolean}=false
 *    }
 *
 * Optionally, it is possible to specify the string 'r' or '-r' instead of
 * `{recursive:true}` as third argument.
 *
 * Removing is implicitly forced (like "rm -f").
 *
 * @throws {Error}
 * @param {string} target_path
 * @param {string|object} [options]
 * @return {boolean}
 */
fs.remove = function(target_path, options) {};

/** @file: mod.fs.file.hh */

/**
 * File object constructor, creates a fs.file object when
 * invoked with the `new` keyword. Optionally, path and openmode
 * can be specified to directly open the file. See `File.open()`
 * for details.
 *
 * @constructor
 * @throws {Error}
 * @param {string} [path]
 * @param {string} [openmode]
 * @return {fs.file}
 */
fs.file = function(path, openmode) {};

/**
 * Opens a file given the path and corresponding "open mode". The
 * mode is a string defining flags from adding or omitting characters,
 * where the characters are (with exception of mode "a+") compliant
 * to the ANSI C `fopen()` options. Additional characters enable
 * further file operations and functionality. The options are:
 *
 * - "r": Open for `r`eading. The file must exist. Set the position
 *        to the beginning of the file (ANSI C).
 *
 * - "w": Open for `w`riting. Create if the file is not existing yet,
 *        truncate the file (discard contents). Start position is the
 *        beginning of the file. (ANSI C).
 *
 * - "a": Open for `a`ppending. Create if the file is not existing yet,
 *        set the write position to the end of the file. (ANSI C).
 *
 * - "r+": Open for `r`eading and writing, the file must exist. The
 *        read/write position is set to the beginning of the file
 *        (ANSI C).
 *
 * - "w+": Open for `w`riting and reading. Create if the file is not
 *        existing yet, truncate the file (discard contents). Start
 *        position is the beginning of the file. (ANSI C).
 *
 * - "a+": Open for `a`ppending and reading. Create if the file is not
 *        existing yet. Start position is the beginning of the file.
 *        Warning: The write position is guaranteed to be the end of
 *        the file, but ANSI C specifies that the read position is
 *        separately handled from the write position, and the write
 *        position is implicitly always the end of the file. This is
 *        NOT guaranteed in this implementation. When reading you must
 *        `seek()` to the read position yourself.
 *
 * - "b": Optional flag: Open to read/write `b`inary. Reading will return
 *        a `Buffer` in this case. Writing a `Buffer` will write binary
 *        data.
 *
 * - "t": Optional flag: Open to read/write `t`ext (in contrast to binary,
 *        this is already the default and only accepted because it is
 *        known on some platforms).
 *
 * - "x": Optional flag: E`x`clusive creation. This causes opening for write
 *        to fail with an exception if the file already exists. You can
 *        use this to prevent accidentally overwriting existing files.
 *
 * - "e": Optional flag: Open `e`xisting files only. This is similar to "r+"
 *        and can be used for higher verbosity or ensuring that no file will
 *        be created. The open() call fails with an exception if the file
 *        does not exist.
 *
 * - "c": Optional flag: `C`reate file if not existing. This is the explicit
 *        specification of the default open for write/append behaviour. This
 *        flag implicitly resets the `e` flag.
 *
 * - "p": Optional flag: `P`reserve file contents. This is an explicit order
 *        that opening for write does not discard the current file contents.
 *
 * - "s": Optional flag: `S`ync. Means that file operations are implicitly
 *        forced to be read from / written to the disk or device. Ignored if
 *        the platform does not support it.
 *
 * - "n": Optional flag: `N`onblocking. Means that read/write operations that
 *        would cause the function to "sleep" until data are available return
 *        directly with empty return value. Ignored if the platform does not
 *        support it (or not implemented for the platform).
 *
 * The flags (characters) are not case sensitive, so `file.open(path, "R")` and
 * `file.open(path, "r")` are identical.
 *
 * Although the ANSI open flags are supported it is at a second glance more explicit
 * to use the optional flags in combination with "r" and "w" or "a", e.g.
 *
 * - `file.open(path, "rwcx")`         --> open for read/write, create if not yet existing,
 *                                         and only if not yet existing.
 *
 * - `file.open(path, "wep")`          --> open for write, only existing, preserve contents.
 *
 * - `file.open("/dev/cdev", "rwens")` --> open a character device for read/write, must exist,
 *                                         nonblocking, sync.
 *
 * The function returns the reference to `this`.
 *
 * @throws {Error}
 * @param {string} [path]
 * @param {string} [openmode]
 * @return {fs.file}
 */
fs.file.open = function(path, openmode) {};

/**
 * Closes a file. Returns `this` reference.
 *
 * @return {fs.file}
 */
fs.file.close = function() {};

/**
 * Returns true if a file is closed.
 *
 * @return {boolean}
 */
fs.file.closed = function() {};

/**
 * Returns true if a file is opened.
 *
 * @return {boolean}
 */
fs.file.opened = function() {};

/**
 * Returns true if the end of the file is reached. This
 * is practically interpreted as:
 *
 *  - when the file or pipe signals EOF,
 *  - when the file is not opened,
 *  - when a pipe is not connected or broken
 *
 * @return {boolean}
 */
fs.file.eof = function() {};

/**
 * Reads data from a file, where the maximum number of bytes
 * to read can be specified. If `max_size` is not specified,
 * then as many bytes as possible are read (until EOF, until
 * error or until the operation would block).
 *
 * Note: If the end of the file is reached, the `eof()`
 *       method will return true and the `read()` method
 *       will return `undefined` as indication.
 *
 * @throws {Error}
 * @param {number} [max_bytes]
 * @return {string|buffer}
 */
fs.file.read = function(max_size) {};

/**
 * Read string data from the opened file and return when
 * detecting a newline character. The newline character
 * defaults to the operating system newline and can be
 * changed for the file by setting the `newline` property
 * of the file (e.g. `myfile.newline = "\r\n"`).
 *
 * Note: This function cannot be used in combination
 * with the nonblocking I/O option.
 *
 * Note: If the end of the file is reached, the `eof()`
 *       method will return true and the `read()` method
 *       will return `undefined` to indicate that no
 *       empty line was read but nothing at all.
 *
 * Note: This function is slower than `fs.file.read()` or
 *       `fs.readfile()` because it has to read unbuffered
 *       char-by-char. If you intend to read an entire file
 *       and filter the lines prefer `fs.readfile()` with
 *       line processing callback.
 *
 * @throws {Error}
 * @return {string}
 */
fs.file.readln = function() {};

/**
 * Write data to a file, returns the number of bytes written.
 * Normally all bytes are written, except if nonblocking i/o
 * was specified when opening the file.
 *
 * @throws {Error}
 * @param {string|buffer} data
 * @return {number}
 */
fs.file.write = function(data) {};

/**
 * Write string data to a file and implicitly append
 * a newline character. The newline character defaults
 * to the operating system newline (Windows CRLF, else
 * LF, no old Mac CR). This character can be changed
 * for the file by setting the `newline` property of
 * the file (e.g. myfile.newline = "\r\n").
 * Note: This function cannot be used in combination
 * with the nonblocking I/O option. The method throws
 * an exception if not all data could be written.
 *
 * @throws {Error}
 * @param {string} data
 */
fs.file.writeln = function(data) {};

/**
 * C style formatted output to the opened file.
 * The method is used identically to `printf()`.
 * Note: This function cannot be used in combination
 * with the nonblocking I/O option. The method
 * throws an exception if not all data could be
 * written.
 *
 * @throws {Error}
 * @param {string} format
 * @param {...*} args
 */
fs.file.printf = function(format, args) {};

/**
 * Flushes the file write buffer. Ignored on platforms where this
 * is not required. Returns reference to `this`.
 *
 * @throws {Error}
 * @return {fs.file}
 */
fs.file.flush = function() {};

/**
 * Returns the current file position.
 *
 * @throws {Error}
 * @return {number}
 */
fs.file.tell = function() {};

/**
 * Sets the new file position (read and write). Returns the
 * actual position (from the beginning of the file) after the
 * position was set. The parameter whence specifies from where
 * the position shall be set:
 *
 *  - "begin" (or "set"): From the beginning of the file (SEEK_SET)
 *  - "end"             : From the end of the file backward (SEEK_END)
 *  - "current" ("cur") : From the current position forward (SEEK_CUR)
 *
 * @throws {Error}
 * @param {number} position
 * @param {string} [whence=begin]
 * @return {number}
 */
fs.file.seek = function(position, whence) {};

/**
 * Returns the current file size in bytes.
 *
 * @throws {Error}
 * @return {number}
 */
fs.file.size = function() {};

/**
 * Returns details about the file including path, size, mode
 * etc. @see fs.stat() for details.
 *
 * @throws {Error}
 * @return {object}
 */
fs.file.stat = function() {};

/**
 * Forces the operating system to write the file to a remote device
 * or block device (disk). This is a Linux/UNIX explicit variant of
 * flush. However, flush is not sync, and sync is only needed in
 * special situations. On operating systems that cannot sync this
 * function is an alias of `fs.flush()`. Returns reference to `this`.
 *
 * The optional argument `no_metadata` specifies (when true) that
 * only the contents of the file shall be synced, but not the file
 * system meta information.
 *
 * @throws {Error}
 * @param {boolean} [no_metadata]
 * @return {fs.file}
 */
fs.file.sync = function() {};

/**
 * Locks the file. By default exclusively, means no other process
 * can read or write. Optionally the file can be locked exclusively
 * or shared depending on the `access` argument:
 *
 *  - "x", "" : Exclusive lock
 *  - "s"     : Shared lock
 *
 * @throws {Error}
 * @param {string} access
 * @return {fs.file}
 */
fs.file.lock = function(access) {};

/**
 * Unlocks a previously locked file. Ignored if the platform does not
 * support locking.
 *
 * @throws {Error}
 * @return {fs.file}
 */
fs.file.unlock = function() {};

/** @file: mod.sys.hh */

/**
 * Operating system functionality object.
 * @var {object}
 */
var sys = {};

/**
 * Returns the ID of the current process or `undefined` on error.
 *
 * @return {number|undefined}
 */
sys.pid = function() {};

/**
 * Returns the ID of the current user or `undefined` on error.
 *
 * @return {number|undefined}
 */
sys.uid = function() {};

/**
 * Returns the ID of the current group or `undefined` on error.
 *
 * @return {number|undefined}
 */
sys.gid = function() {};

/**
 * Returns the login name of a user or `undefined` on error.
 * If the user ID is not specified (called without arguments), the ID of the current user is
 * used.
 *
 * @param {number} [uid]
 * @return {string|undefined}
 */
sys.user = function(uid) {};

/**
 * Returns the group name of a group ID or `undefined` on error.
 * If the group ID is not specified (called without arguments), the group ID of the
 * current user is used.
 *
 * @param {number} [gid]
 * @return {string|undefined}
 */
sys.group = function(gid) {};

/**
 * Returns a plain object containing information about the operating system
 * running on, `undefined` on error. The object looks like:
 *
 * {
 *   sysname: String, // e.g. "Linux"
 *   release: String, // e.g. ""3.16.0-4-amd64"
 *   machine: String, // e.g. "x86_64"
 *   version: String, // e.g. "#1 SMP Debian 3.16.7-ckt20-1+deb8u3 (2016-01-17)"
 * }
 *
 * @return {object|undefined}
 */
sys.uname = function() {};

/**
 * Makes the thread sleep for the given time in seconds (with sub seconds).
 * Note that this function blocks the complete thread until the time has
 * expired or sleeping is interrupted externally.
 *
 * @param {number} seconds
 * @return {boolean}
 */
sys.sleep = function(seconds) {};

/**
 * Returns the time in seconds (with sub seconds) of a selected
 * time/clock source:
 *
 *  - "r": Real time clock (value same as Date object, maybe
 *         higher resolution)
 *  - "b": Boot time (if available, otherwise equal to "m" source)
 *
 *  - "m": Monotonic time, starts at zero when the function
 *         is first called.
 *
 * Returns NaN on error or when a source is not supported on the
 * current platform.
 *
 * @param {string} clock_source
 * @return {number} seconds
 */
sys.clock = function(clock_source) {};

/**
 * Returns true if the descriptor given as string
 * is an interactive TTY or false otherwise, e.g.
 * when connected to a pipe / file source. Valid
 * descriptors are:
 *
 *  - "stdin"  or "i": STDIN  (standard input read with confirm, prompt etc)
 *  - "stdout" or "o": STDOUT (standard output fed by print())
 *  - "stderr" or "e": STDERR (standard error output, e.g. fed by alert())
 *
 * The function returns undefined if not implemented on the
 * platform or if the descriptor name is incorrect.
 *
 * @param {string} descriptorName
 * @return {boolean}
 */
sys.isatty = function(descriptorName) {};

/**
 * Returns path ("realpath") of the executable where the ECMA script is
 * called from (or undefined on error or if not allowed).
 *
 * @return {string|undefined}
 */
sys.executable = function() {};

/**
 * Mundane auditive beeper signal with a given frequency in Hz and duration
 * in seconds. Only applied if the hardware and system supports beeping,
 * otherwise no action. Returns true if the system calls were successful.
 *
 * @param {number} frequency
 * @param {number} duration
 * @return {boolean}
 */
sys.beep = function() {};

/** @file: mod.sys.exec.hh */

/**
 * Execute a process, optionally fetch stdout, stderr or pass stdin data.
 *
 * - The `program` is the path to the executable.
 *
 * - The `arguments` (if not omitted/undefined) is an array with values, which
 *   are coercible to strings.
 *
 * - The `options` is a plain object with additional flags and options.
 *   All these options are optional and have sensible default values:
 *
 *    {
 *      // Plain object for environment variables to set.
 *      env     : {object}={},
 *
 *      // Optional text that is passed to the program via stdin piping.
 *      stdin   : {String}="",
 *
 *      // If true the output is an object containing the fetched output in the property `stdout`.
 *      // The exit code is then stored in the property `exitcode`.
 *      // If it is a function, see callbacks below.
 *      stdout  : {boolean|function}=false,
 *
 *      // If true the output is an object containing the fetched output in the property `stderr`.
 *      // The exit code is then stored in the property `exitcode`.
 *      // If the value is "stdout", then the stderr output is redirected to stdout, and the
 *      // option `stdout` is implicitly set to `true` if it was `false`.
 *      // If it is a function, see callbacks below.
 *      stderr  : {boolean|function|"stdout"}=false,
 *
 *      // Normally the user environment is also available for the executed child process. That
 *      // might cause issues, e.g. with security. To prevent passing through the current environment,
 *      // set this property to `true`.
 *      noenv   : {boolean}=false,
 *
 *      // Normally the execution also uses the search path variable ($PATH) to determine which
 *      // program to run - Means setting the `program` to `env` or `/usr/bin/env` is pretty much
 *      // the same. However, you might not want that programs are searched. By setting this option
 *      // to true, you must use `/usr/bin/env`.
 *      nopath  : {boolean}=false,
 *
 *      // Normally the function throws exceptions on execution errors.
 *      // If that is not desired, set this option to `true`, and the function will return
 *      // `undefined` on errors. However, it is possible that invalid arguments or script
 *      // engine errors still throw.
 *      noexcept: {boolean}=false,
 *
 *      // The function can be called like `fs.exec( {options} )` (options 1st argument). In this
 *      // case the program to execute can be specified using the `program` property.
 *      program : {string},
 *
 *      // The function can also be called with the options as first or second argument. In both
 *      // cases the command line arguments to pass on to the execution can be passed as the `args`
 *      // property.
 *      args    : {array},
 *
 *      // Process run timeout in ms, the process will be terminated (and SIGKILL killed later if
 *      // not terminating itself) if it runs longer than this timeout.
 *      timeout : {number}
 *
 *    }
 *
 * - The return value is:
 *
 *    - the exit code of the process, is no stdout nor stderr fetching is switched on,
 *
 *    - a plain object if any fetching is enabled:
 *
 *        {
 *          exitcode: {number},
 *          stdout  : {string},
 *          stderr  : {string}
 *        }
 *
 *    - `undefined` if exec exceptions are disabled and an error occurs.
 *
 * @throws {Error}
 * @param {string} program
 * @param {array} [arguments]
 * @param {object} [options]
 * @return {number|object}
 */
sys.exec = function(program, arguments, options) {};

/**
 * Execute a shell command and return the STDOUT output. Does
 * not throw exceptions. Returns an empty string on error. Does
 * not redirect stderr to stdout, this can be done in the shell
 * command itself. The command passed to the shell is intentionally
 * NOT escaped (no "'" to "\'" and no "\" to "\\").
 *
 * @throws {Error}
 * @param {string} command
 * @return {string}
 */
sys.shell = function(command) {};

/**
 * Adds quotes and escapes conflicting characters,
 * so that the given text can be used as a shell
 * argument.
 * (This is not needed for `sys.exec()`, as this
 * method allows specifying program arguments as
 * array. `sys.shell()` may very well need escaping).
 *
 * @param {string} arg
 * @return {string}
 */
sys.escapeshellarg = function(arg) {};

/**
 * Starts a program as child process of this application,
 * throws on error. The arguments are identical to
 * `sys.exec()`, except that the callbacks for `stdout`
 * and `stderr` do not apply. Output and error pipes of
 * the child processes can be accessed via the `stdout`
 * and `stderr` properties of this object.
 * To check if the process is still running use the
 * boolean `running` property, which implicitly updates
 * the stdin/stderr/stdout pipes.
 *
 * @constructor
 * @throws {Error}
 * @param {string} program
 * @param {array} [arguments]
 * @param {object} [options]
 *
 * @property {boolean} running      - True if the child process has not terminated yet. Updates stdout/stderr/stdin/exitcode.
 * @property {number}  exitcode     - The exit code of the child process, or -1.
 * @property {string}  stdout       - Current output received from the child process.
 * @property {string}  stderr       - Current stderr output received from the child process.
 * @property {string}  stdin        - Current stdin left to transmit to the child process.
 * @property {number}  runtime      - Time in seconds the process is running for.
 * @property {number}  timeout      - The configured timeout in milliseconds. The process will be "killed" after exceeding.
 */
sys.process = function(program, arguments, options) {};

/**
 * Sends a termination signal to the child process, normally
 * a graceful termination request (SIGTERM/SIGQUIT), when
 * `force` is `true`, the `SIGKILL` event is used.
 * On Windows this it applies a process termination, `force`
 * is ignored.
 *
 * @param {boolean} force
 */
sys.process.prototype.kill = function(force) {};

/** @file: mod.ext.serial_port.hh */

/**
 * Serial port handling object constructor, optionally with
 * initial port settings like '<port>,115200n81'.
 *
 * @constructor
 * @throws {Error}
 * @param {string|undefined} settings
 *
 * @property {string}  port         - Port setting (e.g. "/dev/ttyS0", "COM98", "usbserial1"). Effective after re-opening.
 * @property {number}  baudrate     - Baud rate setting (e.g. 9600, 115200). Effective after re-opening.
 * @property {number}  databits     - Data bit count setting (7, 8). Effective after re-opening.
 * @property {number}  stopbits     - Stop bit count setting (1, 1.5, 2). Effective after re-opening.
 * @property {string}  parity       - Parity setting ("n", "o", "e"). Effective after re-opening.
 * @property {string}  flowcontrol  - Flow control selection ("none", "xonxoff", "rtscts").
 * @property {string}  settings     - String representation of all port config settings.
 * @property {number}  timeout      - Default reading timeout setting in milliseconds. Effective after re-opening.
 * @property {string}  txnewline    - Newline character for sending data (e.g. "\n", "\r", "\r\n", etc)
 * @property {string}  rxnewline    - Newline character for receiving data (end-of-line detection for line based reading).
 * @property {boolean} closed       - Holds true when the port is not opened.
 * @property {boolean} isopen       - Holds true when the port is opened.
 * @property {boolean} rts          - Current state of the RTS line (only if supported by the port/driver).
 * @property {boolean} cts          - Current state of the CTS line (only if supported by the port/driver).
 * @property {boolean} dtr          - Current state of the DTR line (only if supported by the port/driver).
 * @property {boolean} dsr          - Current state of the DSR line (only if supported by the port/driver).
 * @property {boolean} error        - Error code of the last method call.
 * @property {string}  errormessage - Error string representation of the last method call.
 */
sys.serialport = function(optional_settings) {};

/**
 * Closes the port, resets errors.
 * @return {sys.serialport}
 */
sys.serialport.prototype.close = function() {};

/**
 * Opens the port, optionally with given settings.
 *
 * @throws {Error}
 * @param {string|undefined} port
 * @param {string|undefined} settings
 * @return {sys.serialport}
 */
sys.serialport.prototype.open = function(port, settings) {};

/**
 * Clears input and output buffers of the port.
 *
 * @return {sys.serialport}
 */
sys.serialport.prototype.purge = function() {};

/**
 * Reads received data, aborts after `timeout_ms` has expired.
 * Returns an empty string if nothing was received or timeout.
 *
 * @param {number} timeout_ms
 * @return {string}
 */
sys.serialport.prototype.read = function(timeout_ms) {};

/**
 * Sends off the the given data.
 *
 * @param {string} data
 * @return {sys.serialport}
 */
sys.serialport.prototype.write = function(data) {};

/**
 * Reads a line from the received data. Optionally with a given
 * timeout (else the default timeout), and supression of empty
 * lines. Returns `undefined` if no line was received, the fetched
 * line otherwise.
 *
 * @throws {Error}
 * @param {number}  timeout_ms
 * @param {boolean} ignore_empty
 * @return {string|undefined}
 */
sys.serialport.prototype.readln = function(timeout_ms, ignore_empty) {};

/**
 * Sends a line, t.m. `this.txnewline` is automatically appended.
 *
 * @throws {Error}
 * @param {string}  data
 * @return {sys.serialport}
 */
sys.serialport.prototype.writeln = function(data) {};

/**
 * Returns a plain object containing short names and paths for
 * eligible serial devices. Both keys and values are accepted
 * as `port` setting.
 *
 * @throws {Error}
 * @return {object}
 */
sys.serialport.portlist = function(data) {};

/** @file: mod.sys.hash.hh */

/**
 * Hashing functionality of the system object.
 * @var {object}
 */
sys.hash = {};

/**
 * CRC8 (PEC) of a string or buffer.
 * (PEC CRC is: polynomial: 0x07, initial value: 0x00, final XOR: 0x00)
 *
 * @param {string|buffer} data
 * @return {number}
 */
sys.hash.crc8 = function(data) {};

/**
 * CRC16 (USB) of a string or buffer.
 * (USB CRC is: polynomial: 0x8005, initial value: 0xffff, final XOR: 0xffff)
 *
 * @param {string|buffer} data
 * @return {number}
 */
sys.hash.crc16 = function(data) {};

/**
 * CRC32 (CCITT) of a string or buffer.
 *
 * @param {string|buffer} data
 * @return {number}
 */
sys.hash.crc32 = function(data) {};

/**
 * MD5 of a string, buffer or file (if `isfile==true`).
 *
 * @param {string|buffer} data
 * @param {boolean} [isfile=false]
 * @return {string}
 */
sys.hash.md5 = function(data, isfile) {};

/**
 * SHA1 of a string, buffer or file (if `isfile==true`).
 *
 * @param {string|buffer} data
 * @param {boolean} [isfile=false]
 * @return {string}
 */
sys.hash.sha1 = function(data, isfile) {};

/**
 * SHA512 of a string, buffer or file (if `isfile==true`).
 *
 * @param {string|buffer} data
 * @param {boolean} [isfile=false]
 * @return {string}
 */
sys.hash.sha512 = function(data, isfile) {};
