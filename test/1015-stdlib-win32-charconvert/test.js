

function convert_cp1252_file(path)
{
  test_note("Loading '"+path+"' ...");
  const binary_original = new Uint8Array(fs.read(path, "binary"));
  const text_original = fs.read(path);
  const text_utf8 = text_original.toUTF8(1252);
  const text_cp1252 = text_utf8.fromUTF8(1252);
  const binary_chars = []; for(var i=0; i<binary_original.length; ++i) binary_chars.push(binary_original[i]);
  const text_original_chars = text_original.bytes();
  const utf8_chars = text_utf8.bytes();
  test_note("binary_chars=" + JSON.stringify(binary_chars));
  test_note("text_original_chars=" + JSON.stringify(text_original_chars));
  test_note("utf8_chars=" + JSON.stringify(utf8_chars));
  test_note("binary_original.length=" + binary_original.length);
  test_note("text_original.length=" + text_original.length);
  test_note("text_utf8.length=" + text_utf8.length);
  test_note("text_cp1252.length=" + text_cp1252.length);
  test_expect(text_cp1252===text_original);
  // Against manually checked:
  test_expect(text_cp1252.length == 16);
  test_expect(JSON.stringify(binary_chars)=="[246,246,246,228,228,228,97,97,97,32,99,112,49,50,53,50]");
  test_expect(JSON.stringify(text_original_chars)=="[246,246,246,228,228,228,97,97,97,32,99,112,49,50,53,50]");
  test_expect(JSON.stringify(utf8_chars)=="[195,182,195,182,195,182,195,164,195,164,195,164,97,97,97,32,99,112,49,50,53,50]");
}

function convert_utf8_file(path)
{
  test_note("Loading '"+path+"' ...");
  const binary_original = new Uint8Array(fs.read(path, "binary"));
  const text_original = fs.read(path);
  const text_cp1252 = text_original.fromUTF8(1252);
  const text_utf8 = text_cp1252.toUTF8(1252);
  const binary_chars = []; for(var i=0; i<binary_original.length; ++i) binary_chars.push(binary_original[i]);
  const cp1252_chars = text_cp1252.bytes();
  const utf8_chars = text_utf8.bytes();
  test_note("binary_chars=" + JSON.stringify(binary_chars));
  test_note("cp1252_chars=" + JSON.stringify(cp1252_chars));
  test_note("utf8_chars=" + JSON.stringify(utf8_chars));
  test_note("binary_original.length=" + binary_original.length);
  test_note("text_original.length=" + text_original.length);
  test_note("text_utf8.length=" + text_utf8.length);
  test_note("text_cp1252.length=" + text_cp1252.length);
  test_expect(text_utf8===text_original);
  test_expect(text_cp1252.length == 14);
  test_expect(JSON.stringify(binary_chars)=="[195,182,195,182,195,182,195,164,195,164,195,164,97,97,97,32,117,116,102,56]");
  test_expect(JSON.stringify(utf8_chars)=="[195,182,195,182,195,182,195,164,195,164,195,164,97,97,97,32,117,116,102,56]");
  test_expect(JSON.stringify(cp1252_chars)=="[246,246,246,228,228,228,97,97,97,32,117,116,102,56]");
}

function test_win32_codepage_conversion()
{
  convert_cp1252_file("cp1252.dat");
  convert_utf8_file("utf8.dat");
}

if(sys.uname().sysname === "windows") {
  test_win32_codepage_conversion();
} else {
  test_note("Skipping codepage based string conversion, only on windows.");
}
