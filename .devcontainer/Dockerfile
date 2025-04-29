FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

COPY install_dependencies.sh /home/vscode/

RUN apt-get update
RUN apt-get -y install git
RUN cd /home/vscode/ && ./install_dependencies.sh