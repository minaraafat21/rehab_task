import torch
import torch.nn as nn
import numpy as np
from collections import deque
import socket
import time

# ==========================================
# 1. MODEL DEFINITION
# ==========================================
class LocomotionHybrid(nn.Module):
    def __init__(self, num_classes=7):
        super(LocomotionHybrid, self).__init__()
        self.conv_layer = nn.Sequential(
            nn.Conv1d(6, 64, kernel_size=5, padding=2),
            nn.BatchNorm1d(64), nn.ReLU(),
            nn.Conv1d(64, 64, kernel_size=3, padding=1),
            nn.BatchNorm1d(64), nn.ReLU(),
            nn.MaxPool1d(2),
            nn.Conv1d(64, 64, kernel_size=5, padding=2), 
            nn.BatchNorm1d(64), nn.ReLU(),
            nn.Conv1d(64, 64, kernel_size=3, padding=1),
            nn.BatchNorm1d(64), nn.ReLU(),
            nn.MaxPool1d(2),
            nn.Dropout(0.3)
        )
        self.lstm = nn.LSTM(input_size=64, hidden_size=128, num_layers=2, 
                            batch_first=True, bidirectional=True, dropout=0.4)
        self.fc = nn.Sequential(
            nn.Linear(128 * 2, 64), nn.ReLU(),
            nn.BatchNorm1d(64), nn.Dropout(0.5),
            nn.Linear(64, num_classes)
        )

    def forward(self, x):
        x = x.transpose(1, 2)
        x = self.conv_layer(x)
        x = x.transpose(1, 2)
        x, _ = self.lstm(x)
        x = torch.mean(x, dim=1) 
        return self.fc(x)

# ==========================================
# 2. INITIALIZATION
# ==========================================
# Load Model
model = LocomotionHybrid(num_classes=7)
model.load_state_dict(torch.load('best_model_finetuned.pth', map_location=torch.device('cpu')))
model.eval()
print("Model Loaded. Initializing UDP Server...")

# UDP Config
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

# Inference Config
WINDOW_SIZE = 250  
imu_buffer = deque(maxlen=WINDOW_SIZE)
CLASS_NAMES = {0: "Sitting", 1: "Standing", 2: "Walking", 3: "Running", 4: "Stairs Up", 5: "Stairs Down", 6: "Falling"}

print(f"Listening on port {UDP_PORT}...")

# ==========================================
# 3. LIVE SERVER LOOP
# ==========================================
while True:
    try:
        data, addr = sock.recvfrom(1024)
        message = data.decode('utf-8').strip()
        
        # Parse CSV: ax,ay,az,gx,gy,gz
        values = [float(v) for v in message.split(',') if v.strip()]
        
        if len(values) == 6:
            imu_buffer.append(values)
            
            # Run inference only when window is full
            if len(imu_buffer) == WINDOW_SIZE:
                input_array = np.array(imu_buffer)
                input_tensor = torch.tensor(input_array, dtype=torch.float32).unsqueeze(0)
                
                with torch.no_grad():
                    output = model(input_tensor)
                    _, predicted_idx = torch.max(output, 1)
                    class_index = predicted_idx.item()
                
                # Send prediction back to ESP32 with 'P' prefix
                response = f"P{class_index}\n".encode('utf-8')
                sock.sendto(response, addr)
                
                print(f"State: {CLASS_NAMES.get(class_index)} | Sent to ESP: P{class_index}")
                
                # Optional: Clear half the buffer for a "sliding window" effect 
                # (prevents waiting another 250 samples for the next update)
                # for _ in range(125): imu_buffer.popleft() 

    except Exception as e:
        print(f"Error: {e}")