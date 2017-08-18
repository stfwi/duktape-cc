
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
 * @param String path
 * @param undefined|String|function conf
 * @return String|Buffer
 */
fs.readfile = function(path, conf) {};


/**
 * Writes data into a file Reads a file and returns the contents as text.
 *
 * @param String path
 * @param String|Buffer|Number|Boolean|Object data
 * @param String|Object flags
 * @return Boolean
 */
fs.filewrite = function(path, data, flags) {};


/**
 * Returns the current working directory or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return String | undefined
 */
fs.cwd = function() {};


/**
 * Returns the temporary directory or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return String | undefined
 */
fs.tmpdir = function() {};


/**
 * Returns the path for a temporary file. The file is
 * NOT created yet. Note that using this function for
 * creating temporary files is unsafe, as other processes
 * might create the same file before this program has
 * created it.
 * Accepts one optional argument, the file prefix.
 *
 * @param undefined|String prefix
 * @return String | undefined
 */
fs.tempnam = function(prefix) {};


/**
 * Returns the home directory of the current used or `undefined` on error.
 * Does strictly not accept arguments.
 *
 * @return String | undefined
 */
fs.home = function() {};


/**
 * Returns the real, full path (with resolved symbolic links) or `undefined`
 * on error or if the file does not exist.
 * Does strictly require one String argument (the path).
 *
 * @param String path
 * @return String | undefined
 */
fs.realpath = function(path) {};


/**
 * Returns directory part of the given path (without tailing slash/backslash)
 * or `undefined` on error.
 * Does strictly require one String argument (the path).
 *
 * @param String path
 * @return String | undefined
 */
fs.dirname = function(path) {};


/**
 * Returns file base part of the given path (name and extension, without parent directory)
 * or `undefined` on error.
 * Does strictly require one String argument (the path).
 *
 * @param String path
 * @return String | undefined
 */
fs.basename = function(path) {};


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
 * @param Number mode
 * @param String flags [optional]
 * @return String | undefined
 */
fs.mod2str = function(mode, flags='') {};


/**
 * Returns a numeric representation of a file mode bit mask given as string, e.g.
 * "755", "rwx------", etc.
 * Does strictly require one argument (the input mode). Note that numeric arguments
 * will be reinterpreted as string, so that 755 is NOT the bit mask 0x02f3, but seen
 * as 0755 octal.
 *
 * @param String mode
 * @return Number | undefined
 */
fs.str2mod = function(mode) {};


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
 * @param String path
 * @return Object | undefined
 */
fs.stat = function(path) {};


/**
 * Returns the file size in bytes of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Number | undefined
 */
fs.size = function(path) {};


/**
 * Returns the name of the file owner of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return String | undefined
 */
fs.owner = function(path) {};


/**
 * Returns the group name of a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return String | undefined
 */
fs.group = function(path) {};


/**
 * Returns last modified time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Date | undefined
 */
fs.mtime = function(path) {};


/**
 * Returns last access time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Date | undefined
 */
fs.atime = function(path) {};


/**
 * Returns creation time a given file path, or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Date | undefined
 */
fs.ctime = function(path) {};


/**
 * Returns true if a given path points to an existing "node" in the file system (file, dir, pipe, link ...),
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.exists = function(path) {};


/**
 * Returns true if a given path points to a regular file, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.isfile = function(path) {};


/**
 * Returns true if a given path points to a directory, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.isdir = function(path) {};


/**
 * Returns true if a given path points to a link, false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.islink = function(path) {};


/**
 * Returns true if a given path points to a fifo (named pipe), false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.isfifo = function(path) {};


/**
 * Returns true if the current user has write permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.iswritable = function(path) {};


/**
 * Returns true if the current user has read permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.isreadable = function(path) {};


/**
 * Returns true if the current user has execution permission to a given path,
 * false otherwise or undefined on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
 */
fs.isexecutable = function(path) {};


/**
 * Returns the target path of a symbolic link, returns a String or `undefined` on error.
 * Does strictly require one String argument (the path).
 * Note: Windows: returns undefined, not implemented.
 *
 * @param String path
 * @return String | undefined
 */
