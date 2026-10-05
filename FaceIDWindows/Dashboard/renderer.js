const { ipcRenderer } = require('electron');

let currentConfig = {};

// DOM Elements
const pages = document.querySelectorAll('.page');
const navLinks = document.querySelectorAll('.nav-links li');

const strictnessSlider = document.getElementById('strictnessSlider');
const strictVal = document.getElementById('strictVal');
const startWindowsToggle = document.getElementById('startWindowsToggle');
const lockSleepToggle = document.getElementById('lockSleepToggle');
const acrylicToggle = document.getElementById('acrylicToggle');
const transparencySlider = document.getElementById('transparencySlider');
const transparencyVal = document.getElementById('transparencyVal');
const faceUnlockToggle = document.getElementById('faceUnlockToggle');
const requireLivenessToggle = document.getElementById('requireLivenessToggle');
const changePinBtn = document.getElementById('changePinBtn');

const facesList = document.getElementById('facesList');
const overviewTestBtn = document.getElementById('overviewTestBtn');
const addNewFaceBtn = document.getElementById('addNewFaceBtn');

// Modals
const authModal = document.getElementById('authModal');
const setupPinModal = document.getElementById('setupPinModal');
const addFaceModal = document.getElementById('addFaceModal');

// Status Elements
const heroStateTitle = document.getElementById('hero-state-title');
const heroStateSubtitle = document.getElementById('hero-state-subtitle');
const overviewScanLine = document.getElementById('overviewScanLine');

// Modal Helper
function openModal(modal) {
    modal.classList.remove('hiding');
    modal.style.display = 'flex';
}

function closeModal(modal) {
    modal.classList.add('hiding');
    setTimeout(() => {
        if (modal.classList.contains('hiding')) {
            modal.style.display = 'none';
            modal.classList.remove('hiding');
        }
    }, 200);
}

// Navigation
navLinks.forEach(link => {
    link.addEventListener('click', () => {
        navLinks.forEach(l => l.classList.remove('active'));
        pages.forEach(p => p.classList.remove('active'));
        
        link.classList.add('active');
        document.getElementById(`page-${link.dataset.page}`).classList.add('active');
    });
});

// Config handling
function notifyUpdate() {
    ipcRenderer.send('update-config', currentConfig);
}

ipcRenderer.on('init-config', (e, config) => {
    currentConfig = config;
    
    strictnessSlider.value = config.strictness;
    strictVal.innerText = (config.strictness / 100).toFixed(2);
    
    startWindowsToggle.checked = config.startWithWindows;
    lockSleepToggle.checked = config.lockOnSleep;
    
    if (config.faceUnlock !== undefined) faceUnlockToggle.checked = config.faceUnlock;
    if (config.requireLiveness !== undefined) requireLivenessToggle.checked = config.requireLiveness;
    
    if (config.acrylicMode) {
        acrylicToggle.checked = true;
        document.body.classList.add('acrylic-mode');
    } else {
        acrylicToggle.checked = false;
        document.body.classList.remove('acrylic-mode');
    }

    if (config.transparency !== undefined) {
        transparencySlider.value = config.transparency;
        transparencyVal.innerText = config.transparency + '%';
    }
    
    loadFaces();
});

transparencySlider.oninput = (e) => {
    currentConfig.transparency = parseInt(e.target.value);
    transparencyVal.innerText = currentConfig.transparency + '%';
    notifyUpdate();
};

strictnessSlider.oninput = (e) => {
    currentConfig.strictness = parseInt(e.target.value);
    strictVal.innerText = (currentConfig.strictness / 100).toFixed(2);
    notifyUpdate();
};

startWindowsToggle.onchange = (e) => {
    currentConfig.startWithWindows = e.target.checked;
    notifyUpdate();
};

lockSleepToggle.onchange = (e) => {
    currentConfig.lockOnSleep = e.target.checked;
    notifyUpdate();
};

acrylicToggle.onchange = (e) => {
    currentConfig.acrylicMode = e.target.checked;
    if (currentConfig.acrylicMode) {
        document.body.classList.add('acrylic-mode');
    } else {
        document.body.classList.remove('acrylic-mode');
    }
    notifyUpdate();
};

