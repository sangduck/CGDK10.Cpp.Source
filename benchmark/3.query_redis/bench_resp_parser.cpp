#include "cgdk/sdk10/server.query.redis.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
using namespace CGDK;
bool do_process_resp_parsing(resp_data&, shared_buffer&);
int main(int argc,char**argv) {
 const std::string kind=argc>1?argv[1]:"text";
 const std::size_t size=argc>2?std::strtoull(argv[2],nullptr,10):4096;
 std::string wire;
 if(kind=="bulk") wire="$"+std::to_string(size)+"\r\n";
 else wire=kind=="error"?"-":"+";
 wire+=std::string(size,'x'); wire+="\r\n";
 auto frame=alloc_shared_buffer(wire.size()); frame.append(wire.size(),wire.data());
 using clock=std::chrono::steady_clock;
 const auto begin=clock::now(); std::uint64_t count=0,sum=0;
 do {
  for(unsigned i=0;i<1024;++i) {
   auto input=frame; resp_data reply(static_cast<eRESP_DATA_TYPE>(wire[0]));
   if(!do_process_resp_parsing(reply,input) || input.size()!=0 || reply.str_value.size()!=size) return 2;
   sum+=reply.str_value.size(); ++count;
  }
 } while(clock::now()-begin<std::chrono::milliseconds(100));
 const auto seconds=std::chrono::duration<double>(clock::now()-begin).count();
 std::printf("{\"type\":\"%s\",\"size\":%zu,\"ns_per_reply\":%.3f,\"replies_per_s\":%.0f,\"checksum\":%llu}\n",kind.c_str(),size,seconds*1e9/count,count/seconds,(unsigned long long)sum);
}