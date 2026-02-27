docker build -t synonym-generator-gpu .
docker run --rm --gpus all -v $(pwd)/texts:/app/texts synonym-generator-gpu