faceUnlockToggle.onchange = (e) => {
    currentConfig.faceUnlock = e.target.checked;
    notifyUpdate();
};

requireLivenessToggle.onchange = (e) => {
    currentConfig.requireLiveness = e.target.checked;
    notifyUpdate();
};

if (changePinBtn) {
    changePinBtn.onclick = () => {
        showSetupPinModal();
    };
}

// Faces Logic
function loadFaces() {
    ipcRenderer.invoke('get-faces').then((faces) => {
        facesList.innerHTML = '';
        if (faces.length === 0) {
            facesList.innerHTML = '<div style="font-size: 13px; color: var(--text-muted); padding: 10px;">No biometric profiles enrolled.</div>';
            return;
        }
        faces.forEach(face => {
            const div = document.createElement('div');
            div.className = 'profile-item';
            div.innerHTML = `
                <div class="profile-info-block">
                    <span class="profile-name">${face}</span>
                    <span class="profile-meta">Status: Enrolled &nbsp;&bull;&nbsp; Quality: Excellent</span>
                </div>
                <div class="profile-actions">
                    <button class="btn btn-outline btn-sm delete-face-btn" data-face="${face}" style="color: var(--danger); border-color: rgba(239,68,68,0.3);">Delete</button>
                </div>
            `;
            facesList.appendChild(div);
        });

        // Attach delete handlers
        document.querySelectorAll('.delete-face-btn').forEach(btn => {
            btn.onclick = (e) => {
                showAuthModal('delete', e.target.dataset.face);
            };
        });
    });
}

// PIN Setup
ipcRenderer.invoke('check-pin').then((hasPin) => {
    if (!hasPin) {
        openModal(setupPinModal);
    }
});

document.getElementById('confirmSetupPinBtn').onclick = () => {
    const p1 = document.getElementById('setupPin1').value;
    const p2 = document.getElementById('setupPin2').value;
    if (p1.length > 0 && p1 === p2) {
        ipcRenderer.send('save-password', p1);
        closeModal(setupPinModal);
    } else {
        alert('Passwords must match and cannot be empty.');
    }
};

document.getElementById('setupPin2').addEventListener('keyup', (e) => {
    if (e.key === 'Enter') document.getElementById('confirmSetupPinBtn').click();
});

// Change PIN
document.getElementById('changePinBtn').onclick = () => {
    showAuthModal('change-pin');
};

// Auth Modal
let pendingAction = null;
const authTitle = document.getElementById('authTitle');
const authDesc = document.getElementById('authDesc');
const confirmAuthBtn = document.getElementById('confirmAuthBtn');
const authPassword = document.getElementById('authPassword');

function showAuthModal(type, face = null) {
    pendingAction = { type, face };
    authPassword.value = '';
    
    if (type === 'enroll') {
        authTitle.innerText = "Authorize Enrollment";
        authTitle.style.color = "var(--primary)";
        authDesc.innerText = `Enter Password to authorize new profile: ${face}`;
        confirmAuthBtn.innerText = "Authorize";
        confirmAuthBtn.className = "btn btn-primary";
    } else if (type === 'delete') {
        authTitle.innerText = "Authorize Deletion";
        authTitle.style.color = "var(--danger)";
        authDesc.innerText = `Enter Password to delete profile: ${face}`;
        confirmAuthBtn.innerText = "Delete Profile";
        confirmAuthBtn.className = "btn btn-primary";
        confirmAuthBtn.style.background = "var(--danger)";
    } else if (type === 'change-pin') {
        authTitle.innerText = "Verify Current Password";
        authTitle.style.color = "var(--text-main)";
        authDesc.innerText = `Enter your current Password to change it.`;
        confirmAuthBtn.innerText = "Verify";
        confirmAuthBtn.className = "btn btn-primary";
    }
    
    authModal.style.display = 'flex'; // Trigger animation immediately
    authModal.classList.remove('hiding');
    authPassword.focus();
}

document.getElementById('cancelAuthBtn').onclick = () => {
    closeModal(authModal);
};

