"use strict";

const fs = require("fs");
const http = require("http");
const path = require("path");

const express = require("express");
const { Client } = require("ssh2");
const WebSocket = require("ws");

const app = express();
const server = http.createServer(app);
const wss = new WebSocket.Server({ server, path: "/ws" });

const config = {
  port: numberFromEnv("PORT", 8080),
  sshHost: process.env.SSH_HOST || "",
  sshPort: numberFromEnv("SSH_PORT", 22),
  sshUser: process.env.SSH_USER || "",
  sshPrivateKey: process.env.SSH_PRIVATE_KEY || "",
  sshPassphrase: process.env.SSH_PASSPHRASE || "",
  sshPassword: process.env.SSH_PASSWORD || "",
  remoteCommand: process.env.REMOTE_COMMAND || "",
  maxSamples: numberFromEnv("MAX_SAMPLES", 10),
  reconnectMs: numberFromEnv("RECONNECT_MS", 3000),
  readyTimeout: numberFromEnv("SSH_READY_TIMEOUT_MS", 15000)
};

const state = {
  status: "starting",
  message: "Inicializando",
  connected: false,
  lastError: null,
  lastSampleAt: null,
  sshHost: config.sshHost,
  sshUser: config.sshUser,
  remoteCommand: config.remoteCommand
};

const samples = [];
let reconnectTimer = null;
let sshClient = null;
let reconnectRequested = false;

app.use(express.text({ type: "*/*", limit: "128kb" }));
app.use("/vendor/chart.js", express.static(path.join(__dirname, "..", "node_modules", "chart.js", "dist")));
app.use(express.static(path.join(__dirname, "..", "public")));

app.get("/api/status", (_req, res) => {
  res.json({
    ...state,
    sampleCount: samples.length,
    maxSamples: config.maxSamples
  });
});

app.post("/api/samples", (req, res) => {
  const body = typeof req.body === "string" ? req.body : "";
  const lines = body.split(/\r?\n/);
  let accepted = 0;
  let rejected = 0;

  for (const line of lines) {
    if (!line.trim()) {
      continue;
    }

    if (handleJsonLine(line)) {
      accepted += 1;
    } else {
      rejected += 1;
    }
  }

  if (accepted > 0 && state.status !== "receiving") {
    setStatus("receiving", "Recibiendo muestras por POST /api/samples", true);
  }

  res.json({ accepted, rejected });
});

wss.on("connection", (socket) => {
  socket.send(JSON.stringify({ type: "status", payload: state }));
  socket.send(JSON.stringify({ type: "history", payload: samples }));
});

server.listen(config.port, () => {
  console.log(`dashboard listening on http://0.0.0.0:${config.port}`);
  startSshLoop();
});

process.on("SIGINT", shutdown);
process.on("SIGTERM", shutdown);

function numberFromEnv(name, fallback) {
  const raw = process.env[name];
  if (!raw) {
    return fallback;
  }

  const parsed = Number.parseInt(raw, 10);
  return Number.isFinite(parsed) && parsed >= 0 ? parsed : fallback;
}

function shutdown() {
  reconnectRequested = false;
  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
  }
  if (sshClient) {
    sshClient.end();
  }
  server.close(() => process.exit(0));
}

function startSshLoop() {
  if (!config.remoteCommand) {
    setStatus("manual", "Modo manual: envie JSON Lines a POST /api/samples", false);
    return;
  }

  if (!config.sshHost || !config.sshUser) {
    setStatus("disabled", "Faltan SSH_HOST o SSH_USER para ejecutar el comando remoto", false);
    return;
  }

  reconnectRequested = true;
  connectSsh();
}

