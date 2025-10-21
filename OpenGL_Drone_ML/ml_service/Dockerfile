FROM python:3.11-slim

WORKDIR /app

# Open3D needs access to DX11
RUN apt-get update && apt-get install -y --no-install-recommends build-essential apt-utils ca-certificates gnupg libgl1 libgl1-mesa-dri libglib2.0-0 libx11-6 libxcursor1 libxrandr2 libxinerama1 libxi6 && rm -rf /var/lib/apt/lists/*

# Copy requirements first to leverage Docker caching
COPY requirements.txt .

# Install dependencies
RUN pip install --no-cache-dir torch==2.3.0+cpu -f https://download.pytorch.org/whl/cpu/torch_stable.html
RUN pip install --no-cache-dir -r requirements.txt

# Copy app code (might use for production)
COPY ./ml_server /app

EXPOSE 5000
CMD ["python", "server.py"]