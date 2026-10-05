const { ipcRenderer } = require('electron');

const authOrb = document.getElementById('authOrb');
const statusTitle = document.getElementById('statusTitle');
const statusSubtitle = document.getElementById('statusSubtitle');
const fallbackDiv = document.getElementById('fallbackDiv');
const fallbackPwd = document.getElementById('fallbackPwd');
const usePinBtn = document.getElementById('usePinBtn');

let isAuthenticating = true;

function setState(state) {
    if (!isAuthenticating && state !== 'success' && state !== 'error') return;
    
    // states: searching, detected, verifying, success, error
    authOrb.className = `glass-orb state-${state}`;
    
    switch(state) {
        case 'searching':
            statusTitle.innerText = "Looking for you";
            statusTitle.style.color = "var(--text-main)";
            statusSubtitle.innerText = "Position your face within the frame";
            break;
        case 'detected':
            statusTitle.innerText = "Face detected";
            statusTitle.style.color = "var(--text-main)";
            statusSubtitle.innerText = "Hold still for a moment";
            break;
        case 'verifying':
            statusTitle.innerText = "Verifying";
            statusTitle.style.color = "var(--text-main)";
            statusSubtitle.innerText = "Checking your identity";
            break;
        case 'success':
            statusTitle.innerText = "Welcome back";
            statusTitle.style.color = "var(--success)";
            statusSubtitle.innerText = "Authentication successful";
            break;
        case 'error':
            statusTitle.innerText = "Face not recognized";
            statusTitle.style.color = "var(--error)";
            statusSubtitle.innerText = "Try again or use another sign-in method";
            break;
    }
}

// Initial state
setState('searching');

// Wait a moment before starting face scan
setTimeout(() => {
    ipcRenderer.invoke('get-config').then((config) => {
        // Start engine
        ipcRenderer.send('run-faceid', 'test', config.strictness, null, true);
        
        // Simulate "detected" and "verifying" states before it finishes to give it that premium feel.
        // The engine takes a moment to boot. We can simulate the transition.
        setTimeout(() => {
            if (isAuthenticating) setState('detected');
        }, 800);
        
        setTimeout(() => {
            if (isAuthenticating) setState('verifying');
        }, 1500);
    });
}, 500);

ipcRenderer.on('faceid-closed', (event, mode, code) => {
    if (!isAuthenticating) return; // Ignore if they clicked Use PIN
    
    isAuthenticating = false;
    if (code === 0) {
        setState('success');
        
        // After 1.5s, fade out and close
        setTimeout(() => {
            document.body.classList.add('fade-out');
            setTimeout(() => ipcRenderer.send('close-lockscreen'), 800);
        }, 1500);
    } else {
        setState('error');
        
        setTimeout(() => {
            // Show fallback PIN
            usePinBtn.style.display = 'none';
            fallbackDiv.classList.add('show');
            fallbackPwd.focus();
        }, 1500);
    }
});

usePinBtn.onclick = () => {
    usePinBtn.style.display = 'none';
    fallbackDiv.classList.add('show');
    fallbackPwd.focus();
    isAuthenticating = false; // Stop the face ID visual logic
    
    statusTitle.innerText = "Sign-in options";
    statusTitle.style.color = "var(--text-main)";
    statusSubtitle.innerText = "Enter your PIN to continue";
    authOrb.className = "glass-orb"; // Reset to neutral
};

fallbackPwd.addEventListener('keyup', (e) => {
    if (e.key === 'Enter') {
        const pw = fallbackPwd.value;
        if (pw.length > 0) {
            statusTitle.innerText = "Verifying PIN";
            statusTitle.style.color = "var(--text-main)";
            statusSubtitle.innerText = "Checking credentials";
            authOrb.className = "glass-orb state-verifying";
            ipcRenderer.send('verify-password', pw);
        }
    }
});

ipcRenderer.on('password-verified', (event, success) => {
    if (success) {
        setState('success');
        setTimeout(() => {
            document.body.classList.add('fade-out');
            setTimeout(() => ipcRenderer.send('close-lockscreen'), 800);
        }, 1500);
    } else {
        setState('error');
        statusTitle.innerText = "Incorrect PIN";
        statusSubtitle.innerText = "Please try again";
        fallbackPwd.value = '';
    }
});
