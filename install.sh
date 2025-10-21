apt-get update && \
    apt-get install -y zsh build-essential curl git btop  net-tools && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
