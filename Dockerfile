FROM ubuntu:16.04

ENV DEBIAN_FRONTEND noninteractive
RUN apt-get update && \
    apt-get -y install \
        gcc \
        gdb \
        git \
        libc6-dev \
        make && \
    rm -rf /var/lib/apt/lists/*

COPY ./run.sh /root

WORKDIR /root
RUN git clone https://github.com/longld/peda.git ~/peda && echo "source ~/peda/peda.py" >> ~/.gdbinit

ENTRYPOINT ["/root/run.sh"]
