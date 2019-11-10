#pragma once
#include<string>
#include<unordered_map>
#include<MC.h>
#define BDL_EXPORT __attribute__((visibility("default")))
class Player;
class Actor;
class Vec3;
//class AutomaticID<Dimension,int>;
class Dimension;
struct MCRESULT;
BDL_EXPORT void reg_attack(void* a);
BDL_EXPORT void reg_pickup(void* a);
BDL_EXPORT void reg_useitem(void* a);
BDL_EXPORT void reg_useitemon(void* a);
BDL_EXPORT void reg_build(void* a);
BDL_EXPORT void reg_destroy(void* a);
BDL_EXPORT void reg_player_join(void* a);
BDL_EXPORT void reg_player_left(void* a);

BDL_EXPORT void sendText(Player* a,string ct);
BDL_EXPORT void TeleportA(Actor& a,Vec3 b,AutomaticID<Dimension,int> c);
BDL_EXPORT void sendTransfer(Player* a,const string& ip,short port);
BDL_EXPORT Player* getplayer_byname(const string& name);
BDL_EXPORT Player* getplayer_byname2(const string& name);
BDL_EXPORT void KillActor(Actor* a);
BDL_EXPORT void sendText2(Player* a,string ct);
BDL_EXPORT MCRESULT runcmd(const string& a);
BDL_EXPORT Minecraft* getMC();

BDL_EXPORT void split_string(const std::string& s, std::vector<std::string>& v, const std::string& c);
BDL_EXPORT bool execute_cmdchain(const string& chain_,ServerPlayer* sp=nullptr,bool chained=true);
BDL_EXPORT void reg_config_reader(void (*readcb)());

#define ARGSZ(b) if(a.size()<b){outp.error("check your arg");return;}

