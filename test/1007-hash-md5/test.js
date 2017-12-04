// note: md5 c++ code already tested, so only the bindings are tested here_
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.md5(plain) == "5289df737df57326fcdd22597afb1fac");
test_expect(sys.hash.md5("1234567890") == "e807f1fcf82d132f9bb018ca6738a19f");
test_expect(sys.hash.md5(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "7cfdd07889b3295d6a550914ab35e068");

var path = fs.tmpdir() + fs.directoryseparator + "jstestmd5.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.md5(path, true) == "e807f1fcf82d132f9bb018ca6738a19f");
fs.unlink(path);
