//
// PROTO-IMPLEMENTATION, THIS IS NOT AN ACTUAL TEST YET.
// @todo: Implement server side/listen, then connect to
// that server.

//
// TCP connect and socket option query only, requires
// peer running e.g. using:
//
//  socat -v tcp6-l:2345,fork,reuseaddr exec:'/bin/cat'
//
function test_socket()
{
  const host_port = "[::1]:2345";
  const test_ping_line = "Hello World ... as always.";
  const sock = new sys.socket();

  const print_settings = function() {
    test_note("sock.sendbuffer_size = " + sock.sendbuffer_size);
    test_note("sock.recvbuffer_size = " + sock.recvbuffer_size);
    test_note("sock.keepalive = " + sock.keepalive);
    test_note("sock.nodelay = " + sock.nodelay);
    test_note("sock.reuseaddress = " + sock.reuseaddress);
    test_note("sock.nonblocking = " + sock.nonblocking);
    test_note("sock.listening = " + sock.listening);
    test_note("sock.timeout = " + sock.timeout);
    test_note("sock.error = " + sock.error);
    test_note("sock.errno = " + sock.errno);
    test_note("sock.address = " + sock.address);
    test_note("sock.socket_id = " + sock.socket_id);
  }

  const test_ping = function(line) {
    if(!sock.connected) return false;
    sock.send(line+ "\n");
    const rx = sock.recv(100);
    return (rx == line+"\n");
  }

  sock.connect(host_port);
  test_expect(sock.connected);
  print_settings();
  test_expect(sock.connected);

  for(var i=0; i<100000; ++i) {
    if(!sock.connected) {
      test_fail("Connection lost");
      break;
    }
    if(!test_ping(test_ping_line)) {
      test_fail("Ping did not match.");
    }
  }

  test_note("Closing ...");
  sock.close();
}

// Test only if -DWITH_SOCKET compiler flag ist set.
if(sys.socket !== undefined) {
  test_socket();
}
