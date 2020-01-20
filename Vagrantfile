$setup = <<-SCRIPT
sudo apt-get update
SCRIPT

Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/bionic64"
  config.vm.synced_folder ".", "/hux-kernel"
  config.vm.provider "virtualbox" do |v|
    v.gui = false
    v.cpus = 4
    v.memory = 8192
    v.customize [
        "modifyvm", :id, "--uartmode1", "disconnected"  # Disable logs.
    ]
  end
  config.vm.provision "shell", privileged: false, inline: $setup
end
