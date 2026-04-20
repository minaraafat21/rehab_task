import torch
import torch.nn as nn
import numpy as np
from collections import deque
import time
import socket

# # Use "0.0.0.0" to listen on all interfaces
# UDP_IP = "0.0.0.0"  
# UDP_PORT = 8080

# sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# try:
#     sock.bind((UDP_IP, UDP_PORT))
#     print(f"Server started. Listening on all interfaces at port {UDP_PORT}...")
# except Exception as e:
#     print(f"Could not bind to port: {e}")
#     exit()

# while True:
#     try:
#         data, addr = sock.recvfrom(1024)
#         message = data.decode('utf-8').strip()
        
#         # Split by comma and filter out empty strings
#         values = [v for v in message.split(',') if v.strip()]
        
#         if len(values) == 6:
#             ax, ay, az, gx, gy, gz = [float(v) for v in values]
            
#             # Simple threshold logic for testing
#             predicted_class = 2 if az > 10.5 else 1

#             # Send back to ESP32
#             response = f"{predicted_class}\n".encode('utf-8')
#             sock.sendto(response, addr)

#             print(f"From {addr[0]} | IMU: {values} | Class: {predicted_class}")
            
#     except ValueError:
#         print(f"Received malformed data: {message}")
#     except Exception as e:
#         print(f"Unexpected error: {e}")



class LocomotionHybrid(nn.Module):
    def __init__(self, num_classes=7):
        super(LocomotionHybrid, self).__init__()
        
        self.conv_layer = nn.Sequential(
            # Block 1
            nn.Conv1d(6, 64, kernel_size=5, padding=2),
            nn.BatchNorm1d(64),
            nn.ReLU(),
            nn.Conv1d(64, 64, kernel_size=3, padding=1),
            nn.BatchNorm1d(64),
            nn.ReLU(),
            nn.MaxPool1d(2),
            
            # Block 2
            nn.Conv1d(64, 64, kernel_size=5, padding=2), 
            nn.BatchNorm1d(64),
            nn.ReLU(),
            nn.Conv1d(64, 64, kernel_size=3, padding=1),
            nn.BatchNorm1d(64),
            nn.ReLU(),
            nn.MaxPool1d(2),
            nn.Dropout(0.3)
        )
        
        # LSTM 
        self.lstm = nn.LSTM(input_size=64, hidden_size=128, num_layers=2, 
                            batch_first=True, bidirectional=True, dropout=0.4)
        
        self.fc = nn.Sequential(
            nn.Linear(128 * 2, 64),
            nn.ReLU(),
            nn.BatchNorm1d(64),
            nn.Dropout(0.5),
            nn.Linear(64, num_classes)
        )

    def forward(self, x):
       # x shape: [Batch, Seq_Len, 6]
        x = x.transpose(1, 2)  # [Batch, 6, Seq_Len]
        x = self.conv_layer(x)
        x = x.transpose(1, 2)  # [Batch, Reduced_Seq_Len, 64]
        
        x, _ = self.lstm(x)
        
        # Global Average Pooling across the time dimension
        x = torch.mean(x, dim=1) 
        return self.fc(x)

# ==========================================
# 2. LOAD TRAINED WEIGHTS
# ==========================================
# Initialize the model
model = LocomotionHybrid(num_classes=7)


# Load the weights (map to CPU so it runs anywhere)
try:
    model.load_state_dict(torch.load('best_model_finetuned.pth', map_location=torch.device('cpu')))
    model.eval()  # Set model to evaluation mode (turns off dropout)
    print("PyTorch Model loaded successfully.")
except Exception as e:
    print(f"Error loading model weights: {e}")
    exit()


# ==========================================
# 4. BUFFER & CLASS MAPPING
# ==========================================
# IMPORTANT: This must match the sequence length you used when training!
# I am setting it to 50 as a default. Change it to whatever you used.
WINDOW_SIZE = 50
imu_buffer = deque(maxlen=WINDOW_SIZE)

# Update these to match your exact 7 classes!
CLASS_NAMES = {
    0: "Sitting", 
    1: "Standing", 
    2: "Walking", 
    3: "Running",
    4: "Stairs Up",
    5: "Stairs Down",
    6: "Falling"
}

print("Generating dummy data...")
test_data = []
for i in range(WINDOW_SIZE):
    sample = [
        0.1,  # accel_x
        0.2,  # accel_y
        9.8,  # accel_z (slight oscillation)
        0.01, # gyro_x
        0.01, # gyro_y
        0.01  # gyro_z
    ]
    test_data.append(sample)

print(f"Feeding {WINDOW_SIZE} samples into buffer...")

for i, sample in enumerate(test_data):
    imu_buffer.append(sample)
    
    # Check if buffer is ready for inference
    if len(imu_buffer) == WINDOW_SIZE:
        # Prepare input tensor [Batch, Seq_Len, Features]
        input_array = np.array(imu_buffer)
        input_tensor = torch.tensor(input_array, dtype=torch.float32).unsqueeze(0)
        
        # Run model
        with torch.no_grad():
            start_time = time.time()
            output = model(input_tensor)
            _, predicted_idx = torch.max(output, 1)
            end_time = time.time()
            
            class_index = predicted_idx.item()
            locomotion_state = CLASS_NAMES.get(class_index, "Unknown")
            
            print("\n" + "="*30)
            print(f"INFERENCE SUCCESSFUL")
            print(f"Predicted Class: {class_index} ({locomotion_state})")
            print(f"Inference Time: {(end_time - start_time)*1000:.2f} ms")
            print("="*30)

# ==========================================
# 5. THE REAL-TIME INFERENCE LISTENER
# ==========================================
# def on_sensor_update(event):
# data = event.data
# data = dummy_data[0]  # Use dummy data for testing

# # Ensure we are looking at a valid IMU update, not a status update
# if isinstance(data, dict) and 'accel_x' in data:
#     # 1. Extract the 6 features
#     features = [
#         float(data.get('accel_x', 0)),
#         float(data.get('accel_y', 0)),
#         float(data.get('accel_z', 0)),
#         float(data.get('gyro_x', 0)),
#         float(data.get('gyro_y', 0)),
#         float(data.get('gyro_z', 0))
#     ]
    
#     # 2. Add to sliding window
#     imu_buffer.append(features)
#     print(f"Buffer Length: {len(imu_buffer)} | Latest IMU: {features}")
#     print(f"window size: {WINDOW_SIZE}")
    
#     # 3. Once buffer is full, run inference
#     if len(imu_buffer) == WINDOW_SIZE:
#         # Convert to numpy array, then to PyTorch Tensor
#         input_array = np.array(imu_buffer)
        
#         # Convert to tensor and add Batch dimension: [1, Seq_Len, 6]
#         input_tensor = torch.tensor(input_array, dtype=torch.float32).unsqueeze(0)
        
#         # 4. Predict (Disable gradients for faster inference)
#         with torch.no_grad():
#             output = model(input_tensor)
            
#             # Get the index of the highest probability class
#             _, predicted_idx = torch.max(output, 1)
#             class_index = predicted_idx.item()
        
#         # Map index to human-readable string
#         locomotion_state = CLASS_NAMES.get(class_index, "Unknown")
#         print(f"Predicted Class [{class_index}]: {locomotion_state}")
        
