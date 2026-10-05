[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$ModelDir = "Models"

if (-not (Test-Path -Path $ModelDir)) {
    New-Item -ItemType Directory -Path $ModelDir | Out-Null
    Write-Host "Created Models directory."
}

$YuNetUrl = "https://github.com/opencv/opencv_zoo/raw/main/models/face_detection_yunet/face_detection_yunet_2023mar.onnx"
$YuNetFile = Join-Path $ModelDir "face_detection_yunet_2023mar.onnx"

$SFaceUrl = "https://github.com/opencv/opencv_zoo/raw/main/models/face_recognition_sface/face_recognition_sface_2021dec.onnx"
$SFaceFile = Join-Path $ModelDir "face_recognition_sface_2021dec.onnx"

Write-Host "Downloading YuNet face detection model..."
Invoke-WebRequest -Uri $YuNetUrl -OutFile $YuNetFile
Write-Host "YuNet downloaded."

Write-Host "Downloading SFace face recognition model..."
Invoke-WebRequest -Uri $SFaceUrl -OutFile $SFaceFile
Write-Host "SFace downloaded."

Write-Host "All models downloaded successfully!"
