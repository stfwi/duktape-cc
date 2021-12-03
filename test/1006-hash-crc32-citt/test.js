// note: crc32 (CCITT) c++ code already tested, so only the bindings are tested here.
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc32([1,2,3])", sprintf("0x%04x", sys.hash.crc32(plain)));
test_expect(sys.hash.crc32(plain) == 0x55BC801D);
test_expect(sys.hash.crc32("test crc 32") == 0x1C2BE2A9);
test_expect(sys.hash.crc32(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x470B99F4);