fs.readlink = function(path) {};


/**
 * Switches the current working directory to the specified path. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 *
 * @param String path
 * @return Boolean
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
 * @param String path
 * @param String options
 * @return Boolean
 */
fs.mkdir = function(path, options) {};


/**
 * Removes an empty directory specified by a given path. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 * Note that the function also fails if the directory is not empty (no recursion),
 *
 * @param String path
 * @return Boolean
 */
fs.rmdir = function(path) {};


/**
 * Removes a file or link form the file system. Returns true on success, false on error.
 * Does strictly require one String argument (the input path).
 * Note that the function also fails if the given path is a directory. Use `fs.rmdir()` in this case.
 *
 * @param String path
 * @return Boolean
 */
fs.unlink = function(path) {};


/**
 * Changes the modification and access time of a file or directory. Returns true on success, false on error.
 * Does strictly require three argument: The input path (String), the last-modified time (Date) and the last
 * access time (Date).
 *
 * @param String path
 * @param Date mtime
 * @param Date atime
 * @return Boolean
 */
fs.utime = function(path, mtime, atime) {};


/**
 * Changes the name of a file or directory. Returns true on success, false on error.
 * Does strictly require two String arguments: The input path and the new name path.
 * Note that this is a basic filesystem i/o function that fails if the parent directory,
 * or the new file does already exist.
 *
 * @param String path
 * @param String new_path
 * @return Boolean
 */
fs.rename = function(path, new_path) {};


/**
 * Creates a symbolic link, returns true on success, false on error.
 *
 * @param String path
 * @param String link_path
 * @return Boolean
 */
fs.symlink = function(path, link_path) {};


/**
 * Creates a (hard) link, returns true on success, false on error.
 *
 * @param String path
 * @param String link_path
 * @return Boolean
 */
fs.hardlink = function(path, link_path) {};


/**
 * Creates a (hard) link, returns true on success, false on error.
 *
 * @param String path
 * @param String|Number mode
 * @return Boolean
 */
fs.chmod = function(path, mode) {};


/**
 * Lists the contents of a directory (basenames only), undefined if the function failed to open the directory
 * for reading. Results are unsorted.
 *
 * @param String path
 * @return Array|undefined
 */
fs.readdir = function(path) {};


/**
 * File pattern (fnmatch) based listing of files.
 *
 * @param String pattern
 * @return Array|undefined
 */
fs.glob = function(pattern) {};


/**
 * Recursive directory walking. The argument `path` specifies the root directory
 * of the file search - that is not a pattern with wildcards, but a absolute or
 * relative path. The second argument `options` can be
 *
 *  - a string: then it is the pattern to filter by the file name.
 *
 *  - a plain object with one or more of the properties:
 *
 *      - name: [String] Filter by file name match pattern (fnmatch based, means with '*','?', etc).
 *
 *      - type: [String] Filter by file type, where
 *
 *          - "d": Directory
 *          - "f": Regular file
 *          - "l": Symbolic link
 *          - "p": Fifo (pipe)
 *          - "s": Socket
 *          - "c": Character device (like /dev/tty)
 *          - "b": Block device (like /dev/sda)
 *
 *      - depth: [Number] Maximum directory recursion depth. `0` lists nothing, `1` the contents of the
 *               root directory, etc.
 *
 *      - logical: [Boolean] Default is `true`. This affects symbolic links. Normally people are interested
 *                 in the files that links refer to rather than the links themselves. Therefore the logical
 *                 search is default. In contrast, a physical search will refer to the links themselves
 *                 (set `logical:false`) and not to the files pointed to.
 *                 THAT IS ESPESIALLY IMPORTANT IF YOU WANT TO DELETE FILES BASED ON A FIND SEARCH.
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
 *
 * @param String path
 * @param String|Object options
 * @return Array|undefined
 */
fs.find = function(path, options) {};


