package main

import "fmt"

type PackageVersion struct {
	Package string
	Version Version
}

func (p PackageVersion) String() string {
	return fmt.Sprintf("%v %v", p.Package, p.Version)
}
