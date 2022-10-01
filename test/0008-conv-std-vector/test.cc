#include "../testenv.hh"
#include <mod/mod.stdio.hh>
#include <sstream>
#include <vector>

using namespace std;

void test(duktape::engine& js)
{
  duktape::api stack(js);
  {
    duktape::stack_guard sg(stack);
    std::vector<int> values{ 1,2,3,4,5,10,100,1000 };
    js.define("array_values", values);

    string s = js.eval<string>("array_values");
    test_comment("Array values: " << s);

    std::vector<int> readback = js.eval<std::vector<int>>("array_values");

    {
      stringstream sss;
      sss << "{";
      if(!readback.empty()) sss << readback.front();
      for(size_t i=1; i<readback.size(); ++i) sss << "," << std::dec << readback[i];
      sss << "}";
      string s = sss.str();
      test_comment("Readback is: " << s);
    }

    test_expect( readback.size() == values.size() );
    if(readback.size() == values.size()) {
      test_expect( std::equal(values.begin(), values.end(), readback.begin()) );
    }
  }
}
