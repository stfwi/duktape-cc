// note: crc16 (USB) c++ code already tested, so only the bindings are tested here.
var plain = Uint8Array.allocPlain(3);
plain[0] = 0x01; plain[1] = 0x02; plain[2] = 0x03;
test_note("sys.hash.crc16([1,2,3])", sprintf("0x%04x", sys.hash.crc16(plain)));
test_expect(sys.hash.crc16(plain) == 0x9E9E);
test_expect(sys.hash.crc16("test crc 16") == 0xF3B8);
test_expect(sys.hash.crc16(new Uint8Array([0x01,0x02,0x03,0x04,0x05])) == 0x44D5);
