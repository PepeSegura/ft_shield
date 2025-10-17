FROM debian:latest

RUN apt-get update && \
    apt-get install -y zsh build-essential curl git btop netcat-openbsd net-tools && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install Oh My Zsh
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

RUN chsh -s $(which zsh)
CMD ["zsh"]
