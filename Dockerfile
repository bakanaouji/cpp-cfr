FROM ubuntu:latest

ENV HOME /root

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y build-essential
RUN apt-get install -y valgrind cmake wget

# Install Boost
RUN cd /home \
    && wget http://downloads.sourceforge.net/project/boost/boost/1.68.0/boost_1_68_0.tar.gz \
    && tar xfz boost_1_68_0.tar.gz \
    && rm boost_1_68_0.tar.gz \
    && cd boost_1_68_0 \
    && ./bootstrap.sh --with-libraries=serialization,filesystem \
    && ./b2 install

WORKDIR $HOME
