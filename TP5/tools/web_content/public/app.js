"use strict";

const maxPoints = 200;
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
        label: "value_a",
        data: [],
        borderColor: "#2563eb",
        backgroundColor: "rgba(37, 99, 235, 0.12)",
        borderWidth: 2,
        pointRadius: 0,
        tension: 0.2
      },
      {
        label: "value_b",
        data: [],
        borderColor: "#16a34a",
        backgroundColor: "rgba(22, 163, 74, 0.12)",
        borderWidth: 2,
        pointRadius: 0,
        tension: 0.2
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
  chart.data.datasets[1].data = [];
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
  }

  appendLog(`[${new Date().toLocaleTimeString()}] ${status.status}: ${status.message}`);
}

function addSample(sample, updateChart) {
  const a = pickNumber(sample, ["value_a", "channel_a", "gpio_a", "signal_a", "a", "value0", "channel_0"]);
  const b = pickNumber(sample, ["value_b", "channel_b", "gpio_b", "signal_b", "b", "value1", "channel_1"]);

  if (a === null || b === null) {
    appendLog(`Muestra sin value_a/value_b: ${JSON.stringify(sample).slice(0, 120)}`);
    return;
  }

  totalSamples += 1;
  const timestamp = sample.timestamp_ms || sample.received_at_ms || Date.now();
  const label = new Date(timestamp).toLocaleTimeString();

  chart.data.labels.push(label);
  chart.data.datasets[0].data.push(a);
  chart.data.datasets[1].data.push(b);

  while (chart.data.labels.length > maxPoints) {
    chart.data.labels.shift();
    chart.data.datasets[0].data.shift();
    chart.data.datasets[1].data.shift();
  }

  valueA.textContent = String(a);
  valueB.textContent = String(b);
  sampleCount.textContent = String(totalSamples);
  sourceText.textContent = sample.source ? `${sample.source} #${sample.seq || totalSamples}` : `#${sample.seq || totalSamples}`;

  if (updateChart) {
    chart.update();
  }
}

function pickNumber(sample, names) {
  for (const name of names) {
    const value = sample[name];
    if (typeof value === "number" && Number.isFinite(value)) {
      return value;
    }
  }
  return null;
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
