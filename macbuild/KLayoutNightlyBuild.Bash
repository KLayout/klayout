#!/bin/bash
#----------------------------------------------------------------------
# File: KLayoutNightlyBuild.Bash
#
# What: The Bash contents for "KLayoutNightlyBuild.app" script bundle
#       that invokes 'nightlyBuild.py' for automation.
#
# Last modified: 2025-11-05
#----------------------------------------------------------------------
dryrun="no"

#-------------------------------------------------------------------------
# Tool logging
#-------------------------------------------------------------------------
set -Eeuo pipefail
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8

LOG="/tmp/KLayoutNightlyBuild.$(date +%Y%m%d-%H%M%S).log"
{
  echo "=== START $(date) ==="
  echo "whoami: $(whoami)"
  echo "pwd:    $(pwd)"
  echo "shell:  $SHELL"
  echo "osvers: $(sw_vers -productVersion)"
  echo "ENV (partial):"
  env | grep -E '^(PATH|HOME|LANG|LC_|MyEmail)' | sort
} >>"$LOG" 2>&1

BASH_XTRACEFD=9
exec 9>>"$LOG"
set -x

exec >>"$LOG" 2>&1

#-------------------------------------------------------------------------
# Detect and set the OS name.
#-------------------------------------------------------------------------
osvers=$(sw_vers -productVersion)
osname=$(sw_vers -productName)

case "$osvers" in
  12.*) osname="Monterey" ;;
  13.*) osname="Ventura" ;;
  14.*) osname="Sonoma" ;;
  15.*) osname="Sequoia" ;;
  26.*) osname="Tahoe" ;;
  *) osname="Unknown" ;;
esac

#-------------------------------------------------------------------------
# Set PATH
#     to use /opt/local/bin/addr2line -> gaddr2line in MacPorts.
#-------------------------------------------------------------------------
export PATH=/opt/local/bin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin

#-------------------------------------------------------------------------
# Decide PYBIN (priority: env → MacPorts → Homebrew → Anaconda → /usr/bin)
# KLAYOUT_PYBIN or PYBIN
#-------------------------------------------------------------------------
PYBIN_CANDIDATES=()
# launchctl setenv KLAYOUT_PYBIN ... will work）
[[ -n "${KLAYOUT_PYBIN:-}" ]] && PYBIN_CANDIDATES+=("$KLAYOUT_PYBIN")
[[ -n "${PYBIN:-}" ]]         && PYBIN_CANDIDATES+=("$PYBIN")

# candidates
PYBIN_CANDIDATES+=(
  /opt/local/bin/python3               # MacPorts
  /usr/local/bin/python3               # Homebrew (Intel)
  /opt/homebrew/bin/python3            # Homebrew (Apple Silicon)
  /Applications/anaconda3/bin/python3  # Anaconda
  /usr/bin/python3                     # System
)

PYBIN=""
for p in "${PYBIN_CANDIDATES[@]}"; do
  if [[ -n "$p" && -x "$p" ]]; then
    PYBIN="$p"
    break
  fi
done

if [[ -z "$PYBIN" ]]; then
  echo "[FATAL] python3 not found. Tried: ${PYBIN_CANDIDATES[*]}"
  exit 127
fi

echo "[INFO] Using PYBIN=$PYBIN"
"$PYBIN" -V || true

#-------------------------------------------------------------------------
# Set build parameters
#-------------------------------------------------------------------------
workdir=$HOME/GitWork/klayout
passbox=$HOME/Dropbox/PrivatePassBox
drylog=nightlyBuild-dryrun.log
jobsCSV=nightlyBuild.csv

#-------------------------------------------------------------------------
# Set mailer (postfix) parameters
#
#   launchctl setenv MyEmail1 "my_real_email_address_1"
#   launchctl setenv MyEmail2 "my_real_email_address_2"
#
#   launchctl getenv MyEmail1
#   launchctl getenv MyEmail2
#-------------------------------------------------------------------------
if [[ -z "${MyEmail1:-}" || -z "${MyEmail2:-}" ]]; then
  if [[ -f "$HOME/.klayout_env" ]]; then
    # shellcheck disable=SC1090
    source "$HOME/.klayout_env"
  fi
fi
# Re-check
MyEmail1_val="${MyEmail1:-}"
MyEmail2_val="${MyEmail2:-}"

if [[ -z "$MyEmail1_val" ]]; then
  echo "[FATAL] MyEmail1 is not set. Run: launchctl setenv MyEmail1 'you@example.com'"
  exit 64
fi
if [[ -z "$MyEmail2_val" ]]; then
  echo "[WARN]  MyEmail2 is not set. Cc will be empty."
fi

title="Status of KLayout <nightlyBuild.py>"
addrFrom=$MyEmail1_val
addrTo=$MyEmail1_val
addrCc=$MyEmail2_val
mailer="sendmail"
sleeptime=5