authPassword.addEventListener('keyup', (e) => {
    if (e.key === 'Enter') confirmAuthBtn.click();
});

confirmAuthBtn.onclick = () => {
    const pw = authPassword.value;
    if (pw.length > 0) {
        const originalText = confirmAuthBtn.innerText;
        confirmAuthBtn.innerText = "Verifying...";
        
        if (pendingAction.type === 'enroll') {
            ipcRenderer.send('verify-password', pw);
            ipcRenderer.once('password-verified', (event, success) => {
                if (success) {
                    closeModal(authModal);
                    startEnrollment(pendingAction.face);
                } else {
                    confirmAuthBtn.innerText = "Incorrect PIN";
                    setTimeout(() => confirmAuthBtn.innerText = originalText, 2000);
                }
            });
        } else if (pendingAction.type === 'delete') {
            ipcRenderer.send('delete-profile', pendingAction.face, pw);
            ipcRenderer.once('profile-deleted', (event, code) => {
                if (code === 0) {
                    closeModal(authModal);
                    loadFaces();
                } else {
                    confirmAuthBtn.innerText = "Incorrect PIN";
                    setTimeout(() => confirmAuthBtn.innerText = originalText, 2000);
                }
            });
        } else if (pendingAction.type === 'change-pin') {
            ipcRenderer.send('verify-password', pw);
            ipcRenderer.once('password-verified', (event, success) => {
                if (success) {
                    closeModal(authModal);
                    setTimeout(() => openModal(setupPinModal), 250); // Re-use setup modal
                } else {
                    confirmAuthBtn.innerText = "Incorrect PIN";
                    setTimeout(() => confirmAuthBtn.innerText = originalText, 2000);
                }
            });
        }
    }
};

// Add New Face Logic
addNewFaceBtn.onclick = () => {
    document.getElementById('newFaceNameInput').value = '';
    openModal(addFaceModal);
    document.getElementById('newFaceNameInput').focus();
};

document.getElementById('cancelAddFaceBtn').onclick = () => {
    closeModal(addFaceModal);
};

document.getElementById('newFaceNameInput').addEventListener('keyup', (e) => {
    if (e.key === 'Enter') document.getElementById('confirmAddFaceBtn').click();
});

document.getElementById('confirmAddFaceBtn').onclick = () => {
    const name = document.getElementById('newFaceNameInput').value.trim();
    if (name) {
        closeModal(addFaceModal);
        setTimeout(() => showAuthModal('enroll', name), 250);
    }
};

// Camera Diagnostics
const cameraSelect = document.getElementById('cameraSelect');
const cameraPreviewVideo = document.getElementById('cameraPreviewVideo');
const cameraPreviewPlaceholder = document.getElementById('cameraPreviewPlaceholder');
const testCameraBtn = document.getElementById('testCameraBtn');
const refreshCamerasBtn = document.getElementById('refreshCamerasBtn');
const camRes = document.getElementById('camRes');
const camStatus = document.getElementById('camStatus');
let currentStream = null;

async function getCameras() {
    try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        const videoDevices = devices.filter(device => device.kind === 'videoinput');
        
        cameraSelect.innerHTML = '';
        if (videoDevices.length === 0) {
            cameraSelect.innerHTML = '<option>No cameras found</option>';
            return;
        }
        
        videoDevices.forEach(device => {
            const option = document.createElement('option');
            option.value = device.deviceId;
            option.text = device.label || `Camera ${cameraSelect.length + 1}`;
            cameraSelect.appendChild(option);
        });
    } catch (e) {
        console.error(e);
        camStatus.innerText = '● Error';
        camStatus.style.color = 'var(--danger)';
    }
}

refreshCamerasBtn.onclick = getCameras;

