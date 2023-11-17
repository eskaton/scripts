# vim: syntax=zsh

if [[ -t 1 && $(tput colors) -ge 8 ]]; then
   rd='\033[0;31m'
   gr='\033[0;32m'
   yl='\033[0;33m'
   bl='\033[0;34m'
   rs='\033[0m'
fi

info() {
   printf "${bl}I: $1${rs}\n" >&2
}

warn() {
   printf "${yl}W: $1${rs}\n" >&2
}

error() {
   printf "${rd}E: $1${rs}\n" >&2
   exit 1
}
