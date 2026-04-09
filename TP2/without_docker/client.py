from msl.loadlib import Client64

class ClienteBridge(Client64):
    def __init__(self):
        # El parámetro module32 debe ser el nombre del archivo Python 
        # del servidor (sin la extensión .py)
        super().__init__(module32='server')

    def ejecutar_bridge(self, ratios):
        """
        Envía una lista de ratios al servidor 32-bit y retorna el resultado.
        """
        # ========================================================
        # AQUÍ USAMOS request32
        # 'bridge' -> Es el nombre de la def en server_32.py
        # ratios -> Es el argumento que requiere esa def
        # ========================================================
        status, resultados = self.request32('bridge', ratios)
        
        if status != 0:
            print(f"Advertencia: El proceso de C retornó un estado {status}")
            
        return resultados