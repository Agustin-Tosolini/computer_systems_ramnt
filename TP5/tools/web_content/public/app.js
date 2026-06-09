"use strict";

const maxPoints = 10;
const statusBadge = document.getElementById("statusBadge");
const statusText = document.getElementById("statusText");
const commandText = document.getElementById("commandText");
const valueA = document.getElementById("valueA");
const valueB = document.getElementById("valueB");
const sampleCount = document.getElementById("sampleCount");
const sourceText = document.getElementById("sourceText");
const clearButton = document.getElementById("clearButton");
const logOutput = document.getElementById("logOutput");
const canvas = document.getElementById("samplesChart");

let totalSamples = 0;

const chart = new Chart(canvas, {
  type: "line",
  data: {
    labels: [],
    datasets: [
      {
        label: "codigo GPIO",
        data: [],
        borderColor: "#2563eb",
        backgroundColor: "rgba(37, 99, 235, 0.12)",
        borderWidth: 2,
        pointRadius: 4,
        pointHoverRadius: 5,
        stepped: true,
        tension: 0
      }
    ]
  },
  options: {
    animation: false,
    responsive: true,
    maintainAspectRatio: false,
    interaction: {
      intersect: false,
      mode: "index"
    },
    plugins: {
      legend: {
        labels: {
          usePointStyle: true,
          boxWidth: 8
        }
      },
      tooltip: {
        callbacks: {
          label(context) {
            const raw = context.raw || {};
            return `codigo ${raw.code || codeFromLevel(context.parsed.y)}`;
          }
        }
      }
    },
    scales: {
      x: {
        ticks: {
          maxTicksLimit: 8,
          maxRotation: 0
        },
        grid: {
          color: "rgba(148, 163, 184, 0.18)"
        }
      },
      y: {
        min: 0,
        max: 1,
        ticks: {
          stepSize: 1 / 3,
          callback(value) {
            return codeFromLevel(Number(value));
          }
        },
        grid: {
          color: "rgba(148, 163, 184, 0.2)"
        }
      }
    }
  }
});

clearButton.addEventListener("click", () => {
  chart.data.labels = [];
  chart.data.datasets[0].data = [];
  totalSamples = 0;
  sampleCount.textContent = "0";
  valueA.textContent = "--";
  valueB.textContent = "--";
  sourceText.textContent = "Sin datos";
  chart.update();
});

loadInitialStatus();
connectWebSocket();

async function loadInitialStatus() {
  try {
    const response = await fetch("/api/status");
    if (!response.ok) {
      return;
    }
    const status = await response.json();
    updateStatus(status);
  } catch (_err) {
    updateStatus({ status: "offline", message: "Servidor web no disponible", connected: false });
  }
}

function connectWebSocket() {
  const protocol = window.location.protocol === "https:" ? "wss" : "ws";
  const socket = new WebSocket(`${protocol}://${window.location.host}/ws`);

  socket.addEventListener("open", () => {
    appendLog("WebSocket conectado");
  });

  socket.addEventListener("message", (event) => {
    const message = JSON.parse(event.data);
    if (message.type === "status") {
      updateStatus(message.payload);
    } else if (message.type === "history") {
      for (const sample of message.payload) {
        addSample(sample, false);
      }
      chart.update();
    } else if (message.type === "sample") {
      addSample(message.payload, true);
    }
  });

  socket.addEventListener("close", () => {
    updateStatus({ status: "reconnecting", message: "Reconectando WebSocket", connected: false });
    setTimeout(connectWebSocket, 1500);
  });

  socket.addEventListener("error", () => {
    updateStatus({ status: "error", message: "Error de WebSocket", connected: false });
  });
}

function updateStatus(status) {
  statusText.textContent = status.message || status.status || "Sin estado";
  statusBadge.dataset.state = status.connected ? "connected" : status.status || "offline";

  if (status.remoteCommand) {
    commandText.textContent = `${status.sshUser}@${status.sshHost}: ${status.remoteCommand}`;
  } else if (status.status === "manual" || status.status === "receiving") {
    commandText.textContent = "POST /api/samples";
  }

  appendLog(`[${new Date().toLocaleTimeString()}] ${status.status}: ${status.message}`);
}

function addSample(sample, updateChart) {
  const a = pickBit(sample, ["gpio_a_value", "value_a", "channel_a", "signal_a", "a", "value0", "channel_0"]);
  const b = pickBit(sample, ["gpio_b_value", "value_b", "channel_b", "signal_b", "b", "value1", "channel_1"]);

  if (a === null || b === null) {
    appendLog(`Muestra sin codigo binario valido: ${JSON.stringify(sample).slice(0, 120)}`);
    return;
  }

  totalSamples += 1;
  const timestamp = sample.timestamp_ms || sample.received_at_ms || Date.now();
  const label = new Date(timestamp).toLocaleTimeString();
  const binaryValue = Number.isFinite(sample.binary_value) ? sample.binary_value : (a << 1) | b;
  const normalizedValue = Number.isFinite(sample.normalized_value) ? sample.normalized_value : binaryValue / 3;
  const binaryCode = typeof sample.binary_code === "string" ? sample.binary_code : `${a}${b}`;

  chart.data.labels.push(label);
  chart.data.datasets[0].data.push({
    x: label,
    y: normalizedValue,
    code: binaryCode
  });

  while (chart.data.labels.length > maxPoints) {
    chart.data.labels.shift();
    chart.data.datasets[0].data.shift();
  }

  valueA.textContent = String(a);
  valueB.textContent = String(b);
  sampleCount.textContent = String(totalSamples);
  sourceText.textContent = `${binaryCode} (${binaryValue})`;

  if (updateChart) {
    chart.update();
  }
}

function pickBit(sample, names) {
  for (const name of names) {
    const value = sample[name];
    if (typeof value === "number" && Number.isFinite(value)) {
      return value === 0 ? 0 : 1;
    }
  }
  return null;
}

function codeFromLevel(value) {
  const code = Math.round(value * 3);
  if (code <= 0) {
    return "00";
  }
  if (code === 1) {
    return "01";
  }
  if (code === 2) {
    return "10";
  }
  return "11";
}

function appendLog(line) {
  if (!line) {
    return;
  }

  const existing = logOutput.textContent ? logOutput.textContent.split("\n") : [];
  existing.push(line);
  while (existing.length > 80) {
    existing.shift();
  }
  logOutput.textContent = existing.join("\n");
  logOutput.scrollTop = logOutput.scrollHeight;
}
