# vim: syntax=zsh

script_home=$(dirname $(readlink -f $0))

. $script_home/common.inc

opt_help=("help" "" "Show help information" "")
opt_completion=("completion" "<SHELL>" "Generate shell completion" "zsh")
opt_list_env=("list-env" "" "List valid environments" "")
opt_get_env=("get-env" "<ENVIRONMENT>" "Show an environment" "\$env")
opt_edit_env=("edit-env" "<ENVIRONMENT>" "Edit an environment" "\$env")
opt_add_env=("add-env" "<ENVIRONMENT>" "Add a new environment" "\$env")
opt_copy_env=("copy-env" "<EXISTING> <NEW>" "Copy an environment" "\$env")
opt_delete_env=("delete-env" "<ENVIRONMENT>" "Delete an environment" "\$env")

all_options=("opt_help" "opt_completion" "opt_list_env" "opt_get_env" "opt_get_env" "opt_edit_env" "opt_add_env" "opt_copy_env" "opt_delete_env")

function wrapper::usage {

   local options

   for i in $all_options; do
      opt=$(eval echo \$$i"[1]")
      args=$(eval echo \$$i"[2]")
      text=$(eval echo \$$i"[3]")

      options="$options\n"$(printf "   ${gr}--%-10s %-20s${rs} %-50s" $opt "$args" "$text")
   done

   message=$(cat <<_END_
${yl}USAGE:${rs}
    $(basename $1) [OPTIONS | <environment> SUBCOMMAND [OPTIONS]]

${yl}OPTIONS:${rs} ${options}
_END_
)

   printf "$message"

   exit 1
}

function wrapper::completion {
   wrapper=$(basename $1)

   shift

   if [[ $# -ne 1 ]]; then
      wrapper::usage $wrapper
   fi

   shell=$1

   if [[ $1 != "zsh" ]]; then
      error "Completion is only supported for zsh" 
   fi

   program=$(echo $wrapper | sed 's/.$//')

   comp_opts=$(for i in $all_options; do
      opt=$(eval echo \$$i"[1]")
      text=$(eval echo \$$i"[3]")
      comp=$(eval echo \$$i"[4]")

      if [[ -z "$comp" ]]; then
         printf "      \"--%s:[%s]\" " $opt "$text"
      else
         printf "      \"--%s:[%s]:(%s)\" " $opt "$text" "$comp"
      fi

      done)

cat <<_END_
#compdef _$wrapper $wrapper

function _$wrapper {
   local line 

   env=\$(${wrapper} --list-env | tr '\n' ' ')

   function _environments {
      local -a environments
      environments=(\$(echo \$env))
      _describe 'environment' environments
   } 

   _arguments -C \\
      "1: :_environments" \\
      "*::arg:->args" \\
      + '(opt)' \\
$comp_opts

   if [[ -n "\$line[1]" ]]; then
      line[1]="$program"
      words[1]="$program"
      _$program
   fi
}
_END_
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

   if wrapper::list_env | grep "^$env$" >/dev/null 2>&1; then
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

   if ! wrapper::list_env | grep "^$env$" >/dev/null 2>&1; then
      error "Environment $env doesn't exists"
   fi

   if wrapper::list_env | grep "^$new_env$" >/dev/null 2>&1; then
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

   if ! wrapper::list_env | grep "^$env$" >/dev/null 2>&1; then
      error "Environment $env doesn't exists"
   fi

   config_dir=$(dirname $config_file)
   config_tmp=$(tempfile -d $config_dir)

   trap 'rm -f "$config_tmp"' EXIT

   (jq 'del(."'$env'")' < $config_file >$config_tmp) && mv $config_tmp $config_file
}

function wrapper::process_args {
   program=$1

   shift

   case $1 in
      --help)
         wrapper::usage $program
         ;;

      --completion)
         shift
         wrapper::completion $program $@

         exit 0
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
