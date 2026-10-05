<div align="center">
  <img src="https://img.shields.io/badge/Windows%2011-Ready-blue?style=for-the-badge&logo=windows11" alt="Windows 11" />
  <img src="https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++" />
  <img src="https://img.shields.io/badge/OpenCV-4.x-5C3EE8?style=for-the-badge&logo=opencv" alt="OpenCV" />
  <img src="https://img.shields.io/badge/ONNX-Runtime-005CED?style=for-the-badge&logo=onnx" alt="ONNX" />
  <img src="https://img.shields.io/badge/Electron-Dashboard-47848F?style=for-the-badge&logo=electron" alt="Electron" />

  <h1>🚀 FaceRig - Windows 11 Face ID Credential Provider</h1>
  <p><b>Unlock your Windows PC natively with blazing-fast AI Facial Recognition.</b></p>
</div>

---

## ⚡ What is FaceRig?

**FaceRig** is a custom, bare-metal Windows Credential Provider (V2 architecture) that seamlessly integrates state-of-the-art AI facial recognition directly into the Windows Lock Screen. It replaces the need for a PIN or Password by utilizing local neural networks to detect and authenticate your face in real-time.

Say goodbye to typing passwords and hello to instant, secure, and futuristic logins.

## ✨ Features

- 🔐 **Native Windows Integration**: Built using the official Windows Credential Provider framework (`ICredentialProviderV2`). Runs directly inside `LogonUI.exe`.
- 🧠 **Advanced Local AI**: Uses **YuNet** for ultra-fast face detection and **SFace** for highly accurate face recognition (via OpenCV's DNN module & ONNX runtime).
- ⚡ **Lightning Fast & Headless**: The AI engine runs completely headless on the lock screen, processing frames at high speed to instantly unlock your device the moment you sit down.
- 🎨 **Beautiful Electron Dashboard**: Comes with a sleek, modern desktop app for enrolling your face, managing biometric profiles, and securely saving your Windows credentials.
- 🛡️ **Zero Cloud Dependency**: 100% offline. Your facial biometric vectors and encrypted credentials never leave your machine.
- 🗄️ **Military-Grade Encryption**: Uses the Windows DPAPI (Data Protection API) to securely encrypt and store credentials tied directly to your machine's hardware signature.

## 🛠️ Architecture

FaceRig is composed of two main components:
1. **The Credential Provider DLL (`FaceIDProvider.dll`)**: Injected into the Windows Lock Screen. It orchestrates the authentication flow and automatically injects packed credentials (via `CredPackAuthenticationBufferW`) into Winlogon/LSA upon a successful face match.
2. **The AI Engine (`FaceIDApp.exe`)**: A C++ command-line tool that interfaces directly with the webcam. It runs in the background on the lock screen and communicates success/failure to the Credential Provider via process exit codes.

## 🚀 Getting Started

### Prerequisites
- Windows 10 or Windows 11 (64-bit)
- A webcam
- Visual Studio 2022 (with C++ Desktop Development workload)
- CMake
- Node.js & npm (for the Dashboard)

### 1. Build the C++ Backend
```bash
# Generate Visual Studio solutions
mkdir build
cd build
cmake ..

# Compile the project (Release mode)
cmake --build . --config Release
```

### 2. Download the AI Models
You will need the `.onnx` model files for YuNet and SFace. Place them inside the `Models/` directory:
- `face_detection_yunet_2023mar.onnx`
- `face_recognition_sface_2021dec.onnx`

### 3. Run the Electron Dashboard
```bash
cd FaceIDWindows/Dashboard
npm install
npm start
```
Use the dashboard to safely enroll your face and save your Windows password/PIN to the encrypted vault.

### 4. Register the Credential Provider
Once built, navigate to `FaceIDWindows/CredentialProvider` and merge `Register.reg` into your registry to tell Windows to load `FaceIDProvider.dll` on the Lock Screen. (Ensure the DLL path is correctly pointing to your build output).

## ⚠️ Important Privacy & Security Notes
- You must manually update `YOUR_USERNAME` inside `Credential.cpp` to match your local Windows account username before compiling, or it will fail to log you in.
- Hardcoded directory paths within the code (like `C:\FaceID\...`) must be updated to match where you actually place the compiled binaries on your local machine, or the system will not be able to find the AI executable from the lock screen.

## 📄 License
This project is for educational and experimental purposes. See the [LICENSE](LICENSE) file for details.

---
<div align="center">
  <i>Built with ❤️ by Astreas-Core</i>
</div>
