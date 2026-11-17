import ctypes as c, os, pathlib, struct, sys
root=pathlib.Path(__file__).resolve().parents[1]
handles=[os.add_dll_directory(str(root/'runtime'))]
h=c.CDLL(r'C:\Windows\System32\amdhip64.dll');b=c.CDLL(str(root/'runtime/hipblas.dll'))
P=c.c_void_p;I=c.c_int;Z=c.c_size_t
for name,args in [('hipMalloc',[c.POINTER(P),Z]),('hipMemcpy',[P,P,Z,I]),('hipFree',[P]),('hipDeviceSynchronize',[])]:getattr(h,name).argtypes=args;getattr(h,name).restype=I
b.hipblasCreate.argtypes=[c.POINTER(P)];b.hipblasDestroy.argtypes=[P]
b.hipblasGemmEx.argtypes=[P,I,I,I,I,I,P,P,I,I,P,I,I,P,P,I,I,I,I];b.hipblasGemmEx.restype=I
assert h.hipSetDevice(int(sys.argv[1]) if len(sys.argv)>1 else 0)==0
ctx=P();assert b.hipblasCreate(c.byref(ctx))==0
allocs=[]
def malloc(n):
 p=P();assert h.hipMalloc(c.byref(p),n)==0;allocs.append(p);return p
try:
 a=malloc(8);bb=malloc(8);out=malloc(16);data=c.create_string_buffer(struct.pack('<4e',1,1,1,1));assert h.hipMemcpy(a,data,8,1)==0;assert h.hipMemcpy(bb,data,8,1)==0
 alpha=c.c_float(1);beta=c.c_float(0)
 for typ,size,fmt in [(0,16,'<4f'),(2,8,'<4e')]:
  rc=b.hipblasGemmEx(ctx,111,111,2,2,2,c.byref(alpha),a,2,2,bb,2,2,c.byref(beta),out,typ,2,2,160)
  assert rc==0,('GEMM',typ,rc)
  assert h.hipDeviceSynchronize()==0
  buf=c.create_string_buffer(size);assert h.hipMemcpy(buf,out,size,2)==0
  vals=struct.unpack(fmt,buf.raw);assert vals==(2.,2.,2.,2.),vals
  print('PASS modern type',typ,'FP32 accumulation =>',vals,flush=True)
 rc=b.hipblasGemmEx(ctx,111,111,2,2,2,c.byref(alpha),a,99,2,bb,2,2,c.byref(beta),out,0,2,2,160)
 assert rc==10,rc
 print('PASS unsupported type rejected',flush=True)
finally:
 b.hipblasDestroy(ctx)
 for p in allocs:h.hipFree(p)
