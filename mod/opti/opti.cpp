#include"../myhook.h"
#include<Loader.h>
#include<MC.h>
#include"rapidjson/document.h"
#include<fstream>
typedef uint64_t u64;
typedef uint32_t u32;
typedef int64_t s64;
typedef int32_t s32;
using namespace std;
extern "C" {
    BDL_EXPORT void opti_init(std::list<string>& modlist);
}
extern void load_helper(std::list<string>& modlist);
static void dummy() {

}
extern "C" {void _ZN14NetworkHandler16enableAsyncFlushERK17NetworkIdentifier(long a1,long a2);}
THook(void**,_ZN14NetworkHandler23getEncryptedPeerForUserERK17NetworkIdentifier,void **a1, long a2, long a3){
	a1[0]=a1[1]=nullptr;
	_ZN14NetworkHandler16enableAsyncFlushERK17NetworkIdentifier(a2,a3);
	return a1;
}
#define PAGE_SIZE 0x1000
#define ROUND_PAGE_DOWN(num) ((num) & ~(PAGE_SIZE - 1))
#define ROUND_PAGE_UP(num) (((num) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
void opti_init(std::list<string>& modlist) {
    using namespace rapidjson;
    Document dc;
    ifstream ff;
    ff.open("config/opti.json",ios::in);
    char buf[4096];
    buf[ff.readsome(buf,4096)]=0;
    ff.close();
    if(dc.ParseInsitu(buf).HasParseError()){
        printf("[OPTI] Config JSON ERROR!\n");
        exit(1);
    }
    //0x90f4da4
    //0x1959a64
    //0x000000000779b340+1772+0x195938e-0x16
    float* patch=(float*)(dc["pAddr"].GetUint64());
    void* pp=dlsym(NULL,"_ZN10LevelChunk4tickER11BlockSourceRK4Tick");
    patch=(float*)((ulong)pp+(ulong)patch);
    printf("%p\n",patch);
    if(*patch!=9216){
        printf("[OPTI] Warning!!!Broken patch dected.Wont Patch it!\n");
    }else{
        mprotect((void*)ROUND_PAGE_DOWN((ulong)patch),(ROUND_PAGE_UP((ulong)patch)-ROUND_PAGE_DOWN((ulong)patch)),PROT_WRITE|PROT_READ);
        *patch=dc["pVal"].GetInt();
    }
    printf("[OPTI] loaded!\n");
    load_helper(modlist);
}