testCameraBtn.onclick = async () => {
    if (currentStream) {
        // Stop preview
        currentStream.getTracks().forEach(track => track.stop());
        cameraPreviewVideo.srcObject = null;
        cameraPreviewVideo.style.display = 'none';
        cameraPreviewPlaceholder.style.display = 'block';
        testCameraBtn.innerText = 'Start Preview';
        camRes.innerText = '--';
        camStatus.innerText = '● Ready';
        camStatus.style.color = 'var(--success)';
        currentStream = null;
        return;
    }

    const deviceId = cameraSelect.value;
    if (!deviceId) return;

    try {
        camStatus.innerText = '● Starting...';
        camStatus.style.color = 'var(--warning)';
        
        const stream = await navigator.mediaDevices.getUserMedia({
            video: { deviceId: { exact: deviceId } }
        });
        
        currentStream = stream;
        cameraPreviewVideo.srcObject = stream;
        cameraPreviewVideo.style.display = 'block';
        cameraPreviewPlaceholder.style.display = 'none';
        
        // Get actual resolution
        const track = stream.getVideoTracks()[0];
        const settings = track.getSettings();
        camRes.innerText = `${settings.width} × ${settings.height}`;
        
        testCameraBtn.innerText = 'Stop Preview';
        camStatus.innerText = '● Active';
        camStatus.style.color = 'var(--primary)';
    } catch (e) {
        console.error(e);
        camStatus.innerText = '● Access Denied';
        camStatus.style.color = 'var(--danger)';
    }
};

// Initialize cameras if we switch to camera tab
navLinks.forEach(link => {
    link.addEventListener('click', () => {
        if (link.dataset.page === 'camera' && cameraSelect.options.length <= 1) {
            getCameras();
        }
    });
});

// Engine Integration
function setHeroState(state, title, subtitle) {
    heroStateTitle.innerText = title;
    heroStateSubtitle.innerText = "Status: " + subtitle;
    
    if (state === 'scanning') {
        overviewScanLine.classList.add('active');
        heroStateTitle.style.color = "var(--primary)";
    } else if (state === 'success') {
        overviewScanLine.classList.remove('active');
        heroStateTitle.style.color = "var(--success)";
    } else if (state === 'error') {
        overviewScanLine.classList.remove('active');
        heroStateTitle.style.color = "var(--danger)";
    } else {
        overviewScanLine.classList.remove('active');
        heroStateTitle.style.color = "var(--text-main)";
    }
}

let currentEnrollmentName = "";
let streamBuffer = "";

function startEnrollment(faceName) {
    currentEnrollmentName = faceName;
    streamBuffer = "";
    
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById('page-enrollment').classList.add('active');
    
    document.getElementById('enrollmentInstructions').innerText = "Look directly at the camera and follow the instructions.";
    document.getElementById('enrollmentProgressContainer').style.display = 'none';
    document.getElementById('enrollmentCameraContainer').style.display = 'block';
    document.getElementById('faceBoundingBox').style.display = 'none';
    document.getElementById('enrollmentPreview').style.display = 'none';
    
    document.getElementById('startEnrollmentBtn').style.display = 'none';
    document.getElementById('cancelEnrollmentBtn').innerText = "Stop";
    
    document.getElementById('enrollmentFaceGuide').style.borderColor = 'rgba(96, 165, 250, 0.7)';
    document.getElementById('enrollmentFaceGuide').style.boxShadow = '0 0 20px rgba(96, 165, 250, 0.3) inset';
    document.getElementById('enrollmentProgressContainer').style.display = 'block';
    
    // Automatically start the enrollment process with headless=true, stream=true
    ipcRenderer.send('run-faceid', 'enroll', currentConfig.strictness, currentEnrollmentName, true, true);
}

document.getElementById('startEnrollmentBtn').onclick = () => {
    // Kept for manual restart if needed
    document.getElementById('startEnrollmentBtn').style.display = 'none';
    document.getElementById('cancelEnrollmentBtn').innerText = "Stop";
    document.getElementById('enrollmentProgressContainer').style.display = 'block';
    ipcRenderer.send('run-faceid', 'enroll', currentConfig.strictness, currentEnrollmentName, true, true);
};

document.getElementById('cancelEnrollmentBtn').onclick = () => {
    ipcRenderer.send('stop-faceid');
    navLinks[0].click(); // go to overview
};

