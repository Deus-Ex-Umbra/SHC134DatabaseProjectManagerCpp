const express = require('express');
const { spawn } = require('child_process');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = 9542;
const exePath = path.join(__dirname, './SHC134DatabaseProjectManagerCpp.exe');

app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

let sseClient = null;

app.get('/api/stream', (req, res) => {
    res.setHeader('Content-Type', 'text/event-stream');
    res.setHeader('Cache-Control', 'no-cache');
    res.setHeader('Connection', 'keep-alive');
    res.flushHeaders();

    sseClient = res;

    req.on('close', () => {
        sseClient = null;
    });
});

function sendSseMessage(type, data) {
    if (sseClient) {
        const message = `data: ${JSON.stringify({ type, data })}\n\n`;
        sseClient.write(message);
    }
}

app.post('/api/execute', (req, res) => {
    const { action, params } = req.body;

    const args = [action];
    for (const key in params) {
        if (params[key] === true) {
            args.push(`--${key}`);
        } else if (params[key]) {
            args.push(`--${key}`, params[key]);
        }
    }

    res.status(200).json({ message: 'Proceso iniciado' });
    
    sendSseMessage('info', `▶ Ejecutando: ${path.basename(exePath)} ${args.join(' ')}\n` + '─'.repeat(80));

    const child = spawn(exePath, args);

    child.stdout.on('data', (data) => {
        sendSseMessage('stdout', data.toString());
    });

    child.stderr.on('data', (data) => {
        sendSseMessage('stderr', data.toString());
    });

    child.on('close', (code) => {
        sendSseMessage('info', '─'.repeat(80) + `\n✔ Proceso finalizado con código: ${code}`);
        if (sseClient) {
            sseClient.end(); 
            sseClient = null;
        }
    });
    child.on('error', (err) => {
        sendSseMessage('stderr', `Error al iniciar el ejecutable: ${err.message}`);
        if (sseClient) {
            sseClient.end();
            sseClient = null;
        }
    });
});

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`Servidor web iniciado.`);
    console.log(`Abre tu navegador en http://localhost:${PORT}`);
});