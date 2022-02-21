FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y \
    software-properties-common  \
    git \
    gcc \
    g++ \
    make \
    cmake

COPY . /code/wukong

RUN  rm -rf /code/wukong/build &&  \
     mkdir /code/wukong/build &&   \
     mkdir /wukong

WORKDIR /code/wukong/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/wukong .. &&  \
    make -j12

RUN cp -r /code/wukong/build/out/* /wukong/

# Tidy up
RUN apt-get clean autoclean &&  \
    apt-get autoremove

CMD ["/wukong/bin/gateway"]
