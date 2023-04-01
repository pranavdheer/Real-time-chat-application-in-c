#!/bin/sh

cc helper.c -c
cc -pthread client.c helper.o -o client
cc -pthread server.c helper.o -o server