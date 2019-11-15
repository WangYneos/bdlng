
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include"aux.h"
#include<cstdio>
#include<list>
#include<forward_list>
#include<string>
#include<unordered_map>
#include"../cmdhelper.h"
#include<vector>
#include<Loader.h>
#include<MC.h>
#include"seral.hpp"
#include"base.h"
#include<unistd.h>
#include<cstdarg>

#include"base.h"
#include<cmath>
#include<deque>
#include<dlfcn.h>
#include<string>
#include<aio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "gui.h"
using std::string;
using std::unordered_map;
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define dbg_printf(...) {}
//#define dbg_printf printf
extern "C" {
    BDL_EXPORT void gui_init(std::list<string>& modlist);
}
extern void load_helper(std::list<string>& modlist);
using namespace rapidjson;
using std::vector;
using std::unordered_map;
unordered_map<string,BaseForm*> player_forms;

BDL_EXPORT void sendStr(ServerPlayer& sp,string& fm,int id){
    MyPkt xd(100,[&](void* x,BinaryStream& b)->void{
        b.writeUnsignedVarInt(id);
        b.writeUnsignedVarInt(fm.size());
        b.write(fm.data(),fm.size());
    });
    sp.sendNetworkPacket(*(Packet*)&xd);
}
int autoid;
BDL_EXPORT void sendForm(ServerPlayer& sp,BaseForm* fm){
    auto x=fm->getstr();
    fm->setID(++autoid);
    if(player_forms.count(sp.getName())){
        player_forms.erase(sp.getName());
    }
    player_forms[sp.getName()]=fm;
    sendStr(sp,x,fm->getid());
}
THook(void*,_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK23ModalFormResponsePacket,ServerNetworkHandler* sh,NetworkIdentifier const& iden,Packet* pk){
     ServerPlayer* p=sh->_getServerPlayer(iden,pk->getClientSubId());
    if(p){
       // printf("handle %d [%s]\n",access(pk,int,36),access(pk,string,40).c_str());
        if(player_forms.count(p->getName())){
            player_forms[p->getName()]->process(access(pk,string,40));
            delete player_forms[p->getName()];
            player_forms.erase(p->getName());
        }
    }
    return nullptr;
}
static void oncmd(std::vector<string>& a,CommandOrigin const & b,CommandOutput &outp) {
    ServerPlayer& sp=*(ServerPlayer*)b.getEntity();
    //Form* x=new Form([](const string& x)->void{});
    //x->setContent("nmsl")->setTitle("kksk")->addButton("nm","sl")->setID(114514);
    //sendForm(sp,x);
            SimpleInput* xx=new SimpleInput("ms",[](const string& x)->void{printf("get %s\n",x.c_str());});
            xx->setID(111);
            xx->addInput("nmsl!");
            sendForm(sp,xx);
}
void gui_init(std::list<string>& modlist) {
    register_cmd("gui",fp(oncmd),"gui handler");
    printf("[GUI] Loaded\n");
    load_helper(modlist);
}