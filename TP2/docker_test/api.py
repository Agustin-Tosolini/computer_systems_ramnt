from __future__ import annotations

import ctypes
import json
import os
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any
from urllib.parse import urlencode
from urllib.request import urlopen

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from flask import Flask, jsonify


BASE_URL = "https://api.worldbank.org/v2/en/country/{country_scope}/indicator/{indicator}"
DEFAULT_TIMEOUT = 20
DEFAULT_INDICATOR = "SI.POV.GINI"
DEFAULT_COUNTRY = "Argentina"
DEFAULT_START_YEAR = 2011
DEFAULT_END_YEAR = 2020

BASE_DIR = Path(__file__).resolve().parent
BUILD_DIR = Path(os.getenv("BUILD_DIR", BASE_DIR / "runtime" / "build"))
LOG_DIR = Path(os.getenv("LOG_DIR", BASE_DIR / "runtime" / "logs"))
LIBRARY_PATH = Path(os.getenv("LIBRARY_PATH", BUILD_DIR / "libfloat_to_int.so"))

app = Flask(__name__)


@dataclass
class IndicatorPoint:
    year: int
    value: float


def ensure_directory(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def build_world_bank_params(
    start_year: int,
    end_year: int,
    page: int = 1,
    per_page: int = 32500,
    extra_params: dict[str, Any] | None = None,
) -> dict[str, Any]:
    params: dict[str, Any] = {
        "format": "json",
        "date": f"{start_year}:{end_year}",
        "page": page,
        "per_page": per_page,
    }

    if extra_params:
        params.update(extra_params)

    return params


def build_world_bank_url(
    indicator: str,
    country_scope: str = "all",
    params: dict[str, Any] | None = None,
) -> str:
    base_url = BASE_URL.format(country_scope=country_scope, indicator=indicator)
    if not params:
        return base_url
    return f"{base_url}?{urlencode(params)}"


def fetch_world_bank_data(
    indicator: str,
    country_scope: str = "all",
    params: dict[str, Any] | None = None,
) -> list[Any]:
    url = build_world_bank_url(indicator=indicator, country_scope=country_scope, params=params)
    with urlopen(url, timeout=DEFAULT_TIMEOUT) as response:
        return json.load(response)


def get_world_bank_records(
    indicator: str,
    country_scope: str = "all",
    params: dict[str, Any] | None = None,
) -> list[dict[str, Any]]:
    payload = fetch_world_bank_data(indicator=indicator, country_scope=country_scope, params=params)
    if not isinstance(payload, list) or len(payload) < 2 or not isinstance(payload[1], list):
        return []
    return payload[1]


def get_indicator_points_for_country(
    indicator: str,
    country_name: str,
    start_year: int,
    end_year: int,
) -> list[IndicatorPoint]:
    params = build_world_bank_params(start_year=start_year, end_year=end_year)
    records = get_world_bank_records(indicator=indicator, params=params)
    normalized_country = country_name.casefold()

    points: list[IndicatorPoint] = []
    for record in records:
        if not isinstance(record, dict):
            continue

        record_country = record.get("country", {}).get("value", "")
        record_value = record.get("value")
        record_year = record.get("date")

        if record_country.casefold() != normalized_country:
            continue
        if record_value is None or record_year is None:
            continue

        points.append(IndicatorPoint(year=int(record_year), value=float(record_value)))

    points.sort(key=lambda point: point.year)
    return points


def load_conversion_library(library_path: Path = LIBRARY_PATH) -> ctypes.CDLL:
    if not library_path.exists():
        raise FileNotFoundError(
            f"No se encontro la libreria compilada en {library_path}. "
            "Compila C y ASM antes de ejecutar Python."
        )

    library = ctypes.CDLL(str(library_path))
    library.convert_float_array_to_int.argtypes = [
        ctypes.POINTER(ctypes.c_float),
        ctypes.POINTER(ctypes.c_int),
        ctypes.c_int,
    ]
    library.convert_float_array_to_int.restype = ctypes.c_int
    return library


def convert_points_with_c(points: list[IndicatorPoint]) -> list[int]:
    if not points:
        return []

    library = load_conversion_library()
    count = len(points)
    float_array = (ctypes.c_float * count)(*[point.value for point in points])
    int_array = (ctypes.c_int * count)()

    status = library.convert_float_array_to_int(float_array, int_array, count)
    if status != 0:
        raise RuntimeError(f"La conversion en C/ASM fallo con codigo {status}")

    return list(int_array)


def increment_values(values: list[int], delta: int = 1) -> list[int]:
    return [value + delta for value in values]


def build_output_filename(prefix: str = "gini_argentina") -> Path:
    timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M")
    ensure_directory(LOG_DIR)
    return LOG_DIR / f"{prefix}_{timestamp}.png"


def plot_values(years: list[int], values: list[int], output_path: Path) -> Path:
    plt.figure(figsize=(10, 6))
    plt.plot(years, values, marker="o", linewidth=2, color="#1f6feb")
    plt.title("Indice GINI de Argentina convertido a entero y ajustado +1")
    plt.xlabel("Anio")
    plt.ylabel("Valor entero ajustado")
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(output_path)
    plt.close()
    return output_path


def run_pipeline() -> dict[str, Any]:
    points = get_indicator_points_for_country(
        indicator=DEFAULT_INDICATOR,
        country_name=DEFAULT_COUNTRY,
        start_year=DEFAULT_START_YEAR,
        end_year=DEFAULT_END_YEAR,
    )
    converted_values = convert_points_with_c(points)
    adjusted_values = increment_values(converted_values, delta=1)
    output_path = build_output_filename()
    plot_values([point.year for point in points], adjusted_values, output_path)

    return {
        "country": DEFAULT_COUNTRY,
        "indicator": DEFAULT_INDICATOR,
        "years": [point.year for point in points],
        "original_values": [point.value for point in points],
        "converted_values": converted_values,
        "adjusted_values": adjusted_values,
        "plot_path": str(output_path),
    }


@app.get("/api/gini/argentina")
def argentina_gini() -> Any:
    points = get_indicator_points_for_country(
        indicator=DEFAULT_INDICATOR,
        country_name=DEFAULT_COUNTRY,
        start_year=DEFAULT_START_YEAR,
        end_year=DEFAULT_END_YEAR,
    )
    return jsonify([point.value for point in points])


if __name__ == "__main__":
    result = run_pipeline()
    print("Anios:", result["years"])
    print("Valores originales:", result["original_values"])
    print("Valores convertidos:", result["converted_values"])
    print("Valores ajustados (+1):", result["adjusted_values"])
    print("Grafico guardado en:", result["plot_path"])
