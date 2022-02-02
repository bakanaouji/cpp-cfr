FROM ubuntu:20.04

ENV HOME /root

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y tzdata
RUN apt-get install -y build-essential
RUN apt-get install -y valgrind cmake wget

# Install Boost
RUN cd /home \
    && wget https://boostorg.jfrog.io/artifactory/main/release/1.76.0/source/boost_1_76_0.tar.gz \
    && tar xfz boost_1_76_0.tar.gz \
    && rm boost_1_76_0.tar.gz \
    && cd boost_1_76_0 \
    && ./bootstrap.sh --with-libraries=serialization,filesystem \
    && ./b2 install

RUN mkdir $HOME/cpp-cfr
COPY ./ $HOME/cpp-cfr
WORKDIR $HOME/cpp-cfr
