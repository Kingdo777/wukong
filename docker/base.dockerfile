FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y \
    software-properties-common  \
    pkg-config  \
    git \
    gcc \
    g++ \
    make \
    cmake

RUN mkdir /code && cd /code && \
    git clone https://github.com/Kingdo777/wukong.git

RUN rm -rf /code/wukong/build &&  \
    mkdir /code/wukong/build &&   \
    mkdir /wukong

WORKDIR /code/wukong/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/wukong .. &&  \
    make

RUN cp -r /code/wukong/build/out/* /wukong/

# Tidy up
RUN apt-get clean autoclean &&  \
    apt-get autoremove

CMD ["/wukong/bin/gateway"]
