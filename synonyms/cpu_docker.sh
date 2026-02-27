docker build -t synonym-generator .
docker run --rm -v $(pwd)/texts:/app/texts synonym-generator

