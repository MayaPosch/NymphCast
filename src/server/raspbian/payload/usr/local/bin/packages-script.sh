#!/bin/bash
# Start script for unattended package installation on a Raspberry Pi

# 1. INTERNAL SCRIPT BUSINESS
# logging of the script's run
logfile=configuration.log
templog=/dev/shm/$logfile
log() {
	echo "$@" >> $templog;
}
log "Unattended package installation by $0";
exec 2>>$templog;	# log all errors

# parameters - first the default values...
#node_js_source_url=""
packages_to_install=()

# ...then see if values can be read from a file, then remove that (may contain password)
[[ -f /boot/one-time-script.conf ]] && source /boot/one-time-script.conf &&\
 rm -f /boot/one-time-script.conf &&\
 log "Read parameters from /boot/one-time-script.conf" || log "Using default parameters";

# stop this service from running at boot again
# TODO: skip this step if updating packages fails.
log -n "Remove automatic running of package installation script: ";
systemctl disable packages-script.service && log OK || log FAILED;
systemctl disable systemd-time-wait-sync

# 6. PACKAGE INSTALLATION
log $'\nPACKAGE INSTALLATION';
export DEBIAN_FRONTEND=noninteractive	# avoid debconf error messages

log -n "Update APT package lists: "
apt-get update && log OK || log FAILED;

# if [[ $node_js_source_url ]]; then
	# log -n "Install nodejs: "
	# curl -sL "$node_js_source_url" | bash - && apt-get install -y nodejs && log OK || log FAILED;
# fi;

if [[ $packages_to_install ]]; then
	log -n "Install ${packages_to_install[0]}";
	for x in "${packages_to_install[@]:1}"; do
		log -n ", $x";
	done;
	log -n ": ";
	apt-get install -y "${packages_to_install[@]}" && log OK || log FAILED;
fi;

# Enable NymphCast service.
log -n "Set up automatic running of NymphCast service: ";
systemctl enable nymphcast.service && log OK || log FAILED;

# Append to the log on the boot partition 
echo >> /boot/$logfile
date >> /boot/$logfile
cat $templog >> /boot/$logfile
reboot