####----------------------------------------------------------------------
#### Initialize
####----------------------------------------------------------------------
function Initialize()
{
    cd "$workdir"
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] PATH=$PATH PYBIN=$PYBIN" >> "$workdir/nightlyBuild-env.log"

    if [ -f "$jobsCSV" ]; then
        targetOp="--qttarget=$jobsCSV"
    else
        targetOp=""
    fi

    if [ -L ./nightlyBuild.py ]; then
        echo "OK! nightlyBuild.py symbolic link exists ..."
    else
        ln -s ./macbuild/nightlyBuild.py .
    fi

    if [ "$dryrun" == "yes" ]; then
        cmd=( "./nightlyBuild.py" $targetOp --build --pymod --dryrun )
        dryout="> $drylog"
    else
        cmd=( "./nightlyBuild.py" $targetOp --build --pymod --test )
        dryout=""
    fi

    line1="### Initialized Nightly Build ###"
    line2="    OS: $osname"
    msg1=$(printf "%s\n%s\n%s\n" "$line1" "$line2" "${cmd[*]}")
    SendMail "$msg1"
}

####----------------------------------------------------------------------
#### Upload log files
####----------------------------------------------------------------------
function UploadLog()
{
    shopt -s nullglob
    logfiles=(*.log)
    [ ${#logfiles[@]} -gt 0 ] && cp -p "${logfiles[@]}" "$passbox"
    shopt -u nullglob
}

####----------------------------------------------------------------------
#### Build
####----------------------------------------------------------------------
function Build()
{
    # Prepare build log file
    buildlogfile="$workdir/nightlyBuild-$(date +%Y%m%d-%H%M%S).log"
    timestamp_start=$(date "+%Y-%m-%d %H:%M:%S")
    echo "===== Nightly Build Started at $timestamp_start =====" > $buildlogfile

    # Start caffeinate in background to prevent sleep
    # -d : Prevent display sleep
    # -i : Prevent idle sleep
    # -m : Prevent disk sleep
    # -s : Prevent system sleep
    # -u : Declare user activity (optional)
    #caffeinate -dimsu &
    caffeinate -imsu &
    caff_pid=$!

    logmsg1="Started caffeinate (PID=$caff_pid) to prevent sleep during build"
    echo "$logmsg1" | tee -a $buildlogfile

    # Prepare trap cleanup message
    cleanupmsg="Cleaning up caffeinate (PID=$caff_pid)"
    # Set trap
    trap "echo '$cleanupmsg' | tee -a $buildlogfile; kill $caff_pid 2>/dev/null || true; wait $caff_pid 2>/dev/null || true" INT TERM EXIT

    # Run the build and test command
    build_test_cmd=( "$PYBIN" "${cmd[@]}" )
    echo "Executing: ${build_test_cmd[*]} $dryout" | tee -a "$buildlogfile"
    if [ "$dryrun" == "yes" ]; then
        "${build_test_cmd[@]}" >"$drylog" 2>&1
    else
        "${build_test_cmd[@]}"
    fi
    build_status=$?

    # Stop caffeinate
    kill "$caff_pid" 2>/dev/null || true
    wait "$caff_pid" 2>/dev/null || true

    logmsg2="Stopped caffeinate (PID=$caff_pid)"
    echo "$logmsg2" | tee -a $buildlogfile

    # Clear trap
    trap - INT TERM EXIT

    # Build result handling
    timestamp_end=$(date "+%Y-%m-%d %H:%M:%S")
    echo "===== Nightly Build Ended at $timestamp_end =====" >> $buildlogfile

    if [ $build_status -ne 0 ]; then
        line1="!!! Failed Nightly Build !!!"
        line2="    OS: $osname"
        msg2=$(printf "%s\n%s\n%s\n" "$line1" "$line2" "${cmd[*]}")
        echo "$msg2" >> $buildlogfile
        UploadLog
    else
        line1="### Succeeded Nightly Build ###"
        line2="    OS: $osname"
        msg2=$(printf "%s\n%s\n%s\n" "$line1" "$line2" "${cmd[*]}")
        echo "$msg2" >> $buildlogfile
        if [ "$dryrun" == "yes" ]; then
            UploadLog
        fi
    fi

    # Send mail with entire build log
    SendMail "$(cat $buildlogfile)"
    return $build_status
}

####----------------------------------------------------------------------
#### Send mail
####----------------------------------------------------------------------
function SendMail() {
$mailer -i -t <<EOF
From: $addrFrom
To: $addrTo
Cc: $addrCc
Subject: $title
Content-Type: text/plain; charset=UTF-8

$1
EOF
}

####----------------------------------------------------------------------
#### Sleep
####----------------------------------------------------------------------
function Sleep()
{
    sleep $1
    # 10 = 10s => 10 seconds
    # 5m => 5 minutes
}

####----------------------------------------------------------------------
#### Main
####----------------------------------------------------------------------
Initialize
Build
exit $?

# EOF
