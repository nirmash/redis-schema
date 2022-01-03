FROM alpine:latest


ENV LUA_MAJOR_VERSION 5.4
ENV LUA_MINOR_VERSION 3
ENV LUA_VERSION ${LUA_MAJOR_VERSION}.${LUA_MINOR_VERSION}

# Dependencies
RUN apk add --update make tar unzip gcc openssl-dev readline-dev curl libc-dev
RUN apk add ncurses-dev

RUN curl -L http://www.lua.org/ftp/lua-${LUA_VERSION}.tar.gz | tar xzf -
WORKDIR /lua-$LUA_VERSION

# build lua
RUN make linux test
RUN make install

WORKDIR /

# lua env
ENV WITH_LUA /usr/local/
ENV LUA_LIB /usr/local/lib/lua
ENV LUA_INCLUDE /usr/local/include


RUN rm /lua-$LUA_VERSION -rf

RUN apk update \
    && apk add --virtual build-dependencies \
        build-base \
        gcc \
        wget \
        git \
    && apk add \
        bash

RUN apk update 
RUN apk add bash curl make gcc clang redis outils-md5
RUN apk add --no-cache curl jq python3 py3-pip
RUN pip3 install -q Flask==0.11.1 requests==2.18.4 redis json2html datetime
RUN apk add python3-dev

RUN mkdir src
RUN mkdir rmutil

ADD src src
ADD rmutil rmutil


WORKDIR /

COPY redismodule.h redismodule.h
COPY preps.sh preps.sh
RUN chmod u+x preps.sh
RUN mkdir /shared_files 

WORKDIR /src
RUN make

EXPOSE 6379

WORKDIR /

CMD ["redis-server","--protected-mode no","--logfile","/tmp/redis.txt","--loadmodule","/src/module.so"]