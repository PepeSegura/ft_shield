NAME = "debian"

Vagrant.configure("2") do |config|

  config.vm.box = "onlyoffice/base-debian13"

  config.vm.boot_timeout = 600
  config.ssh.forward_agent = true

#   config.vm.provision "Base-Image", type: "shell", run: "once", inline: <<-'SHELL'
#     apk update # && apk upgrade
#     apk add iptables build-base openssh-client netcat-openbsd linux-lts linux-virt virtualbox-guest-additions curl wget --no-cache
#     rc-update add virtualbox-guest-additions default
#     echo "vboxsf" >> /etc/modules
#     adduser vagrant vboxsf  # Add user to vboxsf group
#   SHELL

#   config.vm.provision "Mount-SyncedFolder", type: "shell", run: "always", inline: <<-'SHELL'
#     modprobe overlay
#     modprobe vboxsf
#     mount -t vboxsf -o uid=1000,gid=1000 vagrant_data /vagrant_data
#     echo "overlay" | tee -a /etc/modules
#   SHELL

  config.vm.define NAME do |server|
    server.vm.hostname = NAME
    server.vm.provider "virtualbox" do |vbox|
      vbox.customize ["modifyvm", :id, "--name", NAME]
      vbox.memory = 2048
      vbox.cpus = 4
    end

    server.vm.provision "Install programs", type: "shell", path: "install.sh", run: "always"
  end

end