ipcRenderer.on('faceid-stream-data', (event, data) => {
    streamBuffer += data;
    let newlineIdx;
    while ((newlineIdx = streamBuffer.indexOf('\n')) !== -1) {
        const line = streamBuffer.slice(0, newlineIdx).trim();
        streamBuffer = streamBuffer.slice(newlineIdx + 1);
        
        if (line.startsWith('FRAME:')) {
            const b64 = line.substring(6);
            document.getElementById('enrollmentPreview').src = 'data:image/jpeg;base64,' + b64;
            document.getElementById('enrollmentPreview').style.display = 'block';
        } else if (line.startsWith('STATE:UI:')) {
            const text = line.substring(9);
            document.getElementById('enrollmentInstructions').innerText = text;
            const orb = document.getElementById('enrollmentOrb');
            const guide = document.getElementById('enrollmentFaceGuide');
            
            if (text.includes('Capturing')) {
                orb.className = 'face-orb active pulse';
                guide.style.borderColor = 'rgba(16, 185, 129, 0.8)'; // Green
                guide.style.boxShadow = '0 0 20px rgba(16, 185, 129, 0.4) inset';
            } else if (text.includes('Please') || text.includes('Center')) {
                orb.className = 'face-orb pulse';
                guide.style.borderColor = 'rgba(239, 68, 68, 0.8)'; // Red
                guide.style.boxShadow = '0 0 20px rgba(239, 68, 68, 0.4) inset';
            } else {
                orb.className = 'face-orb pulse active';
                guide.style.borderColor = 'rgba(96, 165, 250, 0.7)'; // Blue
                guide.style.boxShadow = '0 0 20px rgba(96, 165, 250, 0.3) inset';
            }
        } else if (line.startsWith('STATE:FACE:')) {
            const parts = line.substring(11).split(',');
            const bb = document.getElementById('faceBoundingBox');
            bb.style.display = 'block';
            bb.style.left = (parseFloat(parts[0]) * 0.5) + 'px';
            bb.style.top = (parseFloat(parts[1]) * 0.5) + 'px';
            bb.style.width = (parseFloat(parts[2]) * 0.5) + 'px';
            bb.style.height = (parseFloat(parts[3]) * 0.5) + 'px';
        } else if (line.startsWith('STATE:NOFACE')) {
            document.getElementById('faceBoundingBox').style.display = 'none';
            document.getElementById('enrollmentFaceGuide').style.borderColor = 'rgba(96, 165, 250, 0.3)';
            document.getElementById('enrollmentFaceGuide').style.boxShadow = 'none';
        } else if (line.startsWith('STATE:PROGRESS:')) {
            const parts = line.substring(15).split(',');
            const progress = parseInt(parts[1]);
            const max = parseInt(parts[2]);
            document.getElementById('enrollmentStatusText').innerText = `${progress} / ${max} samples`;
            
            const percent = (progress / max) * 100;
            document.getElementById('enrollmentProgressBar').style.width = percent + '%';
        } else if (line === 'STATE:DONE') {
            document.getElementById('enrollmentInstructions').innerText = "Enrollment successful!";
            document.getElementById('enrollmentOrb').className = 'face-orb success';
            setTimeout(() => {
                navLinks[0].click();
            }, 3000);
        }
    }
});

overviewTestBtn.onclick = () => {
    setHeroState('scanning', 'Looking for you...', 'VERIFYING');
    ipcRenderer.send('run-faceid', 'test', currentConfig.strictness, null, true);
};

ipcRenderer.on('faceid-closed', (event, mode, code, output) => {
    if (mode === 'enroll') {
        if (code === 0) {
            loadFaces();
        } else {
            document.getElementById('enrollmentInstructions').innerText = "Enrollment failed or was cancelled.";
            document.getElementById('enrollmentOrb').className = 'face-orb error';
        }
    } else {
        if (code === 0) {
            let matchedName = "Recognized";
            if (output && output.includes("MATCH:")) {
                matchedName = output.split("MATCH:")[1].trim().split('\n')[0];
            }
            setHeroState('success', 'Welcome back, ' + matchedName, 'VERIFIED');
        } else {
            setHeroState('error', 'Face not recognized', 'FAILED');
        }
    }
    
    setTimeout(() => {
        setHeroState('ready', 'Face authentication ready', 'READY');
    }, 4000);
});
