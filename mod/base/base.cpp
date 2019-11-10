#include"../cmdhelper.h"
#include"../myhook.h"
#include<Loader.h>
#include<MC.h>
#include<vector>
#include<seral.hpp>
#include<signal.h>
#include <sys/stat.h>
#include<unistd.h>
using std::list;
using std::vector;
#include"base.h"
#include"hook.hpp"
#include"cmdreg.hpp"
#include"utils.hpp"
#include<sstream>

extern "C" {
    void base_init(list<string>& modlist);
}

//export APIS
void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while(std::string::npos != pos2)
  {
    v.push_back(s.substr(pos1, pos2-pos1));
    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if(pos1 != s.length())
    v.push_back(s.substr(pos1));
}
void execute_cmd_random(const vector<string>& chain){
    int rd=rand()%chain.size();
    runcmd(chain[rd]);
}
bool execute_cmdchain(const string& chain_,ServerPlayer* sp,bool chained){
    string chain=chain_;
    if(sp){
        std::string::size_type pos(0);
        while((pos=chain.find("%name%"))!=std::string::npos){
            chain.replace(pos,pos+6,sp->getName());
        }
    }
    vector<string> dst;
    split_string(chain.substr(chain[0]=='!'),dst,",");
    if(chain[0]=='!'){
        execute_cmd_random(dst);
        return true;
    }
    for(auto& i:dst){
        auto res=runcmd(i);
        if(!res.isSuccess() && chained) return false;
    }
    return true;
}
list<void (*)()> config_reader;
void reg_config_reader(void (*readcb)()){
    config_reader.push_back(readcb);
}
void do_reload(){
    for(auto i:config_reader){
        i();
    }
}
struct TeleportCommand {
    char filler[1024];
    TeleportCommand();
    void teleport(Actor &,Vec3,Vec3*,AutomaticID<Dimension,int>) const;
};
void sendText(Player* a,string ct) {
    TextPacket pk=TextPacket::createRaw(ct);
    ((ServerPlayer*)a)->sendNetworkPacket(pk);
}
void sendText2(Player* a,string ct) {
    TextPacket pk=TextPacket::createJukeboxPopup(ct);
    ((ServerPlayer*)a)->sendNetworkPacket(pk);
}

static TeleportCommand cmd_p;

extern void load_helper(list<string>& modlist);
void TeleportA(Actor& a,Vec3 b,AutomaticID<Dimension,int> c) {
    a.setPos(b);
    cmd_p.teleport(a,b,nullptr,c);
    cmd_p.teleport(a,b,nullptr,c); //fix bugs about chunk sending
}
void KillActor(Actor* a) {
    //dirty workaround
    (*(void ( **)(void*))(*(uintptr_t *)a + 1816LL))(a);
}
Player* getplayer_byname(const string& name) {
    Minecraft* mc=getMC();
    Level* lv=mc->getLevel();
    Player* rt=NULL;
    lv->forEachPlayer([&](Player& tg)->bool{
        if(tg.getRealNameTag()==name) {
            rt=&tg;
            return false;
        }
        return true;
    });
    return rt;
}
#define fcast(a,b) (*((a*)(&b)))
Player* getplayer_byname2(const string& name) {
    Minecraft* mc=getMC();
    Level* lv=mc->getLevel();
    Player* rt=NULL;
    lv->forEachPlayer([&](Player& tg)->bool{
        string bf=tg.getRealNameTag();
#define min(a,b) ((a)<(b)?(a):(b))
        int sz=min(bf.size(),name.size());
        int eq=1;
        for(int i=0; i<sz; ++i) {
            if(tolower(bf[i])!=tolower(name[i])) {
                eq=0;
                break;
            }
        }
        if(eq) {
            rt=&tg;
            return false;
        }
        return true;
    });
    return rt;
}

static Minecraft* MC;
Minecraft* getMC() {
    return MC;
}
THook(void*,_ZN14ServerCommands19setupStandardServerER9MinecraftRKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEES9_P15PermissionsFile,Minecraft& a, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& d, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& b, PermissionsFile* c){
    auto ret=original(a,d,b,c);
    printf("MC %p\n",&a);
    MC=&a;
    return ret;
}
THook(void*,_ZN10SayCommand5setupER15CommandRegistry,CommandRegistry& thi,CommandRegistry& a){
    handle_regcmd(thi);
    return original(thi,a);
}
struct DedicatedServer
{
    void stop();
};
DedicatedServer* dserver;
THook(void*,_ZN15DedicatedServer5startERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE,DedicatedServer* t,string& b){
    printf("starting server %p\n",t);
    dserver=t;
    return original(t,b);
}
void set_int_handler(void* fn);
static void autostop(){
        if(dserver){
            printf("stoping server\n");
            dserver->stop();
        }
}
void base_init(list<string>& modlist)
{
    printf("[MOD/BASE] loaded!\n");    	
    set_int_handler(fp(autostop));
    register_cmd("reload",fp(do_reload));		
    load_helper(modlist);
}
