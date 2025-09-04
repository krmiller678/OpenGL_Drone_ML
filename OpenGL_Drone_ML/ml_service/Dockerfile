FROM python:3.11-slim

WORKDIR /app

# Copy requirements first to leverage Docker caching
COPY requirements.txt .

# Install dependencies
RUN pip install --no-cache-dir -r requirements.txt

# Copy app code (might use for production)
# COPY ./ml_server /app

EXPOSE 5000
CMD ["python", "server.py"]