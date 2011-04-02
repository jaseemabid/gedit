#!/bin/sh

if [ -d gedit.app ] && [ "$1x" = "-fx" ]; then
	rm -rf gedit.app
fi

gtk-mac-bundler gedit.bundle

function do_strip {
    name=$(mktemp -t bundle)
    st=$(stat -f %p "$1")
    
    strip -o "$name" -S "$1"
    mv -f "$name" "$1"
    
    chmod "$st" "$1"
	chmod u+w "$1"
}

# Strip debug symbols
for i in $(find gedit.app/Contents/Resources | grep -E '\.(so|dylib)'); do
    do_strip "$i"
done

for i in $(find gedit.app/Contents/Resources/bin -type f); do
    if [ -x "$i" ]; then
        do_strip "$i"
    fi
done

do_strip gedit.app/Contents/MacOS/gedit-bin
