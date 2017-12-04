// note: crc8 c++ code already tested, so only the bindings are tested here_
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_expect(sys.hash.crc8(plain) == 0x48);
test_expect(sys.hash.crc8("test crc 8") == 0x7e);
test_expect(sys.hash.crc8(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0xbc);

