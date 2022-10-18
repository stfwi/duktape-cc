var plain = null;
var path = "";


// note: crc8 c++ code already tested, so only the bindings are tested here_
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.crc8(plain) == 0x48);
test_expect(sys.hash.crc8("test crc 8") == 0x7e);
test_expect(sys.hash.crc8(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0xbc);


// note: crc16 (USB) c++ code already tested, so only the bindings are tested here.
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc16([1,2,3])", sprintf("0x%04x", sys.hash.crc16(plain)));
test_expect(sys.hash.crc16(plain) == 0x9E9E);
test_expect(sys.hash.crc16("test crc 16") == 0xF3B8);
test_expect(sys.hash.crc16(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x44D5);


// note: crc32 (CCITT) c++ code already tested, so only the bindings are tested here.
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc32([1,2,3])", sprintf("0x%04x", sys.hash.crc32(plain)));
test_expect(sys.hash.crc32(plain) == 0x55BC801D);
test_expect(sys.hash.crc32("test crc 32") == 0x1C2BE2A9);
test_expect(sys.hash.crc32(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x470B99F4);


// note: md5 c++ code already tested, so only the bindings are tested here_
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.md5(plain) == "5289df737df57326fcdd22597afb1fac");
test_expect(sys.hash.md5("1234567890") == "e807f1fcf82d132f9bb018ca6738a19f");
test_expect(sys.hash.md5(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "7cfdd07889b3295d6a550914ab35e068");
path = fs.tmpdir() + fs.directoryseparator + "jstestmd5.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.md5(path, true) == "e807f1fcf82d132f9bb018ca6738a19f");
fs.unlink(path);


// note: sha1 c++ code already tested, so only the bindings are tested here_
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha1(plain) == "7037807198c22a7d2b0807371d763779a84fdfcf");
test_expect(sys.hash.sha1("1234567890") == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
test_expect(sys.hash.sha1(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "11966ab9c099f8fabefac54c08d5be2bd8c903af");
path = fs.tmpdir() + fs.directoryseparator + "jstestsha1.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha1(path, true) == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
fs.unlink(path);


// note: sha512 c++ code already tested, so only the bindings are tested here_
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha512(plain) == "27864cc5219a951a7a6e52b8c8dddf6981d098da1658d96258c870b2c88dfbcb51841aea172a28bafa6a79731165584677066045c959ed0f9929688d04defc29");
test_expect(sys.hash.sha512("1234567890") == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
test_expect(sys.hash.sha512(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "50540bc4ae31875fceb3829434c55e3c2b66ddd7227a883a3b4cc8f6cda965ad1712b3ee0008f9cee08da93f5234c1a7bf0e2570ef56d65280ffea691b953efe");
path = fs.tmpdir() + fs.directoryseparator + "jstestsha512.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha512(path, true) == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
fs.unlink(path);
