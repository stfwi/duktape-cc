/**
 * @test-aux sys-exec
 * Main script of auxiliary binary to test process
 * execution using sys.exec, sys.process, sys.shell, etc.
 * taux.cc implicitly calls `function command_<CLI-ARG0>(CLI_ARGS...){}`
 */

function command_none()
{
  print("STDOUT text");
  alert("STDERR text");
  return 0;
}

function command_echo_stdin()
{
  print(console.read().replace(/[\r\n]+/g,"\n").replace(/[\r\n\s]+$/,""));
  return 0;
}

function command_print_env()
{
  for(var key in sys.env) {
    print(key+"="+sys.env[key]);
  }
  return 0;
}

function command_print_env_keys()
{
  print(JSON.stringify(Object.keys(sys.env)));
  return 0;
}

function command_child_process_print10x50ms()
{
  for(var i=0; i<10; ++i) {
    print(i);
    sys.sleep(50e-3);
  }
  return 0;
}

function command_child_process_reflect_stdin()
{
  var stdin_line = "";
  const deadline = sys.clock() + 500e-3;
  while(sys.clock() < deadline) {
    stdin_line = console.readline().replace(/[\r]+/g,"");
    print(stdin_line);
    alert(stdin_line);
    if(stdin_line.replace(/[^\w]/g, "") == "exit") return 0;
  }
  return 1;
}
