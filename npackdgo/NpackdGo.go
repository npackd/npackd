package main

import (
	"fmt"
)

func main() {
	packages := make([]*Package, 0, 100)

	p := new(Package)
	p.name = "test"
	p.version = "1.1.2"
	packages = append(packages, p)

	for _, p = range packages {
		fmt.Println(p)
	}
}
