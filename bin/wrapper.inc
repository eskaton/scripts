# vim: syntax=zsh

script_home=$(dirname $(readlink -f $0))

. $script_home/common.inc

function wrapper::usage {
   message=$(cat <<_END_
${yl}USAGE:${rs}
    $(basename $1) [OPTIONS | <environment> SUBCOMMAND [OPTIONS]]

${yl}OPTIONS:${rs}
   ${gr}--help${rs}                          Print this message
   ${gr}--list-env${rs}                      List valid environments
   ${gr}--get-env <ENVIRONMENT>${rs}         Show an environment
   ${gr}--edit-env <ENVIRONMENT>${rs}        Edit an environment
   ${gr}--add-env <ENVIRONMENT>${rs}         Add a new environment
   ${gr}--copy-env <EXISTING> <NEW>${rs}     Copy an environment
   ${gr}--delete-env <ENVIRONMENT>${rs}      Delete an environment
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

   trap 'rm -f "$config_edit" "$config_tmp"' EXIT

   (cat $config_edit | jq '."'$env'" = inputs' $config_file - >$config_tmp) && mv $config_tmp $config_file
}

function wrapper::add_env {
   env=$1
   template=$2

   if wrapper::list_env | grep $env >/dev/null 2>&1; then
      error "Environment $env already exists"
   fi

   config_edit=$(tempfile)

   trap 'rm "$config_edit"' EXIT

   echo "$template" >$config_edit

   if [[ -n "$EDITOR" ]]; then
      eval $EDITOR $config_edit || exit 1
   else
      vi $config_edit || exit 1
   fi

   config_dir=$(dirname $config_file)
   config_tmp=$(tempfile -d $config_dir)

   trap 'rm -f "$config_edit" "$config_tmp"' EXIT

   (cat $config_edit | jq '."'$env'" = inputs' $config_file - >$config_tmp) && mv $config_tmp $config_file
}

function wrapper::copy_env {
   env=$1
   new_env=$2

   if ! wrapper::list_env | grep $env >/dev/null 2>&1; then
      error "Environment $env doesn't exists"
   fi

   if wrapper::list_env | grep $new_env >/dev/null 2>&1; then
      error "Environment $new_env already exists"
   fi

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

   trap 'rm -f "$config_edit" "$config_tmp"' EXIT

   (cat $config_edit | jq '."'$new_env'" = inputs' $config_file - >$config_tmp) && mv $config_tmp $config_file
}

function wrapper::delete_env {
   env=$1

   if ! wrapper::list_env | grep $env >/dev/null 2>&1; then
      error "Environment $env doesn't exists"
   fi

   config_dir=$(dirname $config_file)
   config_tmp=$(tempfile -d $config_dir)

   trap 'rm -f "$config_tmp"' EXIT

   (jq 'del(."'$env'")' < $config_file >$config_tmp) && mv $config_tmp $config_file
}


#jq 'del(."prod-ssl")'

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

      --add-env)
         if [[ $# -ne 2 ]]; then
            wrapper::usage $program
         fi

         if [[ -z "$template" ]]; then
            error "template variable not defined"
         fi

         wrapper::add_env $2 "$template"

         exit 0
         ;;

      --copy-env)
         if [[ $# -ne 3 ]]; then
            wrapper::usage $program
         fi

         wrapper::copy_env $2 $3

         exit 0
         ;;

      --delete-env)
         if [[ $# -ne 2 ]]; then
            wrapper::usage $program
         fi

         wrapper::delete_env $2

         exit 0
         ;;
   esac
}
