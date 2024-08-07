FROM gcc:latest

RUN apt-get update && apt-get install -y \
    vim \
    gdb \
    make \
    git \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

COPY . .

CMD ["/bin/bash"]
