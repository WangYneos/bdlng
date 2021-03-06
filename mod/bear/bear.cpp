
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
using std::string;
using std::unordered_map;
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define dbg_printf(...) {}
//#define dbg_printf printf
extern "C" {
    BDL_EXPORT void bear_init(std::list<string>& modlist);
}
extern void load_helper(std::list<string>& modlist);
static unordered_map<string,int> banlist;
static unordered_map<string,string> xuid_name;
static void save() {
    char* bf;
    int sz=maptomem(banlist,&bf,h_str2str,h_int2str);
    mem2file("ban.db",bf,sz);
}
static void load() {
    char* buf;
    int sz;
    struct stat tmp;
    if(stat("ban.db",&tmp)==-1) {
        save();
    }
    file2mem("ban.db",&buf,sz);
    memtomap(banlist,buf,h_str2str_load,h_str2int);
}
static void save2() {
    char* bf;
    int sz=maptomem(xuid_name,&bf,h_str2str,h_str2str);
    mem2file("banxuid.db",bf,sz);
}
static void load2() {
    char* buf;
    int sz;
    struct stat tmp;
    if(stat("banxuid.db",&tmp)==-1) {
        save2();
    }
    file2mem("banxuid.db",&buf,sz);
    memtomap(xuid_name,buf,h_str2str_load,h_str2str_load);
}
bool isBanned(const string& name) {
    if(banlist.count(name)) {
        int tm=banlist[name];
        if(tm==0) return 1;
        if(time(0)>tm) {
            banlist.erase(name);
            save();
            return 0;
        }
        return 1;
    }
    return 0;
}

static int logfd;
static int logsz;
static void initlog() {
    logfd=open("player.log",O_WRONLY|O_APPEND|O_CREAT,S_IRWXU);
    logsz=lseek(logfd,0,SEEK_END);
}
static void async_log(const char* fmt,...) {
    char buf[10240];
    auto x=time(0);
    va_list vl;
    va_start(vl,fmt);
    auto tim=strftime(buf,128,"[%Y-%m-%d %H:%M:%S] ",localtime(&x));
    int s=vsprintf(buf+tim,fmt,vl)+tim;
    write(1,buf,s);
    write(logfd,buf,s);
    va_end(vl);
}

THook(void*,_ZN20ServerNetworkHandler22_onClientAuthenticatedERK17NetworkIdentifierRK11Certificate,u64 t,NetworkIdentifier& a, Certificate& b){
    string pn=ExtendedCertificate::getIdentityName(b);
    string xuid=ExtendedCertificate::getXuid(b);
    async_log("[JOIN]%s joined game with xuid <%s>\n",pn.c_str(),xuid.c_str());
    if(!xuid_name.count(xuid)){
        xuid_name[xuid]=pn;
        save2();
    }
    if(isBanned(pn) || (xuid_name.count(xuid) && isBanned(xuid_name[xuid]))) {
        string ban("§c你在当前服务器的黑名单内!");
        getMC()->getNetEventCallback()->disconnectClient(a,ban,false);
        return nullptr;
    }
    return original(t,a,b);
}

static bool hkc(ServerPlayer const * b,string& c) {
    c="xxoo";
    async_log("[CHAT]%s: %s\n",b->getName().c_str(),c.c_str());
    return 1;
}

#include <sys/socket.h>
#include <arpa/inet.h>
ssize_t (*rori)(int socket, void * buffer, size_t length,
                int flags, struct sockaddr * address,
                socklen_t * address_len);
