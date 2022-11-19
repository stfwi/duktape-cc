
const is_linux = sys.uname().sysname.search(/linux/i)===0;

const settings = {
  baudrates: [ 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200 ],
  optional_baudrates: [ 50, 75, 110, 134, 150, 200, 300, 600, 7200, 14400, 28800, 76800, 230400, 460800, 500000, 576000, 921600, 1000000, 1152000, 1500000, 2000000, 2500000, 3000000, 3500000, 4000000 ],
  databits: [7, 8],
  stopbits: [1, 1.5, 2],
  parities: ["n", "o", "e"],
  flowcontrols: ["none", "xonxoff", "rtscts"]
};

if(sys.serialport === undefined) {
  test_note("Skipping test, sys.mmap() not compiled in.");
  exit(0);
}

function check_ports_and_matching()
{
  // Port list and port to be used
  const port_list = test_expect_noexcept( sys.serialport.portlist() );
  test_comment("Port list" + JSON.stringify(port_list));
  test_expect( typeof(port_list) == 'object' );
  const first_port = (function(){ const p=Object.keys(port_list); return p.length>0 ? p.shift() : ""})();
  const first_port_path = (function(){ return port_list[first_port] || "" })();
  if(first_port != "") settings.port = {name:first_port, path:first_port_path};
  test_comment("First port in the list: '" + first_port + "' (path/w32com: "+ first_port_path +")");
  // Non-strict device/path matching
  test_expect_noexcept( new sys.serialport() ); // default settings, no port.
  test_expect_noexcept( new sys.serialport(first_port.toUpperCase()) ); // variant 1 (may not match win32 COM###)
  test_expect_noexcept( new sys.serialport(first_port.toLowerCase()) ); // variant 2 (should match linux and win32)
  test_expect_noexcept( new sys.serialport(first_port_path.toUpperCase()) ); // variant 1 (may not match win32 COM###)
  test_expect_noexcept( new sys.serialport(first_port_path.toLowerCase()) ); // variant 2 (should match linux and win32)
  test_expect_except( new sys.serialport({not:"a string"}) ); // port has to be a string.
  test_warn("No port found to do further testing with.");
  test_gc();
  return (first_port!="");
}

