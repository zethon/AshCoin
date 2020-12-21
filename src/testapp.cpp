#include <string.h>
#include <unistd.h>          
#include <errno.h>           
#include <sys/types.h>       
#include <sys/socket.h>      
#include <sys/ioctl.h>  
#include <sys/resource.h>    
#include <sys/utsname.h>       
#include <netdb.h>           
#include <netinet/in.h>      
#include <netinet/in_systm.h>                 
#include <netinet/ip.h>      
#include <netinet/ip_icmp.h> 
#include <assert.h>
#include <iostream>


#define DARWIN


#include <net/if_dl.h>       
#include <ifaddrs.h>         
#include <net/if_types.h>    

const char* getMachineName() 
{ 
   static struct utsname u;  

   if ( uname( &u ) < 0 )    
   {       
      assert(0);             
      return "unknown";      
   }       

   return u.nodename;        
}   


//---------------------------------get MAC addresses ---------------------
// ---------------unsigned short-unsigned short----------        
// we just need this for purposes of unique machine id. So any one or two mac's is fine.            
unsigned short hashMacAddress( unsigned char* mac )                 
{ 
   unsigned short hash = 0;             

   for ( unsigned int i = 0; i < 6; i++ )              
   {       
      hash += ( mac[i] << (( i & 1 ) * 8 ));           
   }       
   return hash;              
} 


void print_hex(const char* p, std::size_t size)
{
    std::string_view input(p, size);

    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = static_cast<const unsigned char>(input[i]);
        // output.append("0x");
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
        output.append(" ");
    }

    if (output.size() > 0) output.pop_back();
    std::cout << "HEX (" << size<< "): " << output << std::endl;
}


void getMacHash( unsigned short& mac1, unsigned short& mac2 )       
{ 
   mac1 = 0;                 
   mac2 = 0;                 

   struct ifaddrs* ifaphead; 
   if ( getifaddrs( &ifaphead ) != 0 )        
      return;                

   // iterate over the net interfaces         
   bool foundMac1 = false;   
   struct ifaddrs* ifap;     
   for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next )                  
   {       
      struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;     
      if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER ))                 
      {    
          if ( !foundMac1 )  
          {                  
             foundMac1 = true;                
             mac1 = hashMacAddress( (unsigned char*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );       
          } else {           
             mac2 = hashMacAddress( (unsigned char*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );       
            //  break;          
          }

        auto temp = (unsigned char*)(LLADDR(sdl));
        // std::cout << "temp hex: " << std::hex << temp << '\n';
        print_hex((const char*)temp, 32);
      }    
   }       

   freeifaddrs( ifaphead );  

   // sort the mac addresses. We don't want to invalidate                
   // both macs if they just change order.    
   if ( mac1 > mac2 )        
   {       
      unsigned short tmp = mac2;        
      mac2 = mac1;           
      mac1 = tmp;            
   }       
} 

unsigned short getVolumeHash()          
{ 
   // we don't have a 'volume serial number' like on windows. Lets hash the system name instead.    
   unsigned char* sysname = (unsigned char*)getMachineName();       
   unsigned short hash = 0;             

   for ( unsigned int i = 0; sysname[i]; i++ )         
      hash += ( sysname[i] << (( i & 1 ) * 8 ));       

   return hash;              
} 

 #include <mach-o/arch.h>    
 unsigned short getCpuHash()            
 {         
     const NXArchInfo* info = NXGetLocalArchInfo();    
     unsigned short val = 0;            
     val += (unsigned short)info->cputype;               
     val += (unsigned short)info->cpusubtype;            
     return val;             
 }         

       

int main()
{
    std::cout << "Machine: " << getMachineName() << '\n';
    std::cout << "CPU    : " << getCpuHash() << '\n';
    std::cout << "Volume : " << getVolumeHash() << '\n';

    unsigned short x1, x2;
    getMacHash(x1, x2);
    std::cout << "MacHash: " << std::to_string(x1) << ':' << std::to_string(x2) << '\n';

  return 0;
}    