extern "C" ssize_t recvfrom_hook(int socket, void * buffer, size_t length,
                                 int flags, struct sockaddr * address,
                                 socklen_t * address_len) {
    int rt=rori(socket,buffer,length,flags,address,address_len);
    if(rt && ((char*)buffer)[0]==0x5) {
        char buf[1024];
        inet_ntop(AF_INET,&(((sockaddr_in*)address)->sin_addr),buf,*address_len);
        async_log("[NETWORK] %s send conn\n",buf);
    }
    return rt;
}
static void oncmd(std::vector<string>& a,CommandOrigin const & b,CommandOutput &outp) {
    ARGSZ(1)
    if((int)b.getPermissionsLevel()>0) {
        banlist[a[0]]=a.size()==1?0:(time(0)+atoi(a[1].c_str()));
        runcmd(string("kick \"")+a[0]+"\" §c你号没了");
        save();
        auto x=getplayer_byname(a[0]);
        if(x){
            xuid_name[x->getXUID()]=x->getName();
            save2();
        }
        outp.success("§e玩家已封禁: "+a[0]);
    }
}
static void oncmd2(std::vector<string>& a,CommandOrigin const & b,CommandOutput &outp) {
    ARGSZ(1)
    if((int)b.getPermissionsLevel()>0) {
        banlist.erase(a[0]);
        save();
        outp.success("§e玩家已解封: "+a[0]);
    }
}
//add custom
using std::unordered_set;
unordered_set<string> banitems,warnitems;
static bool handle_u(GameMode* a0,ItemStack * a1,BlockPos const* a2,BlockPos const* dstPos,Block const* a5) {
    if(a0->getPlayer()->getPlayerPermissionLevel()>1) return 1;
    string sn=a0->getPlayer()->getName();
    char buf[1024];
    strcpy(buf,a1->toString().c_str());
    for(int i=0; buf[i]; ++i){
        buf[i]=tolower(buf[i]);
    }
    string sbuf(buf,strlen(buf));
    //#define check(a) strstr(buf,a)!=NULL
    //if(check("barrier") || check("bedrock") || check("spawn")) {
    for(auto& i:banitems){
        if(sbuf.find(i)!=string::npos){
        async_log("[ITEM] %s 使用高危物品(banned) %s pos: %d %d %d\n",sn.c_str(),buf,a2->x,a2->y,a2->z);
        sendText2(a0->getPlayer(),"§c无法使用违禁物品");
        return 0;
        }
    }
    for(auto& i:warnitems){
        if(sbuf.find(i)!=string::npos){
        async_log("[ITEM] %s 使用危险物品(warn) %s pos: %d %d %d\n",sn.c_str(),buf,a2->x,a2->y,a2->z);
        return 1;
        }
    }
    //if(check("tnt") || check("lava")) {
    return 1;
}
static void handle_left(ServerPlayer* a1){
	async_log("[LEFT] %s left game\n",a1->getName().c_str());
}
#include"rapidjson/document.h"

int FPushBlock,FExpOrb,FDest;
enum CheatType{
    FLY,NOCLIP,INV,MOVE
};
void notifyCheat(const string& name,CheatType x){
    const char* CName[]={"FLY","NOCLIP","Creative","Teleport"};
    async_log("[%s] detected for %s\n",CName[x],name.c_str());
    string kick=string("kick \"")+name+"\" §c你号没了";
    switch(x){
        case FLY:
            runcmd(kick);
        break;
        case NOCLIP:
            runcmd(kick);
        break;
        case INV:
            runcmd(kick);
        break;
        case MOVE:
            runcmd(kick);
        break;
    }
}

THook(void*,_ZN5BlockC2EtR7WeakPtrI11BlockLegacyE,Block* a,unsigned short x,void* c){
	auto ret=original(a,x,c);
    if(FPushBlock){
	    auto* leg=a->getLegacyBlock();
	    if(a->isContainerBlock()) leg->addBlockProperty({0x1000000LL});
    }
    return ret;
}

unordered_map<string,clock_t> lastchat;
int FChatLimit;
bool ChatLimit(ServerPlayer* p){
    if(!FChatLimit || p->getPlayerPermissionLevel()>1) return true;
    auto last=lastchat.find(p->getRealNameTag());
    if(last!=lastchat.end()){
    auto old=last->second;
    auto now=clock();
    last->second=now;
    if(now-old<CLOCKS_PER_SEC*0.25) return false;
    return true;
    }else{
        lastchat.insert({p->getRealNameTag(),clock()});
        return true;
    }
}