/**
 * Copies a file from one location `source_path` to another (`target_path`),
 * similar to the `cp` shell command. The argument `options` can  encompass
 * the key-value pairs
 *
 *    {
 *      "recursive": Boolean (default false)
 *    }
 *
 * Optionally, it is possible to specify the string 'r' or '-r' instead of
 * `{recursive:true}` as third argument.
 *
 * @param String source_path
 * @param String target_path
 * @param undefined|Object options
 * @return Boolean
 */
fs.copy = function(source_path, target_path, options) {};


/**
 * Moves a file or directory from one location `source_path` to another (`target_path`),
 * similar to the `mv` shell command.
 *
 * @param String source_path
 * @param String target_path
 * @return Boolean
 */
fs.move = function(source_path, target_path) {};


/**
 * Deletes a file or directory (`target_path`), similar to the `rm` shell
 * command. The argument `options` can  encompass the key-value pairs
 *
 *    {
 *      "recursive": Boolean (default false)
 *    }
 *
 * Optionally, it is possible to specify the string 'r' or '-r' instead of
 * `{recursive:true}` as third argument.
 *
 * Removing is implicitly forced (like "rm -f").
 *
 * @param String target_path
 * @param undefined|Object options
 * @return Boolean
 */
fs.remove = function(target_path, options) {};


/**
 * Returns the ID of the current process or `undefined` on error.
 *
 * @return Number|undefined
 */
sys.pid = function() {};


/**
 * Returns the ID of the current user or `undefined` on error.
 *
 * @return Number|undefined
 */
sys.uid = function() {};


/**
 * Returns the ID of the current group or `undefined` on error.
 *
 * @return Number|undefined
 */
sys.gid = function() {};


/**
 * Returns the login name of a user or `undefined` on error.
 * If the user ID is not specified (called without arguments), the ID of the current user is
 * used.
 *
 * @param undefined|Number uid
 * @return String|undefined
 */
sys.user = function(uid) {};


/**
 * Returns the group name of a group ID or `undefined` on error.
 * If the group ID is not specified (called without arguments), the group ID of the
 * current user is used.
 *
 * @param undefined|Number gid
 * @return String|undefined
 */
sys.group = function(gid) {};


/**
 * Returns path ("realpath") of the executable where the ECMA script is
 * called from (or undefined on error or if not allowed).
 *
 * @return String|undefined
 */
sys.apppath = function() {};


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
 * @return Object|undefined
 */
sys.uname = function() {};


/**
 * Makes the thread sleep for the given time in seconds (with sub seconds).
 * Note that this function blocks the complete thread until the time has
 * expired or sleeping is interrupted externally.
 *
 * @param Number seconds
 * @return bool
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
 * @param String clock_source
 * @return Number seconds
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
 * @param String descriptorName
 * @return Boolean
 */
sys.isatty = function(descriptorName) {};


/**
 * File object constructor, creates a fs.file object when
 * invoked with the `new` keyword. Optionally, path and openmode
 * can be specified to directly open the file. See `File.open()`
 * for details.
 *
 * @constructor
 * @param undefined|String path
 * @param undefined|String openmode
 * @return fs.file
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
 * @param undefined|String path
 * @param undefined|String openmode
 * @return fs.file
 */
fs.file.open = function(path, openmode) {};


/**
 * Closes a file. Returns `this` reference.
 *
 * @return fs.file
 */
fs.file.close = function() {};


/**
 * Returns true if a file is closed.
 *
 * @return Boolean
 */
fs.file.closed = function() {};


/**
 * Returns true if a file is opened.
 *
 * @return Boolean
 */
fs.file.opened = function() {};


/**
 * Reads data from a file, where the maximum number of bytes
 * to read can be specified. If `max_size` is not specified,
 * then as many bytes as possible are read (until EOF, until
 * error or until the operation would block).
 *
 * @param Number|undefined max_bytes
 * @return String|Buffer
 */
fs.file.read = function(max_size) {};


/**
 * Write data to a file, returns the number of bytes written.
 * Normally all bytes are written, except if nonblocking i/o
 * was specified when opening the file.
 *
 * @param String|Buffer data
 * @return Number
 */