function check_basic_setters_getters(port_name, port_path)
{
  const tty = test_expect_noexcept( new sys.serialport(port_name+",9600n81") );
  if(!tty) { test_warn("Skipping further checks, as construction failed."); return; }

  // Write-readback
  {
    test_comment("tty.port = " + tty.port)
    test_expect( tty.port == port_path );
    test_comment("tty.baudrate = " + tty.baudrate)
    test_expect( tty.baudrate == 9600 );
    test_comment("tty.databits = " + tty.databits)
    test_expect( tty.databits == 8 );
    test_comment("tty.stopbits = " + tty.stopbits)
    test_expect( tty.stopbits == 1 );
    test_comment("tty.parity = " + tty.parity)
    test_expect( tty.parity == 'n' );
    test_comment("tty.timeout = " + tty.timeout)
    test_expect( typeof(tty.timeout) == 'number' );
    test_comment("tty.settings = " + tty.settings)
    test_expect( tty.settings.toLowerCase() == (port_path+',9600n81,timeout:'+tty.timeout+"ms").toLowerCase());
    test_comment("tty.txnewline = " + JSON.stringify(tty.txnewline))
    test_expect( tty.txnewline.search(/[\r\n]/) === 0 );
    test_comment("tty.rxnewline = " + JSON.stringify(tty.rxnewline))
    test_expect( tty.rxnewline == "" );
    test_comment("tty.closed = " + tty.closed)
    test_expect( tty.closed );
    test_comment("tty.isopen = " + tty.isopen)
    test_expect( !tty.isopen );
    test_comment("tty.error = " + tty.error)
    test_expect( !tty.error );
    test_comment("tty.errormessage = " + tty.errormessage)
    test_expect( tty.errormessage == "");
    test_comment("tty.flowcontrol = " + tty.flowcontrol)
    test_expect( tty.flowcontrol == "none");
    test_comment("tty.rts = " + tty.rts)
    test_expect( tty.rts !== undefined );
    test_expect_noexcept( tty.rts = false );
    test_comment("tty.cts = " + tty.cts)
    test_expect( tty.cts !== undefined );
    test_expect_except( tty.cts = false );
    test_comment("tty.dtr = " + tty.dtr)
    test_expect( tty.dtr !== undefined );
    test_expect_noexcept( tty.dtr = false );
    test_comment("tty.dsr = " + tty.dsr)
    test_expect( tty.dsr !== undefined );
    test_expect_except( tty.dsr = false);

    test_expect_noexcept( tty.port = port_name );
    test_expect( tty.port == port_name );
    settings.baudrates.filter(function(br){
      test_expect_noexcept( tty.baudrate = br );
      test_expect( tty.baudrate == br );
    });
    test_expect_noexcept( tty.baudrate = 9600 );
    settings.databits.filter(function(db){
      test_expect_noexcept( tty.databits = db );
      test_expect( tty.databits == db );
      test_note( tty.settings );
    });
    test_expect_noexcept( tty.databits = 8 );
    settings.stopbits.filter(function(sb){
      test_expect_noexcept( tty.stopbits = sb );
      test_expect( tty.stopbits == sb );
      test_note( tty.settings );
    });
    test_expect_noexcept( tty.stopbits = 1 );
    settings.parities.filter(function(pa){
      test_expect_noexcept( tty.parity = pa );
      test_expect( tty.parity == pa );
      test_note( tty.settings );
    });
    test_expect_noexcept( tty.parity = 'n' );
    settings.flowcontrols.filter(function(fc){
      test_expect_noexcept( tty.flowcontrol = fc );
      test_expect( tty.flowcontrol == fc );
      test_note( tty.settings );
    });
    test_expect_noexcept( tty.flowcontrol = "none" );
    test_expect_noexcept( tty.timeout = 10 );
    test_expect( tty.timeout == 10 );
    test_expect_noexcept( tty.timeout = -5 );
    test_expect( tty.timeout == 0 );
    test_note( tty.settings );

    test_expect_noexcept( tty.txnewline = "" );
    test_expect( tty.txnewline == "" );
    test_expect_noexcept( tty.txnewline = "\r" );
    test_expect( tty.txnewline == "\r" );
    test_expect_noexcept( tty.txnewline = "\n" );
    test_expect( tty.txnewline == "\n" );

    test_expect_noexcept( tty.rxnewline = "\r" );
    test_expect( tty.rxnewline == "\r" );
    test_expect_noexcept( tty.rxnewline = "\n" );
    test_expect( tty.rxnewline == "\n" );
    test_expect_noexcept( tty.rxnewline = "" );
    test_expect( tty.rxnewline == "" );

    // Invalid
    test_expect_except( tty.stopbits = 0 );
    test_expect_except( tty.stopbits = 3 );
    test_expect_except( tty.databits = 1 );
    test_expect_except( tty.parity = "uneven" );
    test_expect_except( tty.parity = "m" ); // mark not supported yet.
    test_expect_except( tty.flowcontrol = "any" );
    test_expect_except( tty.rxnewline = ["not a string"] );
    test_expect_except( tty.txnewline = ["not a string"] );
  }

  // Settings
  {
    const settigns_equal = function(a, b) {
      const pp = port_path.toLowerCase();
      const pn = port_name.toLowerCase();
      a = a.toLowerCase();
      b = b.toLowerCase();
      if(a.search(pp) === 0) a = a.replace(pp, pn, 1);
      if(b.search(pp) === 0) b = b.replace(pp, pn, 1);
      if(a !== b) {
        test_note("Settings mismatch: '"+a+"' vs '"+b+"'");
      } else {
        test_note("Settings Match: '"+a+"' vs '"+b+"'");
      }
      return (a===b);
    };

    // Invalid
    if(is_linux) {
      test_expect_except( tty.settings = "9600n81" ); // no port
    } else {
      test_warn("@TODO: Settings exception has to be homogenized (win32 and linux)");
    }
    test_expect_except( tty.settings = ",,9600n81" );
    test_expect_except( tty.settings = port_name+",9600s81" );
    test_expect_except( tty.settings = port_name+",9600n91" );
    test_expect_except( tty.settings = port_name+",9600n61" );
    test_expect_except( tty.settings = port_name+",9600n80" );
    test_expect_except( tty.settings = port_name+",9600n83" );
    test_expect_except( tty.settings = port_name+",9600n81,timeout:x" );
    test_expect_except( tty.settings = port_name+",9600n81,timeout:-100" );
    // Empty -> no change
    const settings_before = tty.settings;
    test_expect_noexcept( tty.settings = "" );
    test_expect( tty.settings == settings_before );

    test_expect_noexcept( tty.settings = port_path+",9600n81.5" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81.5,timeout:10ms") );

    // Write-readback
    ["9600","115200"].filter(function(bd){
      ["n","e","o", "N","E","O"].filter(function(pa){
        ["8","7"].filter(function(db){
          ["1","2","1.5"].filter(function(sb){
            [0, 10].filter(function(to){
              const port_settings = port_name+","+bd+pa+db+sb+",timeout:"+to+"ms";
              test_note("Set settings: " + port_settings);
              test_expect_noexcept( tty.settings = port_settings );
              test_note( "tty.settings=" + tty.settings + "  | (ref:<port>,"+port_settings+")");
              test_expect( settigns_equal(tty.settings, port_settings) );
            })
          })
        })
      })
    });

    // Flow control
    test_expect_noexcept( tty.settings = port_path+",9600n81,timeout:10ms,xonxoff" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81,xonxoff,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",9600n81,timeout:10ms,rtscts" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81,rtscts,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",9600n81,timeout:10ms,xon" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81,xonxoff,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",9600n81,timeout:10ms,rts" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81,rtscts,timeout:10ms") );

    // Partial settings
    test_expect_noexcept( tty.settings = port_path+",9600n81,timeout:10ms" );
    test_expect( settigns_equal( tty.settings, port_path+",9600n81,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",115200" );
    test_expect( settigns_equal( tty.settings, port_path+",115200n81,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",115200e7" );
    test_expect( settigns_equal( tty.settings, port_path+",115200e71,timeout:10ms") );
    test_expect_noexcept( tty.settings = port_path+",115200e72" );
    test_expect( settigns_equal( tty.settings, port_path+",115200e72,timeout:10ms") );
  }

  // Responses when closed
  {
    test_expect_noexcept( tty.close() );
    test_expect_noexcept( tty.purge() );
    test_expect_except( tty.open(port_name, {no_string:"115200n81"}) );
    test_expect_except( tty.open({no_string:port_name}) );
    test_expect_except( tty.write() );
    test_expect_except( tty.writeln() );
    test_expect_except( tty.write("write-while-closed") );
    test_expect_except( tty.writeln("write-while-closed") );
    test_expect_except( tty.read() );
    test_expect_except( tty.read(4) );
    test_expect_except( tty.readln() );
    test_expect_except( tty.readln(4) );
    test_expect_except( tty.readln(4, true) ); // ignore empty lines
  }
  test_gc();
}

function check_basic_operation(port_name, port_path)
{
  const tty = test_expect_noexcept( new sys.serialport(port_name+",9600n81") );
  try { tty.open(); } catch(ex) { test_comment("SKIP: Could not open port, maybe no permissions. Error: '" + tty.errormessage + "'"); }
  if(!tty.closed) {
    test_expect_noexcept( tty.open() );
    test_expect_noexcept( tty.open(port_name) );
    test_expect_noexcept( tty.open(port_name, "115200n81") );
    test_expect_noexcept( tty.close() );
    test_expect_noexcept( tty.read() );
    test_expect_noexcept( tty.read(10) );
    test_expect( tty.readln(5) === undefined );
  }
  test_gc();
}


if(check_ports_and_matching()) {
  check_basic_setters_getters(settings.port.name, settings.port.path);
  //check_basic_operation(settings.port.name, settings.port.path);
}

//---------