THook(void*,_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK10TextPacket,ServerNetworkHandler* sh,NetworkIdentifier const& iden,Packet* pk){
    ServerPlayer* p=sh->_getServerPlayer(iden,pk->getClientSubId());
    if(p){
        if(ChatLimit(p))
        return original(sh,iden,pk);
    }
    return nullptr;
}
THook(void*,_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK20CommandRequestPacket,ServerNetworkHandler* sh,NetworkIdentifier const& iden,Packet* pk){
    ServerPlayer* p=sh->_getServerPlayer(iden,pk->getClientSubId());
    if(p){
        if(ChatLimit(p))
        return original(sh,iden,pk);
    }
    return nullptr;
}


THook(void*,_ZN20ServerNetworkHandler6handleERK17NetworkIdentifierRK24SpawnExperienceOrbPacket,ServerNetworkHandler* sh,NetworkIdentifier const& iden,Packet* pk){
    if(FExpOrb){
	    return nullptr;
    }
    return original(sh,iden,pk);
}

THook(void*,_ZNK15StartGamePacket5writeER12BinaryStream,void* this_,void* a){
    //dirty patch to hide seed
    access(this_,uint,40)=114514;
    return original(this_,a);
}

int limitLevel(int input, int max) {
  if (input < 0)
    return 0;
  else if (input > max)
    return 0;
  return input;
}
struct Enchant {
  enum Type : int {};
  static std::vector<std::unique_ptr<Enchant>> mEnchants;
  virtual ~Enchant();
  virtual bool isCompatibleWith(Enchant::Type type);
  virtual int getMinCost(int);
  virtual int getMaxCost(int);
  virtual int getMinLevel();
  virtual int getMaxLevel();
  virtual bool canEnchant(int) const;
};

