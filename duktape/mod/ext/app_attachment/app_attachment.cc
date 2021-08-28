#ifndef APP_ATTACHMENT_PATCHING_BINARY
  #define APP_ATTACHMENT_PATCHING_BINARY
#endif
#include "./app_attachment.hh"
int main(int argc, const char** argv)
{ return sw::util::detail::basic_executable_attachment<>::patch_application(argc, argv); }
