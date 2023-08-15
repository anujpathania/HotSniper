#!/usr/bin/env bash

###############################################################
# Script for generating patches in a standardized way.
###############################################################


####################
# Globals
####################

search_dir=""
patch_file=""

####################
# Helper functions 
####################

function display_help() {
    echo "Usage: ./create_patch.sh [OPTIONS] SEARCH_DIR PATCH_FILE"
    echo "Description:"
    echo "  The script recurses into SEARCH_DIR looking for '-stock' and '-patched' infixed files."
    echo "  A diff between these files is extracted and printed to the PATCH_FILE."
    echo "Options:"
    echo "  --help         Display this help page"
    echo
}

function check_directory() {
    if [ ! -d "$1" ]; then
        echo "Provided path '$1' does not exist."
        display_help
        exit 1
    fi
}

function expand_path() {
    echo "$(realpath "$1")"
}

####################
# Parse options.
####################

while [[ $# -gt 0 ]]; do
    key="$1"

    case $key in
        --help)
            display_help
            exit 0
            ;;
        *)
            if [ -z "$search_dir" ]; then
                search_dir="$(expand_path "$1")"
                check_directory "$search_dir"
            elif [ -z "$patch_file" ]; then
                patch_file="$(expand_path "$1")"
                check_directory "$(dirname "$1")"
            else
                echo "Provided too many arguments"
                display_help
                exit 1
            fi
            shift
            ;;
    esac
done

if [ -z "$search_dir" ] || [ -z "$patch_file" ]; then
    echo "Please provide at least arguments for SEARCH_DIR and PATCH_FILE"
    display_help
    exit 1
fi

####################
# Navigate to search directory.
####################
cd "$search_dir"

####################
# Clean target files with stock HotSniper version.
####################

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

rm -f "$patch_file"

for index in "${!targetpaths[@]}"; do
    targetpath="${targetpaths[$index]#./}"
    patchedpath="${patchedpaths[$index]#./}"

    diff -u "$targetpath" "$patchedpath" >> "$patch_file"
done

echo "Bye!"
