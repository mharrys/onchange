package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"syscall"
	"time"

	"golang.org/x/exp/inotify"
)

func main() {
	hiddenPtr := flag.Bool("h", false, "include hidden")
	recursivePtr := flag.Bool("r", false, "recursive watcher")
	flag.Parse()

	args := flag.Args()
	if len(args) < 2 {
		fmt.Printf("Usage: %s PATH ACTION\n", os.Args[0])
		return
	}

	watcher, err := inotify.NewWatcher()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	err = addWatch(args[0], *recursivePtr, *hiddenPtr, watcher)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		return
	}

	acceptRate := int64(1000) // multiple events are sent at once
	now := timestamp()
	child := make(chan error, 1)
	var cmd *exec.Cmd
	for {
		select {
		case <-watcher.Event:
			curr := timestamp()
			if curr-now > acceptRate {
				now = curr
				cmd = waitNext(cmd, args[1])
				cmd.Stdout = os.Stdout
				cmd.Stderr = os.Stderr
				go func() {
					child <- cmd.Run()
				}()
			}
		case err := <-watcher.Error:
			fmt.Fprintln(os.Stderr, err)
			return
		case err := <-child:
			if err != nil {
				fmt.Fprintln(os.Stderr, err)
			}
		}
	}
}

func addWatch(from string, rec bool, hidden bool, watcher *inotify.Watcher) error {
	mask := inotify.IN_MODIFY | inotify.IN_CLOSE_WRITE | inotify.IN_MOVE |
		inotify.IN_CREATE
	walkFn := func(path string, _ os.FileInfo, err error) error {
		if !hidden && path != "." && strings.HasPrefix(path, ".") {
			return nil
		}
		return watcher.AddWatch(path, mask)
	}
	if rec {
		return filepath.Walk(from, walkFn)
	}
	return watcher.AddWatch(from, mask)
}

func waitNext(cmd *exec.Cmd, action string) *exec.Cmd {
	if cmd != nil {
		cmd.Process.Signal(syscall.SIGTERM)
		cmd.Wait()
	}
	return exec.Command("sh", "-c", action)
}

func timestamp() int64 {
	ns, ms := int64(time.Nanosecond), int64(time.Millisecond)
	return ns * time.Now().UnixNano() / ms
}