function connectSsh() {
  if (!reconnectRequested) {
    return;
  }

  if (reconnectTimer) {
    clearTimeout(reconnectTimer);
    reconnectTimer = null;
  }

  setStatus("connecting", `Conectando a ${config.sshUser}@${config.sshHost}:${config.sshPort}`, false);

  const client = new Client();
  sshClient = client;

  client
    .on("ready", () => {
      setStatus("connected", "SSH conectado, ejecutando comando remoto", true);
      client.exec(config.remoteCommand, { pty: false }, (err, stream) => {
        if (err) {
          setStatus("error", `No se pudo ejecutar el comando remoto: ${err.message}`, false);
          client.end();
          scheduleReconnect();
          return;
        }

        consumeJsonLines(stream);

        stream.stderr.on("data", (chunk) => {
          const text = chunk.toString("utf8").trim();
          if (text) {
            console.error(`[remote stderr] ${text}`);
            setStatus("remote-warning", text, true);
          }
        });

        stream.on("close", (code, signal) => {
          const detail = signal ? `signal ${signal}` : `code ${code}`;
          setStatus("closed", `Comando remoto finalizado con ${detail}`, false);
          client.end();
          scheduleReconnect();
        });
      });
    })
    .on("error", (err) => {
      setStatus("error", `Error SSH: ${err.message}`, false);
      scheduleReconnect();
    })
    .on("close", () => {
      if (sshClient === client) {
        sshClient = null;
      }
    });

  try {
    client.connect(buildSshConfig());
  } catch (err) {
    setStatus("error", `No se pudo iniciar SSH: ${err.message}`, false);
    scheduleReconnect();
  }
}

function buildSshConfig() {
  const sshConfig = {
    host: config.sshHost,
    port: config.sshPort,
    username: config.sshUser,
    readyTimeout: config.readyTimeout,
    keepaliveInterval: 10000,
    keepaliveCountMax: 3
  };

  if (config.sshPrivateKey) {
    sshConfig.privateKey = fs.readFileSync(config.sshPrivateKey);
  }

  if (config.sshPassphrase) {
    sshConfig.passphrase = config.sshPassphrase;
  }

  if (config.sshPassword) {
    sshConfig.password = config.sshPassword;
  }

  return sshConfig;
}

function scheduleReconnect() {
  if (!reconnectRequested || reconnectTimer) {
    return;
  }

  reconnectTimer = setTimeout(() => {
    reconnectTimer = null;
    connectSsh();
  }, config.reconnectMs);
}

function consumeJsonLines(stream) {
  let buffer = "";

  stream.on("data", (chunk) => {
    buffer += chunk.toString("utf8");
    const lines = buffer.split(/\r?\n/);
    buffer = lines.pop() || "";

    for (const line of lines) {
      handleJsonLine(line);
    }
  });
}

function handleJsonLine(line) {
  const trimmed = line.trim();
  if (!trimmed) {
    return false;
  }

  let sample = null;
  try {
    sample = JSON.parse(trimmed);
  } catch (err) {
    setStatus("parse-warning", `Linea JSON invalida: ${trimmed.slice(0, 120)}`, true);
    return false;
  }

  if (!sample || typeof sample !== "object" || Array.isArray(sample)) {
    setStatus("parse-warning", "La linea JSON no contiene un objeto", true);
    return false;
  }

  const normalized = normalizeSample(sample);
  if (normalized === null) {
    setStatus("parse-warning", `Muestra sin codigo binario valido: ${trimmed.slice(0, 120)}`, true);
    return false;
  }

  sample = { ...sample, ...normalized };
  sample.received_at_ms = Date.now();
  samples.push(sample);
  while (samples.length > config.maxSamples) {
    samples.shift();
  }

  state.lastSampleAt = sample.received_at_ms;
  broadcast({ type: "sample", payload: sample });
  return true;
}

function setStatus(status, message, connected) {
  state.status = status;
  state.message = message;
  state.connected = connected;
  state.lastError = status === "error" ? message : state.lastError;
  broadcast({ type: "status", payload: state });
  console.log(`[${status}] ${message}`);
}

function broadcast(event) {
  const payload = JSON.stringify(event);
  for (const client of wss.clients) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  }
}

function normalizeSample(sample) {
  const gpioA = pickBit(sample, ["gpio_a_value", "value_a", "a", "channel_a", "signal_a"]);
  const gpioB = pickBit(sample, ["gpio_b_value", "value_b", "b", "channel_b", "signal_b"]);

  if (gpioA === null || gpioB === null) {
    return null;
  }

  const binaryValue = (gpioA << 1) | gpioB;
  return {
    gpio_a_value: gpioA,
    gpio_b_value: gpioB,
    binary_code: `${gpioA}${gpioB}`,
    binary_value: binaryValue,
    normalized_value: binaryValue / 3
  };
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
