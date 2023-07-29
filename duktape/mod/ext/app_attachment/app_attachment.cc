#ifndef APP_ATTACHMENT_PATCHING_BINARY
  #define APP_ATTACHMENT_PATCHING_BINARY
#endif

#include "./app_attachment.hh"
#include <deque>
#include <string>

int main(int argc, const char** argv)
{
  using namespace std;
  auto args = deque<string>();
  for(int i=1; (i<argc) && (argv[i]); ++i) args.push_back(argv[i]);
  const auto verbose = (args.size() > 0) && (args.front() == "-v");
  if(verbose) args.pop_front();
  const auto binary_path = ((args.size() >= 2) && (args[0] == "-p")) ? string(args[1]) : string();
  const auto script_path = ((args.size() == 4) && (args[2] == "-a")) ? string(args[3]) : string();
  auto script_contents = string();
  if(binary_path.empty() || (args.size() & 0x1)) {
    cerr << "Expected -p <path-to-binary> [-a <path-to-attachment>]\n";
    return 1;
  }
  if(!script_path.empty()) {
    std::ifstream fis(script_path.c_str(), std::ios::in|std::ios::binary);
    if(!fis.good()) { cerr << "Failed to open data file: '" << script_path << "'\n"; return 1; }
    script_contents.assign((std::istreambuf_iterator<char>(fis)), std::istreambuf_iterator<char>());
    if(!fis.good() && !fis.eof()) { cerr << "Failed to read data: '" << script_path << "'\n"; return 1; }
    fis.close();
  }
  return sw::util::detail::basic_executable_attachment<>::patch_application(binary_path, verbose, move(script_contents));
}
