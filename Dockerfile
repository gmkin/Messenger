FROM python:3.6

WORKDIR /d_server
ADD . /d_server
CMD python /d_server/server.py -v 5000 10 "FROM DOCKER MOTD"
