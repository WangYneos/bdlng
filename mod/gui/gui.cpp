
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
struct BaseForm{
    virtual void setID(int i)=0;
    virtual string getstr()=0;
    virtual int getid()=0;
    virtual void process(const string&)=0;
};
#include<functional>
unordered_map<string,BaseForm*> player_forms;
struct Form:BaseForm{
    int fid;
    Document dc;
    vector<string> labels;
    std::function< void(const string&)> cb;
    Form(std::function< void(const string&)> cbx){
        cb=cbx;
        dc.SetObject();
        dc.AddMember("type","form",dc.GetAllocator());
        dc.AddMember("title","",dc.GetAllocator());
        dc.AddMember("content","",dc.GetAllocator());
        Value x(kArrayType);
        dc.AddMember("buttons",x,dc.GetAllocator());
    }
    void process(const string& d){
        int idx=atoi(d.c_str());
        //printf("%s %d %s\n",d.c_str(),idx,labels[idx].c_str());
        cb(labels[idx]);
    }
    void setID(int i){
        fid=i;
    }
    int getid(){
        return fid;
    }
    string getstr(){
        StringBuffer buf;
        Writer<StringBuffer> writer(buf);
        dc.Accept(writer);
        return string(buf.GetString());
    }
    Form* addButton(const string& text,const string& label){
        Value bt(kObjectType);
        Value ss;
        ss.SetString(text.data(),text.size(),dc.GetAllocator());
        bt.AddMember("text",ss,dc.GetAllocator());
        dc["buttons"].PushBack(bt,dc.GetAllocator());
        labels.push_back(label);
        return this;
    }
    Form* setContent(const string& text){
        dc["content"].SetString(text.c_str(),dc.GetAllocator());
        return this;
    }
    Form* setTitle(const string& text){
        dc["title"].SetString(text.c_str(),dc.GetAllocator());
        return this;
    }
};
struct SimpleInput:BaseForm{
    int fid;
    Document dc;
    vector<string> labels;
    std::function< void(std::unordered_map<string,string>&)> cb;
    SimpleInput(const string& ti,std::function< void(std::unordered_map<string,string>&)> cbx){
        cb=cbx;
        Value ss;
        dc.SetObject();
        ss.SetString(ti.data(),ti.size(),dc.GetAllocator());
        dc.AddMember("type","custom_form",dc.GetAllocator());
        dc.AddMember("title",ss,dc.GetAllocator());
        Value x(kArrayType);
        dc.AddMember("content",x,dc.GetAllocator());
    }
    void process(const string& d){
        dc.Parse(d.c_str());
        if(!dc.IsObject()) return;
        std::unordered_map<string,string> dat;
        for(auto& i:dc.GetObject()){
            printf("[%d %s]\n",i.name.GetInt(),i.value.GetString());
        }
    }
    int getid(){
        return fid;
    }
    void setID(int i){
        fid=i;
    }
    string getstr(){
        StringBuffer buf;
        Writer<StringBuffer> writer(buf);
        dc.Accept(writer);
        return string(buf.GetString());
    }
    Form* addInput(const string& text){
        Value ss;
        ss.SetString(text.data(),text.size(),dc.GetAllocator());
        dc["content"].PushBack(Value(kObjectType)
        .AddMember("type","input",dc.GetAllocator())
        .AddMember("text",ss,dc.GetAllocator())
        .AddMember("placeholder",Value(kNullType),dc.GetAllocator())
        .AddMember("default",Value(kNullType),dc.GetAllocator()),dc.GetAllocator()
        );
        labels.push_back(text);
    }
};
BDL_EXPORT void sendStr(ServerPlayer& sp,string& fm,int id){
    MyPkt xd(100,[&](void* x,BinaryStream& b)->void{
        b.writeUnsignedVarInt(id);
        b.writeUnsignedVarInt(fm.size());
        b.write(fm.data(),fm.size());
    });
    sp.sendNetworkPacket(*(Packet*)&xd);
}
BDL_EXPORT void sendForm(ServerPlayer& sp,BaseForm* fm){
    auto x=fm->getstr();
    if(player_forms.count(sp.getName())){
        player_forms.erase(sp.getName());
    }
    player_forms[sp.getName()]=fm;
    sendStr(sp,x,fm->getid());
}
THook(void*,_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK23ModalFormResponsePacket,ServerNetworkHandler* sh,NetworkIdentifier const& iden,Packet* pk){
     ServerPlayer* p=sh->_getServerPlayer(iden,pk->getClientSubId());
    if(p){
        printf("handle %d [%s]\n",access(pk,int,36),access(pk,string,40).c_str());
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
            SimpleInput* xx=new SimpleInput("ms",[](unordered_map<string,string>&)->void{});
            xx->setID(111);
            xx->addInput("nmsl!");
            sendForm(sp,xx);
}
void gui_init(std::list<string>& modlist) {
    register_cmd("gui",fp(oncmd),"gui handler");
    printf("[GUI] Loaded\n");
    load_helper(modlist);
}
/*
$formData = [
    'type' => 'custom_form',
    'title' => "§l§cREPORT",
    'content' => [],
];
$formData["content"][] = ["type" => "dropdown", "text" => "Players\n", "options" => ["a"]];
$formData["content"][] = ["type" => "dropdown", "text" => "Reason\n", "options" => ["a"]];
$formData["content"][] = ["type" => "input", "text" => "Explain in detail\n"];
echo json_encode($formData);*/