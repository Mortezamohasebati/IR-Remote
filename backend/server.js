const express = require('express');
const mqtt = require('mqtt');
const { randomUUID } = require('crypto');

const app = express();
const PORT = 3001;

// به Mosquitto که روی همین سرور نصب است وصل می‌شویم
const mqttClient = mqtt.connect('mqtt://localhost:1883');
const CMD_TOPIC = 'irremote/cmd';
const RESP_TOPIC = 'irremote/resp';

// نگهداری درخواست‌های در حال انتظار پاسخ از ESP32
const pending = new Map();

mqttClient.on('connect', () => {
  console.log('[MQTT] Connected to broker');
  mqttClient.subscribe(RESP_TOPIC);
});

mqttClient.on('error', (err) => {
  console.error('[MQTT] Error:', err.message);
});

mqttClient.on('message', (topic, message) => {
  if (topic !== RESP_TOPIC) return;
  try {
    const data = JSON.parse(message.toString());
    const resolver = pending.get(data.id);
    if (resolver) {
      resolver(data.payload);
      pending.delete(data.id);
    }
  } catch (e) {
    console.error('[MQTT] Bad message:', e.message);
  }
});

// فرستادن فرمان به ESP32 از طریق MQTT و انتظار برای پاسخ (با تایم‌اوت)
function sendCommand(action, params = {}) {
  return new Promise((resolve, reject) => {
    const id = randomUUID();
    const timeout = setTimeout(() => {
      pending.delete(id);
      reject(new Error('ESP32 timeout'));
    }, 5000);

    pending.set(id, (payload) => {
      clearTimeout(timeout);
      resolve(payload);
    });

    mqttClient.publish(CMD_TOPIC, JSON.stringify({ id, action, params }));
  });
}

function handle(action, paramsFn) {
  return async (req, res) => {
    try {
      const data = await sendCommand(action, paramsFn ? paramsFn(req) : {});
      res.json(data);
    } catch (e) {
      res.status(504).json({ error: 'ESP32 پاسخ نداد', detail: e.message });
    }
  };
}

app.get('/devices', handle('devices'));
app.get('/ir', handle('ir', (req) => ({ device: req.query.device, btn: req.query.btn })));
app.get('/learn/start', handle('learn_start', (req) => ({ device: req.query.device, btn: req.query.btn })));
app.get('/learn/status', handle('learn_status'));
app.get('/learn/cancel', handle('learn_cancel'));
app.get('/learn/delete', handle('learn_delete', (req) => ({ device: req.query.device, btn: req.query.btn })));

app.listen(PORT, '127.0.0.1', () => {
  console.log(`[HTTP] Backend listening on 127.0.0.1:${PORT}`);
});
