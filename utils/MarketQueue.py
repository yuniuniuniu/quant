# -*- coding:utf-8 -*-
import ctypes

class TMarketData(ctypes.Structure):
    _fields_ = [
      ("Colo", ctypes.c_char * 16), 
      ("Tick", ctypes.c_int),
      ("UpdateTime", ctypes.c_char * 32),
      ("Data", ctypes.c_char * 4096)
    ]

class MarketQueue(object):
    loader = ctypes.cdll.LoadLibrary('./libMarketQueue.so')

    def __init__(self, TickCount:int, IPCKey:int):
        loader.MarketQueue_New.argtypes = [ctypes.c_uint, ctypes.c_uint]
        loader.MarketQueue_New.restype = ctypes.c_void_p

        loader.MarketQueue_Write.argtypes = [ctypes.c_void_p, ctypes.c_uint, ctypes.c_void_p]
        loader.MarketQueue_Write.restype = ctypes.c_bool

        loader.MarketQueue_Read.argtypes = [ctypes.c_int, ctypes.c_void_p]
        loader.MarketQueue_Read.restype = ctypes.c_bool

        loader.MarketQueue_ReadLast.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        loader.MarketQueue_ReadLast.restype = ctypes.c_bool
        
        loader.MarketQueue_Delete.argtypes = [ctypes.c_void_p]

        self.obj = loader.MarketQueue_New(TickCount, IPCKey)

    def Write(self, index:int, data:TMarketData):
        return loader.MarketQueue_Write(self.obj, index, data)

    def Read(self, index:int, data:TMarketData):
        return loader.MarketQueue_Read(self.obj, index, data)

    def ReadLast(self, data:TMarketData):
        return loader.MarketQueue_ReadLast(self.obj, data)
        
    def __del__(self):
        return loader.MarketQueue_Delete(self.obj)


if __name__ == '__main__':
    queue = MarketQueue(28801, 0XFF000003)
    data = TMarketData()
    queue.ReadLast(byref(data))
    print(data)

