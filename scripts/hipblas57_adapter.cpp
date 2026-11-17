#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include <cstdio>
static HMODULE self;
BOOL WINAPI DllMain(HINSTANCE h,DWORD why,LPVOID){if(why==DLL_PROCESS_ATTACH)self=h;return TRUE;}
static int datatype(int t){switch(t){case 0:return 151;case 1:return 152;case 2:return 150;case 14:return 168;default:return -1;}}
static int compute(int t){switch(t){case 0:case 1:return 150;case 2:case 3:return 151;case 7:case 8:return 152;case 9:case 10:return 162;default:return -1;}}
using Gemm=int(*)(void*,int,int,int,int,int,const void*,const void*,int,int,const void*,int,int,const void*,void*,int,int,int,int);
extern "C" __declspec(dllexport) int hipblasGemmEx(void*h,int ta,int tb,int m,int n,int k,const void*alpha,const void*a,int at,int lda,const void*b,int bt,int ldb,const void*beta,void*c,int ct,int ldc,int comp,int algo){
 static Gemm fn=[](){wchar_t path[MAX_PATH];if(!GetModuleFileNameW(self,path,MAX_PATH))return (Gemm)nullptr;auto slash=wcsrchr(path,L'\\');if(!slash)return (Gemm)nullptr;wcscpy_s(slash+1,MAX_PATH-(slash+1-path),L"hipblas57.dll");auto lib=LoadLibraryExW(path,nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);return lib?(Gemm)GetProcAddress(lib,"hipblasGemmEx"):nullptr;}();
 const int aa=datatype(at),bb=datatype(bt),cc=datatype(ct),cp=compute(comp);
 if(!fn)return 1;if(aa<0||bb<0||cc<0||cp<0||algo!=160)return 10;
 return fn(h,ta,tb,m,n,k,alpha,a,aa,lda,b,bb,ldb,beta,c,cc,ldc,cp,algo);
}
#pragma comment(linker,"/export:hipblasCreate=hipblas57.hipblasCreate")
#pragma comment(linker,"/export:hipblasDestroy=hipblas57.hipblasDestroy")
#pragma comment(linker,"/export:hipblasSetStream=hipblas57.hipblasSetStream")
#pragma comment(linker,"/export:hipblasSgemm=hipblas57.hipblasSgemm")
