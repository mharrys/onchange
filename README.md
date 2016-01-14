# onchange
Execute string command or script on file or directory modifications. Everytime
a modification happens it will terminate any previous running process.

# Dependencies
For GNU/Linux, inotify-tools must be installed.

## Examples

### Print "changed" when "foo.txt" is modified
    onchange foo.txt "echo 'changed'"

### Compile LaTeX document when files in directory are modified
    onchange . "pdflatex document.tex"

### Execute script when files in directory are modified
    onchange . ./script.sh

Note that if a running process is started in `script.sh` it will not be
terminated if the directory is modified again. This is because Bash will
not forward the signal to the child process.

There are ways around this, you can read about it
[here](https://veithen.github.io/2014/11/16/sigterm-propagation.html).

## Flags
Add `-r` for adding watchers recursively and `-h` for including hidden files
(ignored in recursive mode as default).
