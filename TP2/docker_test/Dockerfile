FROM python:3.12-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential nasm \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

COPY requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir -r /tmp/requirements.txt

COPY . /workspace

CMD ["bash", "/workspace/run_pipeline.sh"]
