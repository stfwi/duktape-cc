// note: sha1 c++ code already tested, so only the bindings are tested here_
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha1(plain) == "7037807198c22a7d2b0807371d763779a84fdfcf");
test_expect(sys.hash.sha1("1234567890") == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
test_expect(sys.hash.sha1(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "11966ab9c099f8fabefac54c08d5be2bd8c903af");

var path = fs.tmpdir() + fs.directoryseparator + "jstestsha1.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha1(path, true) == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
fs.unlink(path);
