#!/usr/bin/zsh

script_home=$(dirname $(readlink -f $0))

. $script_home/common.inc

function wrapper::usage {
   message=$(cat <<_END_
${yl}USAGE:${rs}
    $(basename $1) [OPTIONS | <environment> SUBCOMMAND [OPTIONS]]

${yl}OPTIONS:${rs}
   ${gr}--help${rs}                     Print this message
   ${gr}--list-env${rs}                 List valid environments
   ${gr}--get-env <ENVIRONMENT>${rs}    Show an environment
   ${gr}--edit-env <ENVIRONMENT>${rs}   Edit an environment
_END_
)

   printf "$message"

   exit 1
}

function wrapper::list_env {
   jq -r '.|keys[]' <$config_file | sort
}

function wrapper::get_env {
   env=$1

   if jq -r '.|keys[]' <$config_file | grep "^$env$" >/dev/null 2>&1; then
      jq '."'$env'" // empty' <$config_file
   else
      echo "Configuration for environment '$env' not found" >/dev/stderr
      exit 1
   fi
}

function wrapper::edit_env {
   env=$1
   config_edit=$(tempfile)

   trap 'rm "$config_edit"' EXIT

   wrapper::get_env $env >$config_edit

   if [[ -n "$EDITOR" ]]; then
      eval $EDITOR $config_edit || exit 1
   else
      vi $config_edit || exit 1
   fi

   config_dir=$(dirname $config_file)
   config_tmp=$(tempfile -d $config_dir)

   (cat $config_edit | jq '."'$env'" = inputs' $config_file - >$config_tmp) && mv $config_tmp $config_file
}

function wrapper::process_args {
   program=$1

   shift

   case $1 in
      --help)
         wrapper::usage $program
         ;;

      --list-env)
         if [[ $# -ne 1 ]]; then
            wrapper::usage $program
         fi

         wrapper::list_env

         exit 0
         ;;

      --get-env)
         if [[ $# -ne 2 ]]; then
            wrapper::usage $program
         fi

         wrapper::get_env $2

         exit 0
         ;;

      --edit-env)
         if [[ $# -ne 2 ]]; then
            wrapper::usage $program
         fi

         wrapper::edit_env $2

         exit 0
         ;;
   esac
}