struct EnchantmentInstance {
  enum Enchant::Type type;
  int level;
  int getEnchantType() const;
  int getEnchantLevel() const;
  void setEnchantLevel(int);
};
//将外挂附魔无效
THook(int, _ZNK19EnchantmentInstance15getEnchantLevelEv, EnchantmentInstance* thi) {
  int level2 =  thi->level;
  auto &enchant = Enchant::mEnchants[thi->type];
  auto max      = enchant->getMaxLevel();
  auto result   = limitLevel(level2, max);
  if (result != level2) thi->level = result;
  return result;
}
typedef unsigned long IHash;
IHash MAKE_IHASH(ItemStack* a){
    return  (((unsigned long)a->getUserData()->hash())&0xffffffffffff0000ull) ^ a->getIdAuxEnchanted();
}
struct VirtInv{
    unordered_map<IHash,int> items;
    bool bad;
    VirtInv(Player& p){
        bad=0;
    }
    void setBad(){
        bad=1;
    }
    void addItem(IHash item,int count){
        if(item==0) return;
        int prev=items[item];
        items[item]=prev+count;
    }
    void takeItem(IHash item,int count){
        addItem(item,-count);
    }
    bool checkup(){
        if(bad) return 1;
        for(auto &i:items){
            if(i.second<0){
                return false;
            }
        }
        return true;
    }
    void clear(){
        items.clear();
        bad=false;
    }
};
/*
unordered_map<string,VirtInv*> player_inv;
THook(void*,_ZN27InventoryTransactionManager17addExpectedActionERK15InventoryAction,InventoryTransactionManager* itm,InventoryAction& b){
    const string& name=itm->getPlayer()->getRealNameTag();
    auto& x=b.getSource();
    auto id1=x.getType();
    auto id2=x.getFlags();
    auto id3=x.getContainerId();
    printf("wtf %d %d %d %s %s\n",id1,id2,id3,b.getFromItem()->toString().c_str(),b.getToItem()->toString().c_str());
    if((id1==0 && id3==119) && b.getToItem()->getId()!=0){
        //move item to offhand
        if(!b.getToItem()->isOffhandItem()){
            VirtInv* pinv=player_inv.count(name)?player_inv[name]:(player_inv[name]=new VirtInv(*itm->getPlayer()));
            pinv->setBad();
        }
    }else{
        if(id1==100 && id2==0){
            //from virtual inv,need to sanitize fromItem!
            VirtInv* pinv=player_inv.count(name)?player_inv[name]:(player_inv[name]=new VirtInv(*itm->getPlayer()));
            auto x=b.getFromItem();
            auto y=b.getToItem();
            pinv->takeItem(MAKE_IHASH(x),x->getStackSize());
            pinv->addItem(MAKE_IHASH(y),y->getStackSize());
        }
    }
    return original(itm,b);
}
THook(void*,_ZNK20InventoryTransaction11executeFullER6Playerb,void* _thi,Player &player, bool b){
    const string& name=player.getRealNameTag();
    VirtInv* pinv=player_inv.count(name)?player_inv[name]:(player_inv[name]=new VirtInv(player));
    if(!pinv->checkup()){
        notifyCheat(name,INV);
        pinv->clear();
        return nullptr;
    }
    //pinv->clear();
    return original(_thi,player,b);
}
*/
char buf[1024*1024];
using namespace rapidjson;
void load_config(){
    banitems.clear();warnitems.clear();
    int fd=open("config/bear.json",O_RDONLY);
    if(fd==-1){
        printf("[BEAR] Cannot load config file!check your configs folder\n");
        exit(0);
    }
    buf[read(fd,buf,1024*1024-1)]=0;
    close(fd);
    Document d;
    if(d.ParseInsitu(buf).HasParseError()){
        printf("[ANTIBEAR] JSON ERROR!\n");
        exit(1);
    }
    FPushBlock=d["FPushChest"].GetBool();
    FExpOrb=d["FSpwanExp"].GetBool();
    FDest=d["FDestroyCheck"].GetBool();
    FChatLimit=d["FChatLimit"].GetBool();
    string x=d["banitems"].GetString();
    using std::vector;
    vector<string> ite;
    split_string(x,ite,",");
    for(auto& i:ite){
        printf("[ANTIBEAR] adding banitem %s\n",i.c_str());
        banitems.insert(i);
    }
    x=d["warnitems"].GetString();
    ite.clear();
    split_string(x,ite,",");
    for(auto& i:ite){
        warnitems.insert(i);
    }
}
string lastn;
clock_t lastcl;
int fd_count;
#include<ctime>
static int handle_dest(GameMode* a0,BlockPos const& a1,unsigned char a2) {
    if(!FDest) return 1;
    int pl=a0->getPlayer()->getPlayerPermissionLevel();
    if(pl>1 || a0->getPlayer()->isCreative()) {
        return 1;
    }
    const string& name=a0->getPlayer()->getName();
    int x(a1.x),y(a1.y),z(a1.z); 
    Block& bk=*a0->getPlayer()->getBlockSource()->getBlock(a1);
    int id=bk.getLegacyBlock()->getBlockItemId();
    if(id==7 || id==416){
        return 0;
    }
    if(name==lastn && clock()-lastcl<CLOCKS_PER_SEC*(1.0/100)/*10ms*/) {
        lastcl=clock();
        fd_count++; 
        if(fd_count>=5){
            fd_count=0;
            return 0;
        }
        return 1;
    }
    lastn=name;
    lastcl=clock();
    fd_count=0;
    const Vec3& fk=a0->getPlayer()->getPos();
#define abs(x) ((x)<0?-(x):(x))
    int dx=fk.x-x;
    int dy=fk.y-y;
    int dz=fk.z-z;
    int d2=dx*dx+dy*dy+dz*dz;
    if(d2>45) {
        return 0;
    }
    return 1;
}
void bear_init(std::list<string>& modlist) {
    load();
    load2();
    initlog();
    register_cmd("ban",fp(oncmd),"封禁玩家",1);
    register_cmd("unban",fp(oncmd2),"解除封禁",1);
    register_cmd("reloadbear",fp(load_config),"Reload Configs for antibear",1);
    reg_useitemon(handle_u);
    reg_player_left(handle_left);
    reg_chat(hkc);
    load_config();
    rori=(typeof(rori))(MyHook(fp(recvfrom),fp(recvfrom_hook)));
    printf("[ANTI-BEAR] Loaded\n");
    load_helper(modlist);
}

