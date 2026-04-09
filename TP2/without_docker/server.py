import ctypes
from msl.loadlib import Server32

class Server(Server32):
    def __init__(self, host, port, **kwargs):
        super().__init__('./libbridge.so', 'cdll', host, port)
        
        # 1. Definimos la firma UNA sola vez al inicializar
        self.lib.bridge.argtypes = (
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_int)
        )
        # Asumiendo que tu función de C devuelve un entero (ej. código de estado)
        self.lib.bridge.restype = ctypes.c_int 

    def bridge(self, ratios_list):
        # 2. El cliente enviará una lista normal de Python. 
        # Aquí determinamos el tamaño basados en esa lista.
        size = len(ratios_list)
        
        # 3. Creamos los arrays de C en la memoria de 32-bits
        RatiosArrayType = ctypes.c_float * size
        ratios_c = RatiosArrayType(*ratios_list) # Llenamos el array con los datos
        
        ResultArrayType = ctypes.c_int * size
        result_c = ResultArrayType() # Array vacío para que C escriba los resultados
        
        # 4. Llamamos a la librería compilada
        status = self.lib.bridge(size, ratios_c, result_c)
        
        # 5. Extraemos los resultados del puntero de C a una lista de Python
        # para poder enviarlos de vuelta al proceso de 64-bits
        result_list = [result_c[i] for i in range(size)]
        
        return status, result_list