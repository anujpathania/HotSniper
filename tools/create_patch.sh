#!/usr/bin/env bash

###############################################################
# Script for generating and optionally applying PARSEC patches.
###############################################################

####################
# Parse options.
####################
apply = 0

function display_help() {
    echo "Usage: ./create_patch.sh [OPTIONS]"
    echo "Options:"
    echo "  --apply        Apply the generated patch file"
    echo "  --help         Display this help page"
    echo
}

# Parse command line arguments
if [[ "$1" == "--help" ]]; then
    display_help
    exit 0
elif [[ "$1" == "--apply" ]]; then
    apply = 1
fi

####################
# Navigate to correct start directory.
####################
cd ../benchmarks/parsec

####################
# Clean target files with stock HotSniper version.
####################

# Store the stock file paths in an array
stockpaths=()
targetpaths=()
patchedpaths=()
while IFS= read -r -d '' filepath; do
    stockpaths+=("$filepath")
done < <(find . -type d -name "obj" -prune -o -type f -name "*-stock*" -print0)

for stockpath in "${stockpaths[@]}"; do
    target_file=$(echo "$stockpath" | sed 's/-stock//')
    patched_file=$(echo "$stockpath" | sed 's/-stock/-patched/')
    
    # Check if the target file exists
    if [ -f "$target_file" ] && [ -f "$patched_file" ]; then
        targetpaths+=("$target_file")
        patchedpaths+=("$patched_file")
        echo "Copying stock version to file: $target_file"
        cp -f "$stockpath" "$target_file"
    else
        echo "Either the target file OR patch file does not exist for stock file: $stockpath"
        exit 1
    fi
done


####################
# Write diff of patched and cleaned target files
####################

rm -f ./patches/heartbeat.patch

for index in "${!targetpaths[@]}"; do
    targetpath="${targetpaths[$index]#./}"
    patchedpath="${patchedpaths[$index]#./}"

    diff -u "$targetpath" "$patchedpath" >> patches/heartbeat.patch
done


####################
# Apply the patch
####################
if [[ apply == 1 ]]; then
    echo "Applying generated patch file"
    patch -p1 -dparsec-2.1 < patches/heartbeat.patch
fi

echo "Bye!"
