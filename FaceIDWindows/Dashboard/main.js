const { app, BrowserWindow, ipcMain, powerMonitor } = require('electron');
const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');

let dashboardWindow = null;
let lockWindow = null;
const configPath = path.join(app.getPath('userData'), 'faceid-config.json');

let config = {
    theme: 'blue',
    strictness: 36,
    transparency: 90,
    startWithWindows: false,
    lockOnSleep: true,
    acrylicMode: false,
    faceUnlock: true,
    requireLiveness: true
};

try {
    if (fs.existsSync(configPath)) {
        config = { ...config, ...JSON.parse(fs.readFileSync(configPath)) };
    }
} catch (e) {}

function saveConfig() {
    fs.writeFileSync(configPath, JSON.stringify(config));
}

const isLockScreen = process.argv.includes('--lockscreen');

function createLockWindow() {
    if (lockWindow && !lockWindow.isDestroyed()) return;
    lockWindow = new BrowserWindow({
        width: 800,
        height: 600,
        fullscreen: true,
        alwaysOnTop: true,
        frame: false,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    lockWindow.loadFile('lock.html');
}

function createDashboardWindow() {
    if (dashboardWindow && !dashboardWindow.isDestroyed()) return;
    dashboardWindow = new BrowserWindow({
        width: 1000,
        height: 700,
        minWidth: 900,
        minHeight: 600,
        resizable: true,
        autoHideMenuBar: true,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });

    if (config.acrylicMode) {
        dashboardWindow.setBackgroundMaterial('acrylic');
    } else {
        dashboardWindow.setBackgroundMaterial('none');
    }

    dashboardWindow.loadFile('index.html');
    dashboardWindow.setOpacity(config.transparency / 100);

    dashboardWindow.webContents.on('did-finish-load', () => {
        dashboardWindow.webContents.send('init-config', config);
    });
}

app.whenReady().then(() => {
    if (isLockScreen) {
        if (config.faceUnlock) {
            createLockWindow();
        } else {
            app.quit();
        }
    } else {
        createDashboardWindow();
    }

    powerMonitor.on('lock-screen', () => {
        if (config.lockOnSleep && config.faceUnlock) {
            createLockWindow();
        }
    });
});

ipcMain.on('quit-app', () => app.quit());

ipcMain.on('minimize-app', () => {
    if (dashboardWindow) dashboardWindow.minimize();
});

ipcMain.on('maximize-app', () => {
    if (dashboardWindow) {
        if (dashboardWindow.isMaximized()) dashboardWindow.unmaximize();
        else dashboardWindow.maximize();
    }
});

ipcMain.handle('check-pin', () => {
    const pinFile = path.join(__dirname, '..', 'password_DefaultUser.bin');
    return fs.existsSync(pinFile);
});

ipcMain.handle('get-faces', () => {
    const dir = path.join(__dirname, '..');
    const files = fs.readdirSync(dir);
    const faces = [];
    files.forEach(f => {
        if (f.startsWith('profile_') && f.endsWith('.bin')) {
            faces.push(f.substring(8, f.length - 4));
        }
    });
    return faces;
});

ipcMain.handle('get-config', () => {
    return config;
});

ipcMain.on('close-lockscreen', () => {
    if (lockWindow && !lockWindow.isDestroyed()) {
        lockWindow.close();
        lockWindow = null;
    }
    if (!dashboardWindow || dashboardWindow.isDestroyed()) {
        app.quit();
    }
});

ipcMain.on('update-config', (e, newConfig) => {
    config = { ...config, ...newConfig };
    saveConfig();
    
    if (dashboardWindow && !dashboardWindow.isDestroyed()) {
        dashboardWindow.setOpacity(config.transparency / 100);
        
        if (config.acrylicMode) {
            dashboardWindow.setBackgroundMaterial('acrylic');
        } else {
            dashboardWindow.setBackgroundMaterial('none');
        }
    }
    
    app.setLoginItemSettings({
        openAtLogin: config.startWithWindows
    });
});

let currentFaceIDProcess = null;

ipcMain.on('stop-faceid', () => {
    if (currentFaceIDProcess) {
        currentFaceIDProcess.kill();
        currentFaceIDProcess = null;
    }
});

ipcMain.on('run-faceid', (event, mode, strictness, slot = null, headless = false, stream = false) => {
    const exePath = path.join(__dirname, '..', 'build', 'App', 'Release', 'FaceIDApp.exe');
    // Convert 36 to 0.36
    const strictVal = (strictness / 100).toString();
    const args = [`--${mode}`];
    if (mode === 'enroll' && slot) {
        args.push(slot);
    }
    args.push('--strictness', strictVal);
    if (headless) args.push('--headless');
    if (stream) args.push('--stream');
    if (config.requireLiveness === false) args.push('--no-liveness');
    
    if (currentFaceIDProcess) {
        currentFaceIDProcess.kill();
    }
    const child = spawn(exePath, args, { cwd: path.join(__dirname, '..') });
    currentFaceIDProcess = child;
    let output = '';
    
    child.stdout.on('data', (data) => {
        const text = data.toString();
        if (stream) {
            // Forward live stdout chunks directly to renderer for stream processing
            event.reply('faceid-stream-data', text);
        } else {
            output += text;
        }
    });
    
    child.on('close', (code) => {
        if (currentFaceIDProcess === child) currentFaceIDProcess = null;
        event.reply('faceid-closed', mode, code, output.trim());
    });
});

ipcMain.on('save-password', (event, password) => {
    const exePath = path.join(__dirname, '..', 'build', 'App', 'Release', 'FaceIDApp.exe');
    const child = spawn(exePath, ['--save-password', password], { cwd: path.join(__dirname, '..') });
    
    child.on('close', (code) => {
        event.reply('password-saved', code);
    });
});

ipcMain.on('delete-profile', (event, slot, password) => {
    const exePath = path.join(__dirname, '..', 'build', 'App', 'Release', 'FaceIDApp.exe');
    const child = spawn(exePath, ['--delete-profile', slot, password], { cwd: path.join(__dirname, '..') });
    
    child.on('close', (code) => {
        event.reply('profile-deleted', code);
    });
});

ipcMain.on('verify-password', (event, password) => {
    const exePath = path.join(__dirname, '..', 'build', 'App', 'Release', 'FaceIDApp.exe');
    const child = spawn(exePath, ['--verify-password', password], { cwd: path.join(__dirname, '..') });
    
    child.on('close', (code) => {
        event.reply('password-verified', code === 0);
    });
});