fs.file.write = function(data) {};


/**
 * Returns the current file position.
 *
 * @return Number
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
 * @param Number position
 * @param String whence
 * @return Number
 */
fs.file.seek = function(position, whence) {};


/**
 * Returns the current file size in bytes.
 *
 * @return Number
 */
fs.file.size = function() {};


/**
 * Returns details about the file including path, size, mode
 * etc. @see fs.stat() for details.
 *
 * @return Object
 */
fs.file.stat = function() {};


/**
 * Flushes the file write buffer. Ignored on platforms where this
 * is not required. Returns reference to `this`.
 *
 * @return fs.file
 */
fs.file.flush = function() {};


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
 * @param Boolean|undefined no_metadata
 * @return fs.file
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
 * @param string access
 * @return fs.file
 */
fs.file.lock = function(access) {};


/**
 * Unlocks a previously locked file. Ignored if the platform does not
 * support locking.
 *
 * @return fs.file
 */
fs.file.unlock = function() {};


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
 *      env     : [Object]={},
 *
 *      // Optional text that is passed to the program via stdin piping.
 *      stdin   : [String]="",
 *
 *      // If true the output is an object containing the fetched output in the property `stdout`.
 *      // The exit code is then stored in the property `exitcode`.
 *      // If it is a function, see callbacks below.
 *      stdout  : [Boolean|Function]=false,
 *
 *      // If true the output is an object containing the fetched output in the property `stderr`.
 *      // The exit code is then stored in the property `exitcode`.
 *      // If the value is "stdout", then the stderr output is redirected to stdout, and the
 *      // option `stdout` is implicitly set to `true` if it was `false`.
 *      // If it is a function, see callbacks below.
 *      stderr  : [Boolean|Function|"stdout"]=false,
 *
 *      // Normally the user environment is also available for the executed child process. That
 *      // might cause issues, e.g. with security. To prevent passing through the current environment,
 *      // set this property to `true`.
 *      noenv   : [Boolean]=false,
 *
 *      // Normally the execution also uses the search path variable ($PATH) to determine which
 *      // program to run - Means setting the `program` to `env` or `/usr/bin/env` is pretty much
 *      // the same. However, you might not want that programs are searched. By setting this option
 *      // to true, you must use `/usr/bin/env`.
 *      nopath  : [Boolean]=false,
 *
 *      // Normally the function throws exceptions on execution errors.
 *      // If that is not desired, set this option to `true`, and the function will return
 *      // `undefined` on errors. However, it is possible that invalid arguments or script
 *      // engine errors still throw.
 *      noexcept: [Boolean]=false,
 *
 *      // The function can be called like `fs.exec( {options} )` (options 1st argument). In this
 *      // case the program to execute can be specified using the `program` property.
 *      program : [String],
 *
 *      // The function can also be called with the options as first or second argument. In both
 *      // cases the command line arguments to pass on to the execution can be passed as the `args`
 *      // property.
 *      args    : [Array],
 *
 *      // Process run timeout in ms, the process will be terminated (and SIGKILL killed later if
 *      // not terminating itself) if it runs longer than this timeout.
 *      timeout : [Number]
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
 *          exitcode: [Number],
 *          stdout: [String],
 *          stderr: [String]
 *        }
 *
 *    - `undefined` if exec exceptions are disabled and an error occurs.
 *
 * @throw Error
 * @param String program
 * @param undefined|Array arguments
 * @param undefined|Object options
 * @return Number|Object
 */
sys.exec = function(program, arguments, options) {};


/**
 * Execute a shell command and return the STDOUT output. Does
 * not throw exceptions. Returns an empty string on error. Does
 * not redirect stderr to stdout, this can be done in the shell
 * command itself. The command passed to the shell is intentionally
 * NOT escaped (no "'" to "\'" and no "\" to "\\").
 *
 * @throw Error
 * @param String command
 * @return String
 */
sys.shell = function(command) {};

