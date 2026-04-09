import requests
from client import ClienteBridge

def obtener_gini_pais(iso_code):
    # 1. Configuración de la URL
    # Nota: Usamos per_page=32500 para asegurarnos de traer todo de un solo golpe
    url = f"https://api.worldbank.org/v2/en/country/all/indicator/SI.POV.GINI?format=json&date=2011:2020&per_page=32500&page=1"
    
    try:
        print(f"Consultando datos para: {iso_code}...")
        response = requests.get(url)
        response.raise_for_status()  # Lanza error si la descarga falla
        
        # La API del Banco Mundial devuelve: [ {metadatos}, [datos_reales] ]
        raw_data = response.json()
        
        if len(raw_data) < 2:
            print("No se encontraron datos en la respuesta.")
            return []

        data_points = raw_data[1] # Aquí están los registros reales
        
        # 2. Filtrado y armado del arreglo
        # Buscamos coincidencias con el countryiso3code y extraemos el 'value'
        gini_index = [
            register['value'] 
            for register in data_points 
            if register['countryiso3code'] == iso_code.upper() and register['value'] is not None
        ]
        
        return gini_index

    except Exception as e:
        print(f"Error al conectar con la API: {e}")
        return []
    

# --- Ejemplo de uso ---
country = "BRA"  # Puedes cambiar esto por "BRA", "CHL", "COL", etc.
result = obtener_gini_pais(country)

print(f"\nResultados para {country}:")
print(result)
print(f"Total de registros encontrados: {len(result)}")


# --- Ejemplo de Uso ---

# Inicializamos el cliente (esto levanta el server_32.py en segundo plano)
cliente = ClienteBridge()

# Llamamos a la función
array_resultado = cliente.ejecutar_bridge(result)
    
print("Datos enviados:", result)
print("Resultados de la DLL de 32 bits:", array_resultado)