var plain = null;
var path = "";

const own_script_code = fs.read(sys.script);

const expect_except = function(code) {
  try {
    eval(code);
    test_fail("Exception was expected for '"+code+"'.");
  } catch(ex) {
    test_pass("Expected exception thrown for '"+code+"': " + ex.message);
  }
}

// note: crc8 c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.crc8({invalid:"object_argument"});');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.crc8(plain) == 0x48);
test_expect(sys.hash.crc8("test crc 8") == 0x7e);
test_expect(sys.hash.crc8(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0xbc);

// note: crc16 (USB) c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.crc16({invalid:"object_argument"});');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc16([1,2,3])", sprintf("0x%04x", sys.hash.crc16(plain)));
test_expect(sys.hash.crc16(plain) == 0x9E9E);
test_expect(sys.hash.crc16("test crc 16") == 0xF3B8);
test_expect(sys.hash.crc16(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x44D5);

// note: crc32 (CCITT) c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.crc32({invalid:"object_argument"});');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc32([1,2,3])", sprintf("0x%04x", sys.hash.crc32(plain)));
test_expect(sys.hash.crc32(plain) == 0x55BC801D);
test_expect(sys.hash.crc32("test crc 32") == 0x1C2BE2A9);
test_expect(sys.hash.crc32(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x470B99F4);

// note: md5 c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.md5({invalid:"object_argument"});');
expect_except('sys.hash.md5({invalid_path:1}, true);');
expect_except('sys.hash.md5("not-existing.file", true);');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.md5(plain) == "5289df737df57326fcdd22597afb1fac");
test_expect(sys.hash.md5("1234567890") == "e807f1fcf82d132f9bb018ca6738a19f");
test_expect(sys.hash.md5(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "7cfdd07889b3295d6a550914ab35e068");
path = fs.tmpdir() + fs.directoryseparator + "jstestmd5.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.md5(path, true) == "e807f1fcf82d132f9bb018ca6738a19f");
fs.unlink(path);
test_expect( sys.hash.md5(own_script_code) == sys.hash.md5(sys.script, true) );


// note: sha1 c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.sha1({invalid:"object_argument"});');
expect_except('sys.hash.sha1({invalid_path:1}, true);');
expect_except('sys.hash.sha1("not-existing.file", true);');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha1(plain) == "7037807198c22a7d2b0807371d763779a84fdfcf");
test_expect(sys.hash.sha1("1234") == "7110eda4d09e062aa5e4a390b0a572ac0d2c0220");
test_expect(sys.hash.sha1("123456") == "7c4a8d09ca3762af61e59520943dc26494f8941b");
test_expect(sys.hash.sha1("1234567890") == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
test_expect(sys.hash.sha1("123456789012345678901234567890") == "cc84fa5a361f86a589169fde1e4e6d62bc786e6c");
test_expect(sys.hash.sha1("123456789012345678901234567890123456789012345678901234567890") == "245be30091fd392fe191f4bfcec22dcb30a03ae6");
test_expect(sys.hash.sha1("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890") == "ea6fc029b244563f3368a35697954d7cca3110f8");
test_expect(sys.hash.sha1(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "11966ab9c099f8fabefac54c08d5be2bd8c903af");
path = fs.tmpdir() + fs.directoryseparator + "jstestsha1.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha1(path, true) == "01b307acba4f54f55aafc33bb06bbbf6ca803e9a");
fs.unlink(path);
test_expect( sys.hash.sha1(own_script_code) == sys.hash.sha1(sys.script, true) );


// note: sha512 c++ code already tested, so only the bindings are tested here.
expect_except('sys.hash.sha512({invalid:"object_argument"});');
expect_except('sys.hash.sha512({invalid_path:1}, true);');
expect_except('sys.hash.sha512("not-existing.file", true);');
plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.sha512(plain) == "27864cc5219a951a7a6e52b8c8dddf6981d098da1658d96258c870b2c88dfbcb51841aea172a28bafa6a79731165584677066045c959ed0f9929688d04defc29");
test_expect(sys.hash.sha512(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == "50540bc4ae31875fceb3829434c55e3c2b66ddd7227a883a3b4cc8f6cda965ad1712b3ee0008f9cee08da93f5234c1a7bf0e2570ef56d65280ffea691b953efe");
test_expect(sys.hash.sha512("1234") == "d404559f602eab6fd602ac7680dacbfaadd13630335e951f097af3900e9de176b6db28512f2e000b9d04fba5133e8b1c6e8df59db3a8ab9d60be4b97cc9e81db");
test_expect(sys.hash.sha512("123456") == "ba3253876aed6bc22d4a6ff53d8406c6ad864195ed144ab5c87621b6c233b548baeae6956df346ec8c17f5ea10f35ee3cbc514797ed7ddd3145464e2a0bab413");
test_expect(sys.hash.sha512("1234567890") == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
test_expect(sys.hash.sha512("123456789012345678901234567890") == "eba392e2f2094d7ffe55a23dffc29c412abd47057a0823c6c149c9c759423afde56f0eef73ade8f79bc1d16a99cbc5e4995afd8c14adb49410ecd957aecc8d02");
test_expect(sys.hash.sha512("123456789012345678901234567890123456789012345678901234567890") == "c155814e7b9c17236269d981d416569f02993f25bdd25c3aca37e3c031f4dafc9471d1568d55e5a06ce66796216dc5ae1b1c751e478a4e8040baf199f0058ae6");
test_expect(sys.hash.sha512("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890") == "0d9a7df5b6a6ad20da519effda888a7344b6c0c7adcc8e2d504b4af27aaaacd4e7111c713f71769539629463cb58c86136c521b0414a3c0edf7dc6349c6edaf3");
path = fs.tmpdir() + fs.directoryseparator + "jstestsha512.tmp";
fs.writefile(path, "1234567890");
test_expect(sys.hash.sha512(path, true) == "12b03226a6d8be9c6e8cd5e55dc6c7920caaa39df14aab92d5e3ea9340d1c8a4d3d0b8e4314f1f6ef131ba4bf1ceb9186ab87c801af0d5c95b1befb8cedae2b9");
fs.unlink(path);
test_expect( sys.hash.sha512(own_script_code) == sys.hash.sha512(sys.script, true) );
