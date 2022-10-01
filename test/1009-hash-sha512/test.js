// note: sha512 c++ code already tested, so only the bindings are tested here_
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha512(plain) == "27864cc5219a951a7a6e52b8c8dddf6981d098da1658d96258c870b2c88dfbcb51841aea172a28bafa6a79731165584677066045c959ed0f9929688d04defc29");
test_expect(sys.hash.sha512("1234567890") == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
test_expect(sys.hash.sha512(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "50540bc4ae31875fceb3829434c55e3c2b66ddd7227a883a3b4cc8f6cda965ad1712b3ee0008f9cee08da93f5234c1a7bf0e2570ef56d65280ffea691b953efe");

var path = fs.tmpdir() + fs.directoryseparator + "jstestsha512.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha512(path, true) == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
fs.unlink(path);
